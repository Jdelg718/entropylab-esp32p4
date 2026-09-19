#!/usr/bin/env python3
"""Run accepted source, preserving historical identities; host only, no firmware."""
from pathlib import Path, PurePosixPath
import hashlib, json, os, runpy, shutil, subprocess, sys, tarfile, tempfile
R = Path(__file__).resolve().parents[1]
ARCHIVE = '82d62829596fe080f6bf02cf67c3337d88d4287a98e6b82ad3c70aa56f8e4f19'
ANCHOR = '3110081933447e5cb0e7e7d2e326a089701b2cba74e4a1341a5152f66765a7e9'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok, msg):
    if not ok: raise ValueError(msg)
def main():
    require(not sys.flags.optimize and os.environ.get('PYTHONOPTIMIZE') in (None, '', '0'), 'optimized Python forbidden')
    require(sys.argv[1:] in ([], ['--check']), 'use --check or no arguments for complete host gate')
    archive = R/'release/retention-public-source.tar.gz'
    require(sha(archive) == ARCHIVE, 'accepted archive drift')
    with tempfile.TemporaryDirectory(prefix='retention-host-') as tmp:
        work = Path(tmp)
        with tarfile.open(archive) as t:
            members = t.getmembers()
            require(len(members) < 2000 and sum(m.size for m in members) < 100000000, 'archive budget')
            for m in members:
                p = PurePosixPath(m.name)
                require(not p.is_absolute() and '..' not in p.parts and (m.isfile() or m.isdir()), 'unsafe archive entry')
            t.extractall(work, filter='data')
        package = work/'retention-public-source'
        verifier = runpy.run_path(str(package/'verify-public.py'))
        verifier['verify'](package, ANCHOR)
        source = package/'source'
        identity = json.loads((package/'provenance/target-source-identity.json').read_text())
        current = {f['path']: f['sha256'] for f in json.loads((package/'provenance/source-allowlist.json').read_text())['files']}
        # README and CI are release packaging, not target inputs. Every other
        # accepted file (including ignored sdkconfig) must match the checkout.
        packaging = {'README.md', '.github/workflows/host.yml'}
        # Finite documentation-only checkout successors; archive/source pins stay exact.
        successor_file = R/'docs/retention-host/documentation-successors.json'
        require(sha(successor_file) == 'd05977f93afbbf2d65d68e0303d99af87b1c8b7c2ac1b5dc2212c427aed1d77b', 'documentation mapping drift')
        successors = json.loads(successor_file.read_text())
        require(set(successors) <= set(current), 'unknown documentation successor')
        for n, pins in successors.items():
            require(n.endswith('.md') and not n.startswith('fixture-firmware/'), 'non-documentation successor')
            require(pins['old_sha256'] == current[n], 'documentation predecessor drift: '+n)
            require(sha(source/n) == pins['old_sha256'], 'archived documentation drift: '+n)
        for n, h in current.items():
            if n in packaging: continue
            p = R/n
            require(not any(x.is_symlink() for x in [p, *p.parents]), 'checkout symlink')
            require(p.is_file() and sha(p) == successors.get(n, {}).get('new_sha256', h), 'checkout source drift: '+n)
        proof = R/'docs/retention-host/historical.json'
        require(sha(proof) == 'd6c299b13ac854425abafc5e77cdc85ab53b5832b2ebe53fe88ef696dfea083e', 'historical mapping drift')
        mapping = json.loads(proof.read_text())
        historical = work/'historical'
        for n, h in mapping.items():
            src = source/n
            if identity['source_paths'].get(n) != h:
                src = R/'docs/retention-host/inputs'/n
            require(sha(src) == h, 'historical input drift: '+n)
            dst = historical/n; dst.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(src, dst)
        if sys.argv[1:] == ['--check']:
            print('PASS accepted archive745, identity437, checkout source/config with finite documentation successors, historical436; no host execution')
            return
        env = {k: os.environ[k] for k in ['HOME','PATH','LANG','LC_ALL','TZ','CARGO','CARGO_HOME','RUSTUP_HOME','LVGL_SOURCE_DIR','BUILD_JOBS','CARGO_BUILD_JOBS','CARGO_NET_OFFLINE'] if k in os.environ}
        env.update(PYTHONDONTWRITEBYTECODE='1', PYTHONOPTIMIZE='0', HOST_TOOLCHAIN='1.95.0', CARGO_INCREMENTAL='0')
        # Disable capture persistence only; all assertions and rendering run.
        shim = work/'no-captures.so'
        subprocess.run(['cc','-shared','-fPIC',str(R/'scripts/retention-no-captures.c'),'-ldl','-o',str(shim)], check=True)
        adapters = work/'adapters'; adapters.mkdir()
        names = ['test-host.sh','test-coin-host.sh','test-dice-host.sh','test-words-host.sh','test-gui-host.sh','test-combined-host.sh']
        history_tests = ['test-d6-release-import.py','test-review-repair-import.py','test-modal-import.py','test-global-saver-import.py','verify-dice-import.py','test-dice-import.py']
        import shlex
        for name in names:
            text = (source/'scripts'/name).read_text()
            lines = text.splitlines()
            require(sum(x.startswith('ROOT=') for x in lines) == 1, 'unexpected ROOT adapter')
            text = '\n'.join('ROOT='+shlex.quote(str(source)) if x.startswith('ROOT=') else x for x in lines)+'\n'
            for test in history_tests:
                text = text.replace('"$ROOT/scripts/'+test+'"', shlex.quote(str(historical/'scripts'/test)))
            for nested in names:
                text = text.replace('"$ROOT/scripts/'+nested+'"', shlex.quote(str(adapters/nested)))
            if name == 'test-gui-host.sh':
                text = text.replace('"$BUILD/gui_host"', 'env LD_PRELOAD='+shlex.quote(str(shim))+' "$BUILD/gui_host"')
            (adapters/name).write_text(text)
        subprocess.run(['bash', str(adapters/'test-host.sh')], env=env, check=True)
        subprocess.run([str(source/'fixture-firmware/build/gui-host/gui_host')], cwd=work, env={**env,'DICE_NAV_ONLY':'1','LD_PRELOAD':str(shim)}, check=True)
        for n,h in current.items(): require(sha(source/n) == h, 'post-test accepted source drift: '+n)
        print('PASS complete original host chain + focused retention; source745/identity437 unchanged; capture persistence NOT TESTED; firmware NOT BUILT')
if __name__ == '__main__': main()
