#!/usr/bin/env python3
"""Fail-closed local evidence, NOT hermetic/reproducible build attestation."""
import argparse
import datetime
import importlib.util
import json
import os
from pathlib import Path
import runpy
import shlex
import signal
import subprocess
import tempfile
import time

spec = importlib.util.spec_from_file_location('identity', Path(__file__).with_name('education-successor.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

MANAGED = 'fixture-firmware/app/managed_components'
GENERATED = ('fixture-firmware/build/', 'fixture-firmware/runtime/target/', 'fixture-firmware/logs/')
OUTPUTS = ['entropylab_fixture.bin', 'entropylab_fixture.elf', 'entropylab_fixture.map',
           'bootloader/bootloader.bin', 'partition_table/partition-table.bin',
           'flasher_args.json', 'compile_commands.json', 'project_description.json']
LIMITS = ['Local expected inventories are operator trust anchors, not upstream signatures.',
          'System /usr/bin tools, dynamic libraries, kernel and hardware are not authenticated.',
          'Process groups do not contain descendants that escape with setsid; no sandbox claim.',
          'No network namespace: offline flags are not an OS network isolation guarantee.',
          'Concurrent same-UID/root writers and uncatchable termination require external isolation/stale-run detection.',
          'No reproducibility, independent binary acceptance or hardware qualification.']


def inventory(root):
    return {n: {'sha256': m.sha((root / n).read_bytes()), 'bytes': (root / n).stat().st_size}
            for n in sorted(m.tree_files(root))}


def validate_inputs(source, entries, managed, post=False):
    combined = dict(entries)
    for name, info in managed.items():
        path = MANAGED + '/' + name
        m.require(path not in combined, 'dependency overlaps source manifest')
        combined[path] = info
    m.validate_tree(source, combined)
    actual = m.tree_files(source)  # rejects links, including in generated outputs
    extra = actual - set(combined)
    m.require(not extra or (post and all(n.startswith(GENERATED) for n in extra)),
              'unexpected build input inventory: ' + repr(sorted(extra)[:10]))
    if not post:
        for prefix in GENERATED:
            m.require(not os.path.lexists(source / prefix), 'preexisting output directory')
    # Reject unexpected empty directories too: Cargo/component discovery sees directories.
    allowed_dirs = {str(p) for n in combined for p in Path(n).parents if str(p) != '.'}
    for parent, dirs, _ in os.walk(source):
        for name in dirs:
            rel = (Path(parent) / name).relative_to(source).as_posix()
            generated = any(rel == p.rstrip('/') or rel.startswith(p) for p in GENERATED)
            m.require(rel in allowed_dirs or (post and generated), 'unexpected input directory: ' + rel)


def atomic_status(path, status):
    payload = m.canonical(status)  # fail before touching previous receipt
    m.no_links(path)
    m.private_directory(path.parent)
    fd, temporary = tempfile.mkstemp(prefix='.receipt-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)  # atomic; never follows destination symlinks
        fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)
    finally:
        if os.path.lexists(temporary):
            os.unlink(temporary)


def stop_group(child, grace=1):
    def send(sig):
        try:
            os.killpg(child.pid, sig)
        except ProcessLookupError:
            pass
    send(signal.SIGTERM)
    # The leader's exit says nothing about descendants. Always escalate the group.
    time.sleep(grace)
    send(signal.SIGKILL)
    child.wait(timeout=10)
    # Linux /proc: zombies cannot write. Wait for every live member of this group.
    deadline = time.monotonic() + 10
    while True:
        live = []
        for p in Path('/proc').glob('[0-9]*/stat'):
            try:
                fields = p.read_text().rsplit(')', 1)[1].split()
                if int(fields[2]) == child.pid and fields[0] != 'Z':
                    live.append(p)
            except (FileNotFoundError, ProcessLookupError):
                pass
        if not live:
            return
        if time.monotonic() > deadline:
            raise RuntimeError('process group still has live writers; evidence not sealed')
        time.sleep(.02)


def build_environment(work, roots):
    env = {'PATH': '/usr/bin:/bin', 'LANG': 'C.UTF-8', 'LC_ALL': 'C.UTF-8',
           'PYTHONDONTWRITEBYTECODE': '1', 'PYTHONOPTIMIZE': '0', 'PYTHONNOUSERSITE': '1',
           'CARGO_NET_OFFLINE': 'true', 'IDF_CCACHE_ENABLE': '0',
           'GIT_CONFIG_NOSYSTEM': '1', 'GIT_CONFIG_GLOBAL': '/dev/null'}
    for key in ['HOME', 'TMPDIR', 'CARGO_HOME', 'IDF_COMPONENT_CACHE_PATH', 'XDG_CACHE_HOME']:
        env[key] = str(m.fresh_directory(work / key.lower()))
    env.update({key: str(value['path']) for key, value in roots.items() if key not in ('TOOL_BIN', 'PYTHON_BIN')})
    if 'TOOL_BIN' in roots:
        env['PATH'] = str(roots['TOOL_BIN']['path']) + ':' + env['PATH']
    if 'PYTHON_BIN' in roots:
        env['PATH'] = str(roots['PYTHON_BIN']['path']) + ':' + env['PATH']
    return env


def installation_inventory(root, bind_targets=True):
    """Physical entries only; directory links are never traversed."""
    import stat
    result = {}
    for parent, dirs, files in os.walk(root, followlinks=False):
        for name in dirs + files:
            p = Path(parent) / name
            mode = p.lstat().st_mode
            rel = p.relative_to(root).as_posix()
            if stat.S_ISLNK(mode):
                result[rel] = {'link': os.readlink(p), 'resolved': str(p.resolve(strict=True))}
            elif stat.S_ISREG(mode):
                result[rel] = {'sha256': m.sha(p.read_bytes()), 'bytes': p.stat().st_size}
            else:
                m.require(stat.S_ISDIR(mode), 'special installation entry: ' + str(p))
                result[rel] = {'directory': True}
    if bind_targets:
        for info in result.values():
            if 'link' in info:
                target = Path(info['resolved'])
                info['target_sha256'] = m.sha(target.read_bytes()) if target.is_file() else m.sha(m.canonical(installation_inventory(target, bind_targets=False)))
    return result


def validate_cargo(item):
    root = m.private_directory(Path(item['path']))
    m.require(Path(item['path']).is_absolute(), 'absolute Cargo input path required')
    # Only registry data, never inherited configuration, credentials or executables.
    for name in item['entries']:
        m.require(name.startswith(('registry/cache/', 'registry/src/', 'registry/index/')),
                  'unapproved Cargo input: ' + name)
    m.validate_tree(root, item['entries'], exact=True)
    return root


def provision_cargo(item, destination):
    root = validate_cargo(item)
    for name in item['entries']:
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        m.exclusive_write(target, (root / name).read_bytes())
    m.validate_tree(destination, item['entries'], exact=True)


def validate_private_cargo(item, destination):
    m.validate_tree(destination, item['entries'])
    # Cargo may update only these top-level lock/usage databases. New dependency
    # bytes (including newly extracted crates) require approval before invocation.
    extra = m.tree_files(destination) - set(item['entries'])
    # Cargo also creates this fixed cache-directory marker, not dependency bytes.
    if 'registry/CACHEDIR.TAG' in extra:
        m.require((destination / 'registry/CACHEDIR.TAG').read_bytes() ==
                  b'Signature: 8a477f597d28d172789f06886806bc55\n# This file is a cache directory tag created by cargo.\n# For information about cache directory tags see https://bford.info/cachedir/\n',
                  'unexpected Cargo cache tag bytes')
        extra.remove('registry/CACHEDIR.TAG')
    m.require(extra <= {'.package-cache', '.package-cache-mutate', '.global-cache'},
              'unexpected private Cargo input')


def validate_external(external):
    roots = external['roots']
    required = {'IDF_PATH', 'IDF_TOOLS_PATH', 'RUSTUP_HOME', 'TOOL_BIN'}
    m.require(required <= set(roots) <= required | {'PYTHON_BIN'}, 'exact external roots required')
    paths = []
    snapshots = {}
    for key, item in roots.items():
        m.require(Path(item['path']).is_absolute(), 'absolute external path required')
        root = m.private_directory(Path(item['path']))
        paths.append(root)
        snapshots[key] = installation_inventory(root)
        m.require(snapshots[key] == item['entries'], 'installation inventory drift: ' + key)
    cargo = validate_cargo(external['cargo_inputs'])
    all_paths = paths + [cargo]
    for i, path in enumerate(all_paths):
        m.require(not any(path.is_relative_to(other) or other.is_relative_to(path)
                          for other in all_paths[i + 1:]), 'overlapping external roots')
    approved = external.get('external_interpreters', {})
    for key, entries in snapshots.items():
        for name, info in entries.items():
            if 'link' not in info:
                continue
            target = Path(info['resolved'])
            if any(target.is_relative_to(root) for root in paths):
                # Every physical contained target is covered by the exact snapshots;
                # directory targets bind their complete subtree, including link text.
                continue
            m.require(key in ('PYTHON_BIN', 'IDF_TOOLS_PATH') and Path(name).name in ('python', 'python3', 'python3.12'),
                      'external link is not an approved interpreter')
            m.require(str(target) in approved and target.is_file() and os.access(target, os.X_OK),
                      'unapproved external interpreter target')
            m.require(approved[str(target)] == {'sha256': m.sha(target.read_bytes()),
                                               'bytes': target.stat().st_size},
                      'external interpreter drift')
    return roots


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--external-inputs', required=True, type=Path)
    parser.add_argument('--external-inputs-sha256', required=True,
                        help='independently approved digest; do not derive blindly at launch')
    args = parser.parse_args()
    d = m.private_directory(args.candidate)  # do not resolve away symlinks
    m.require(d.name.startswith('education-candidate-'), 'candidate namespace required')
    m.require({p.name for p in d.iterdir()} == {'source'}, 'candidate must contain only fresh source')
    source = m.private_directory(d / 'source')
    work = m.fresh_directory(d / 'run')
    receipt = work / 'build-status.json'
    now = lambda: datetime.datetime.now(datetime.timezone.utc).isoformat()
    status = {'schema': 'education-local-build-v2', 'status': 'running', 'started': now(),
              'runner_pid': os.getpid(), 'distribution': 'HOLD', 'hardware_acceptance': 'NOT TESTED',
              'provenance_limits': LIMITS, 'artifacts': {}, 'timeout_seconds': 2400}
    child = None
    code = 1
    def interrupted(signum, frame):
        raise InterruptedError('runner signal ' + str(signum))
    old_handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        atomic_status(receipt, status)
        data = m.load()
        external_bytes = m.no_links(args.external_inputs).read_bytes()
        m.require(m.sha(external_bytes) == args.external_inputs_sha256, 'external inventory digest mismatch')
        external = json.loads(external_bytes)
        roots = validate_external(external)
        for root in [*roots.values(), external['cargo_inputs']]:
            path = Path(root['path'])
            m.require(not path.is_relative_to(d) and not d.is_relative_to(path), 'external root overlaps candidate')
        for parent in source.parents:
            m.require(not os.path.lexists(parent / '.cargo'), 'ancestor Cargo configuration is forbidden')
        managed = external['managed_components']
        validate_inputs(source, data['entries'], managed)
        recipe_names = ['education-successor.py', 'build-education-successor.sh', 'run-education-build.py', 'verify-education-runtime.py']
        recipe = {n: m.sha((m.ROOT / 'scripts' / n).read_bytes()) for n in recipe_names}
        m.require(not m.git('status', '--porcelain', '--', *['scripts/' + n for n in recipe_names]), 'commit recipe before build')
        digest = m.sha(m.MANIFEST.read_bytes())
        env = build_environment(work, roots)
        provision_cargo(external['cargo_inputs'], Path(env['CARGO_HOME']))
        # No inherited Cargo config or unsupervised tool execution for remap discovery.
        pairs = []
        # The immutable checker maps rustc's sysroot, NOT its RUSTUP_HOME.
        # This exact pinned host toolchain is covered by validate_external above;
        # do not execute caller-selected rustc during admission. The post-export
        # checker independently resolves rustc and rejects any selection drift.
        sysroot = m.private_directory(Path(env['RUSTUP_HOME']) /
                                     'toolchains/nightly-2026-04-15-x86_64-unknown-linux-gnu')
        for path, dest in [(source, '/src/entropylab'), (Path(env['IDF_PATH']), '/IDF'),
                           (Path(env['CARGO_HOME']), '/deps/cargo'),
                           (sysroot, '/toolchain/rust')]:
            for spelling in {str(path.absolute()), str(path.resolve())}:
                pairs.extend([(spelling, dest), ('/' + spelling, dest)])
        pairs = sorted(set(pairs), key=lambda pair: (len(pair[0]), pair[0]))
        m.require(not any(any(c.isspace() or c in '=;\"\'\\' for c in p) for pair in pairs for p in pair), 'unsupported remap path')
        cflags = ' '.join('-ffile-prefix-map=' + a + '=' + b for a, b in pairs)
        values = {'ENTROPYLAB_C_PREFIX_FLAGS': cflags,
                  'CFLAGS_riscv32imafc_esp_espidf': '-march=rv32imafc -mabi=ilp32f -fno-pic -fno-pie ' + cflags,
                  'CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS': '-C relocation-model=static ' + ' '.join('--remap-path-prefix=' + a + '=' + b for a, b in pairs),
                  'ENTROPYLAB_PROJECT_VER': 'e' + digest[:24] + '-e1', 'ENTROPYLAB_SOURCE_SHA256': digest}
        exports = work / 'exports.sh'
        m.exclusive_write(exports, ''.join('export ' + k + '=' + shlex.quote(v) + '\n' for k, v in values.items()).encode())
        m.exclusive_write(work / 'managed-inputs.json', m.canonical(managed))
        env.update(EDUCATION_SOURCE=str(source), EDUCATION_EXPORTS=str(exports), EDUCATION_RUN=str(work))
        command = ['/bin/bash', '--noprofile', '--norc', str(m.ROOT / 'scripts/build-education-successor.sh')]
        status.update(source_revision=m.REVISION, source_manifest_sha256=digest,
                      external_inventory_sha256=args.external_inputs_sha256,
                      recipe_revision=m.git('rev-parse', 'HEAD').decode().strip(), recipe_sha256=recipe,
                      descriptor=values['ENTROPYLAB_PROJECT_VER'], environment=env,
                      exports_sha256=m.sha(exports.read_bytes()), command=command, cwd=str(d))
        m.exclusive_write(work / 'build.log', b'')
        fd = os.open(work / 'build.log', os.O_WRONLY | os.O_NOFOLLOW)
        with os.fdopen(fd, 'wb') as log:
            child = subprocess.Popen(command, cwd=d, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            status['child_pid'] = child.pid
            atomic_status(receipt, status)
            try:
                code = child.wait(timeout=2400)
            except subprocess.TimeoutExpired:
                code = 124
                status['error'] = 'build timeout'
    except BaseException as exc:
        status['error'] = type(exc).__name__ + ': ' + str(exc)
        code = 1
    finally:
        # Ignore a second termination request while stopping supervised writers/sealing failure.
        for sig in old_handlers:
            signal.signal(sig, signal.SIG_IGN)
        try:
            if child is not None:
                stop_group(child)
                status['child_exit_code'] = child.returncode
                status['process_group_quiescent'] = True
            if child is not None and code == 0:
                validate_inputs(source, data['entries'], managed, post=True)
                validate_external(external)
                validate_private_cargo(external['cargo_inputs'], Path(env['CARGO_HOME']))
                m.require(recipe == {n: m.sha((m.ROOT / 'scripts' / n).read_bytes()) for n in recipe}, 'recipe drift')
                status['post_source_check'] = 'PASS'
                effective = work / 'effective-environment.json'
                status['effective_environment'] = json.loads(m.no_links(effective).read_bytes())
                build = source / 'fixture-firmware/build'
                for name in OUTPUTS:
                    p = m.no_links(build / name)
                    status['artifacts'][name] = {'sha256': m.sha(p.read_bytes()), 'bytes': p.stat().st_size}
            if (work / 'build.log').exists():
                status['log_sha256'] = m.sha(m.no_links(work / 'build.log').read_bytes())
        except BaseException as exc:
            code = code or 1
            status['finalization_error'] = type(exc).__name__ + ': ' + str(exc)
            status['artifacts'] = {}
        status.update(exit_code=code, finished=now(), status='local-build-passed' if code == 0 else 'failed')
        try:
            atomic_status(receipt, status)
        finally:
            for sig, handler in old_handlers.items():
                signal.signal(sig, handler)
    print(json.dumps(status, indent=2))
    raise SystemExit(code)


if __name__ == '__main__':
    main()
