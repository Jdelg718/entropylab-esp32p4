#!/usr/bin/env python3
"""Additive education candidate identity; never changes accepted release pins.

This records local build evidence, not reproducibility, publication or hardware approval.
"""
import argparse
import hashlib
import json
import os
import stat
from pathlib import Path
import subprocess
import tarfile

REVISION = '2b919dc73c9cbcbb5e845850650d71c6782ad68b'
ARCHIVE_SHA = '82d62829596fe080f6bf02cf67c3337d88d4287a98e6b82ad3c70aa56f8e4f19'
CHANGED = 'fixture-firmware/app/main/education_content.inc'
ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'docs/education-successor/source.json'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])


def expected():
    # Immutable Git objects, not a caller-supplied list or mutable checkout pins.
    paths = git('ls-tree', '-r', '--name-only', '-z', REVISION).decode().rstrip('\0').split('\0')
    entries = {}
    for name in paths:
        raw = git('show', REVISION + ':' + name)
        entries[name] = {'sha256': sha(raw), 'bytes': len(raw)}
    archive = git('show', REVISION + ':release/retention-public-source.tar.gz')
    require(sha(archive) == ARCHIVE_SHA, 'accepted archive drift')
    import io
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        base = 'retention-public-source/'
        allow = json.load(tar.extractfile(base + 'provenance/source-allowlist.json'))['files']
        production = {e['path']: e for e in allow if e['path'].startswith('fixture-firmware/')}
        require(set(production) == {p for p in entries if p.startswith('fixture-firmware/')}, 'firmware inventory drift')
        changes = []
        for name, entry in production.items():
            raw = tar.extractfile(base + 'source/' + name).read()
            require(sha(raw) == entry['sha256'], 'accepted source member drift')
            if entries[name]['sha256'] != entry['sha256']:
                changes.append({'path': name, 'before_sha256': entry['sha256'],
                                'after_sha256': entries[name]['sha256']})
    require([e['path'] for e in changes] == [CHANGED], 'non-education firmware delta')
    return {'schema': 'education-successor-source-v1', 'source_revision': REVISION,
            'source_tree': git('rev-parse', REVISION + '^{tree}').decode().strip(),
            'accepted_archive_sha256': ARCHIVE_SHA, 'firmware_changes': changes,
            'status': 'candidate-only; no inherited hardware acceptance; distribution HOLD',
            'entries': entries}


def validate_manifest(data, trusted):
    require(data == trusted, 'successor manifest does not match immutable source/accepted predecessor')


def no_links(path):
    path = Path(os.path.abspath(path))
    require(not any(p.is_symlink() for p in [path, *path.parents]), 'symlink path: ' + str(path))
    return path


def private_directory(path):
    path = no_links(path)
    st = path.stat()
    require(stat.S_ISDIR(st.st_mode) and st.st_uid == os.getuid() and not st.st_mode & 0o022,
            'directory must be owned by runner and not group/world writable: ' + str(path))
    return path


def fresh_directory(path):
    path = no_links(path)
    private_directory(path.parent)
    path.mkdir(mode=0o700)  # lexists, including dangling links, fails closed
    return path


def exclusive_write(path, data):
    path = no_links(path)
    private_directory(path.parent)
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, 'wb') as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())


def tree_files(root):
    root = no_links(root)
    require(root.is_dir(), 'missing input directory: ' + str(root))
    files = set()
    for parent, dirs, names in os.walk(root, followlinks=False):
        for name in dirs + names:
            p = Path(parent) / name
            mode = p.lstat().st_mode
            require(stat.S_ISDIR(mode) or stat.S_ISREG(mode), 'non-regular input: ' + str(p))
            if stat.S_ISREG(mode):
                files.add(p.relative_to(root).as_posix())
    return files


def validate_tree(root, entries, exact=False):
    for name, info in entries.items():
        require(not Path(name).is_absolute() and '..' not in Path(name).parts, 'unsafe inventory path')
        p = no_links(root / name)
        require(p.is_file() and p.stat().st_size == info['bytes'] and sha(p.read_bytes()) == info['sha256'], 'source drift: ' + name)
    if exact:
        require(tree_files(root) == set(entries), 'unexpected source inventory')


def load():
    def unique(pairs):
        obj = {}
        for k, v in pairs:
            require(k not in obj, 'duplicate JSON key')
            obj[k] = v
        return obj
    data = json.loads(MANIFEST.read_text(), object_pairs_hook=unique)
    validate_manifest(data, expected())
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['freeze', 'check', 'stage'])
    parser.add_argument('--destination', type=Path)
    args = parser.parse_args()
    if args.action == 'freeze':
        require(not MANIFEST.exists(), 'refusing to overwrite a source identity')
        MANIFEST.parent.mkdir(parents=True, exist_ok=True)
        MANIFEST.write_bytes(canonical(expected()))
    else:
        data = load()
        validate_tree(ROOT, data['entries'])
        if args.action == 'stage':
            dest = args.destination
            require(dest is not None and dest.name == 'source' and dest.parent.name.startswith('education-candidate-'), 'use new education-candidate-*/source namespace')
            # Validate ancestors BEFORE creating or populating anything.
            no_links(dest)
            fresh_directory(dest.parent)
            fresh_directory(dest)
            # Only regular files from the verified immutable revision; no stale build output.
            for name in data['entries']:
                p = dest / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(git('show', REVISION + ':' + name))
            validate_tree(dest, data['entries'], exact=True)
    print('PASS successor source identity; no target/hardware qualification')


if __name__ == '__main__':
    main()
