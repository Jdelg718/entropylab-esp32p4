"""Strict read-only schema dispatch for supplemental checks."""
import importlib.util
import json
from pathlib import Path
import re

spec = importlib.util.spec_from_file_location('identity', Path(__file__).with_name('education-successor.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
ARTIFACTS = {'entropylab_fixture.bin', 'entropylab_fixture.elf', 'entropylab_fixture.map', 'bootloader/bootloader.bin', 'partition_table/partition-table.bin', 'flasher_args.json', 'compile_commands.json', 'project_description.json'}
BASE = {'education-successor.py', 'build-education-successor.sh', 'run-education-build.py'}
SCHEMAS = {'education-local-build-v1': ('.', BASE), 'education-local-build-v2': ('run', BASE | {'verify-education-runtime.py'})}
FILES = ('build-status.json', 'build.log', 'exports.sh', 'managed-inputs.json')

def inventory(receipt):
    m.require(receipt.get('schema') in SCHEMAS, 'unsupported receipt schema')
    m.require(set(receipt['artifacts']) == ARTIFACTS, 'artifact inventory')
    m.require(set(receipt['recipe_sha256']) == SCHEMAS[receipt['schema']][1], 'recipe inventory')
    m.require(re.fullmatch('[0-9a-f]{40}', receipt['recipe_revision']) is not None, 'recipe revision')
    for name, digest in receipt['recipe_sha256'].items():
        m.require(m.sha(m.git('show', receipt['recipe_revision'] + ':scripts/' + name)) == digest, 'committed recipe binding')

def load(candidate, inspect_failed=False):
    d = Path(candidate).absolute()
    paths = [d / 'build-status.json', d / 'run/build-status.json']
    present = [p for p in paths if p.exists() or p.is_symlink()]
    m.require(len(present) == 1, 'ambiguous or missing receipt')
    path = m.no_links(present[0])
    receipt = json.loads(path.read_bytes())
    inventory(receipt)
    work = d / SCHEMAS[receipt['schema']][0]
    m.require(path == work / 'build-status.json', 'schema layout mismatch')
    other = d / 'run' if receipt['schema'].endswith('v1') else d
    m.require(not any((other / n).exists() or (other / n).is_symlink() for n in FILES), 'mixed receipt layouts')
    passed = receipt.get('status') == 'local-build-passed' and type(receipt.get('exit_code')) is int and receipt['exit_code'] == 0 and type(receipt.get('child_exit_code')) is int and receipt['child_exit_code'] == 0
    failed = receipt.get('status') == 'failed' and type(receipt.get('exit_code')) is int and receipt['exit_code'] != 0
    m.require(bool(receipt.get('finished')) and (passed or (inspect_failed and failed)), 'build not completed successfully')
    if receipt['schema'].endswith('v2'):
        m.require(receipt.get('process_group_quiescent') is True, 'process group not quiescent')
    m.require(receipt.get('post_source_check') == 'PASS', 'post source check')
    for name, key in [('build.log', 'log_sha256'), ('exports.sh', 'exports_sha256')]:
        m.require(m.sha(m.no_links(work / name).read_bytes()) == receipt[key], key)
    build = d / 'source/fixture-firmware/build'
    for name, info in receipt['artifacts'].items():
        p = m.no_links(build / name)
        m.require(p.stat().st_size == info['bytes'] and m.sha(p.read_bytes()) == info['sha256'], 'artifact binding: ' + name)
    return receipt, work, path, passed
