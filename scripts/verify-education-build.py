#!/usr/bin/env python3
"""Read back candidate outputs; never promotes them to accepted release status."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

spec = importlib.util.spec_from_file_location('identity', Path(__file__).with_name('education-successor.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def image(data):
    m.require(len(data) >= 24 and data[0] == 0xe9, 'image header')
    m.require(0 < data[1] <= 16, 'segment count')
    m.require(struct.unpack_from('<H', data, 12)[0] == 18, 'ESP32-P4 chip id')
    m.require(struct.unpack_from('<HH', data, 15) == (100, 199), 'revision interval')
    pos = 24
    checksum = 0xef
    for _ in range(data[1]):
        m.require(pos + 8 <= len(data), 'segment header overflow')
        size = struct.unpack_from('<I', data, pos + 4)[0]
        pos += 8
        m.require(pos + size <= len(data), 'segment overflow')
        for byte in data[pos:pos + size]:
            checksum ^= byte
        pos += size
    end = pos + (15 - pos % 16)
    m.require(end < len(data) and data[end] == checksum, 'image checksum')
    m.require(data[23] == 1 and len(data) == end + 33, 'appended image digest required')
    m.require(hashlib.sha256(data[:end + 1]).digest() == data[end + 1:], 'image digest')
    return {'chip_id': 18, 'revision': [100, 199], 'segments': data[1],
            'flash_mode_byte': data[2], 'flash_size_frequency_byte': data[3],
            'checksum_and_digest': 'PASS'}


def receipt_inventory_check(receipt):
    m.require(set(receipt['artifacts']) == {
        'entropylab_fixture.bin', 'entropylab_fixture.elf', 'entropylab_fixture.map',
        'bootloader/bootloader.bin', 'partition_table/partition-table.bin',
        'flasher_args.json', 'compile_commands.json', 'project_description.json'}, 'artifact inventory')
    m.require(set(receipt['recipe_sha256']) == {
        'education-successor.py', 'build-education-successor.sh', 'run-education-build.py'}, 'recipe inventory')


def config_check(config):
    expected = {'IDF_TARGET': 'esp32p4', 'IDF_TARGET_ARCH': 'riscv',
                'ESP32P4_REV_MIN_FULL': 100, 'ESP32P4_REV_MAX_FULL': 199,
                'ESPTOOLPY_FLASHMODE': 'dio', 'ESPTOOLPY_FLASHSIZE': '32MB',
                'ESPTOOLPY_FLASHFREQ': '80m', 'PARTITION_TABLE_OFFSET': 32768,
                'SPIRAM': True, 'SPIRAM_MODE_HEX': True, 'SPIRAM_SPEED': 200}
    for key, value in expected.items():
        m.require(config.get(key) == value, 'generated config: ' + key)
    return expected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--inspect-failed', action='store_true', help='inspect completed failed outputs; still exits 2, never qualifies the build')
    args = parser.parse_args()
    d = args.candidate.resolve()
    receipt = json.loads((d / 'build-status.json').read_text())
    receipt_inventory_check(receipt)
    manifest = m.load()
    passed = receipt['status'] == 'local-build-passed' and receipt['child_exit_code'] == 0 and receipt['exit_code'] == 0
    failed = receipt['status'] == 'failed' and receipt['exit_code'] != 0 and 'finished' in receipt
    m.require(passed or (args.inspect_failed and failed), 'build did not pass')
    m.require(receipt['post_source_check'] == 'PASS', 'post-build source check')
    m.require(receipt['source_revision'] == m.REVISION and receipt['source_manifest_sha256'] == m.sha(m.MANIFEST.read_bytes()), 'source binding')
    m.require(receipt['log_sha256'] == m.sha((d / 'build.log').read_bytes()), 'stream binding')
    m.require(receipt['exports_sha256'] == m.sha((d / 'exports.sh').read_bytes()), 'export binding')
    for name, digest in receipt['recipe_sha256'].items():
        m.require('/' not in name and name.endswith(('.py', '.sh')), 'recipe name')
        m.require(m.sha(m.git('show', receipt['recipe_revision'] + ':scripts/' + name)) == digest, 'committed recipe binding')
    source = d / 'source'
    m.validate_tree(source, manifest['entries'])
    managed = source / 'fixture-firmware/app/managed_components'
    inputs = d / 'managed-inputs.json'
    m.require(m.sha(inputs.read_bytes()) == receipt['managed_input_inventory_sha256'], 'managed inventory binding')
    m.require(json.loads(inputs.read_text()) == {
        p.relative_to(managed).as_posix(): m.sha(p.read_bytes())
        for p in managed.rglob('*') if p.is_file()}, 'managed source drift')
    build = source / 'fixture-firmware/build'
    config = config_check(json.loads((build / 'config/sdkconfig.json').read_text()))
    for name, info in receipt['artifacts'].items():
        m.require(not Path(name).is_absolute() and '..' not in Path(name).parts, 'artifact path')
        p = build / name
        m.require(not any(q.is_symlink() for q in [p, *p.parents]) and p.stat().st_size == info['bytes'] and m.sha(p.read_bytes()) == info['sha256'], 'artifact binding: ' + name)
    app = (build / 'entropylab_fixture.bin').read_bytes()
    boot = (build / 'bootloader/bootloader.bin').read_bytes()
    images = {'app': image(app), 'bootloader': image(boot)}
    m.require(struct.unpack_from('<I', app, 32)[0] == 0xabcd5432, 'app descriptor magic')
    descriptor = app[48:80].split(b'\0')[0].decode('ascii')
    expected = 'e' + m.sha(m.MANIFEST.read_bytes())[:24] + '-e1'
    m.require(descriptor == receipt['descriptor'] == expected, 'descriptor source binding')
    m.require(app[176:208].hex() == m.sha((build / 'entropylab_fixture.elf').read_bytes()), 'descriptor ELF hash')
    for text in [b'Replacement restores equal rank chances', b'Never reroll an accepted face', b'D16 faces 11 and 16']:
        m.require(text in app, 'new lesson missing from image')
    privacy = {name: [p.decode() for p in [b'/opt/data/', b'/home/', b'/Users/', b'/root/', b'/tmp/'] if p in raw]
               for name, raw in [('app', app), ('bootloader', boot)]}
    m.require(not any(privacy.values()), 'local path found in runnable image')
    table = (build / 'partition_table/partition-table.bin').read_bytes()
    partitions = []
    md5_found = False
    for pos in range(0, len(table), 32):
        record = table[pos:pos + 32]
        if record == b'\xff' * 32:
            break
        magic = struct.unpack_from('<H', record)[0]
        if magic == 0xebeb:
            m.require(record[16:32] == hashlib.md5(table[:pos]).digest(), 'partition MD5')
            md5_found = True
            break
        m.require(magic == 0x50aa, 'partition magic')
        _, kind, subtype, offset, size, label, flags = struct.unpack('<HBBII16sI', record)
        partitions.append({'type': kind, 'subtype': subtype, 'offset': offset, 'size': size, 'label': label.split(b'\0')[0].decode(), 'flags': flags})
    m.require(md5_found, 'partition digest missing')
    ordered = sorted(partitions, key=lambda p: p['offset'])
    m.require(all(a['offset'] + a['size'] <= b['offset'] for a, b in zip(ordered, ordered[1:])), 'overlapping partitions')
    factory = [p for p in partitions if p['type'] == 0 and p['subtype'] == 0]
    m.require(len(factory) == 1 and len(app) <= factory[0]['size'], 'app partition fit')
    flash = json.loads((build / 'flasher_args.json').read_text())
    offsets = {name: int(offset, 0) for offset, name in flash['flash_files'].items()}
    m.require(offsets['entropylab_fixture.bin'] == factory[0]['offset'], 'flash app offset')
    m.require(offsets['bootloader/bootloader.bin'] + len(boot) <= offsets['partition_table/partition-table.bin'], 'bootloader fit')
    report = {'schema': 'education-generated-image-check-v1', 'status': 'PASS-local-only' if passed else 'INSPECTED-build-failed',
              'image_checks': 'PASS', 'build_exit_code': receipt['exit_code'],
              'generated_config': config,
              'generated_config_sha256': {name: m.sha((build / name).read_bytes()) for name in
                                         ['config/sdkconfig.json', 'config/sdkconfig.h', 'bootloader/config/sdkconfig.json']},
              'artifacts': receipt['artifacts'],
              'build_receipt_sha256': m.sha((d / 'build-status.json').read_bytes()),
              'source_manifest_sha256': receipt['source_manifest_sha256'], 'descriptor': descriptor,
              'images': images, 'partitions': partitions, 'flash_offsets': offsets,
              'new_lessons_in_binary': 'PASS', 'known_local_path_scan': privacy,
              'hardware': 'NOT TESTED', 'reproducibility': 'NOT TESTED', 'distribution': 'HOLD'}
    print(json.dumps(report, indent=2))
    return 0 if passed else 2


if __name__ == '__main__':
    raise SystemExit(main())
