#!/usr/bin/env python3
"""Real GNU ELF32 fixtures; no target compiler or existing build required."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).with_name('localize-coin.py')

class ArchiveChecks(unittest.TestCase):
    def test_mutations(self):
        tools = {t: shutil.which(os.environ.get('COIN_TEST_' + t.upper(), t))
                 for t in ('as', 'ar', 'nm', 'objcopy', 'ld')}
        if not all(tools.values()):
            self.skipTest('GNU ELF32 fixture tools missing; set COIN_TEST_AS/AR/NM/OBJCOPY/LD')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            firmware = root / 'fixture-firmware'
            coin = firmware / 'coin/target/riscv32imafc-esp-espidf/release'
            # No historical HEX target exists in this isolated fixture.
            hexd = firmware / 'dice/target/riscv32imafc-esp-espidf/release'
            coin.mkdir(parents=True); hexd.mkdir(parents=True)
            (root / 'scripts').mkdir()
            shutil.copyfile(SCRIPT, root / 'scripts/localize-coin.py')
            bindir = root / 'bin'; bindir.mkdir()
            for name in ('nm', 'ar'):
                (bindir / ('riscv32-esp-elf-' + name)).symlink_to(tools[name])
            def wrapper(name, code):
                p = bindir / ('riscv32-esp-elf-' + name)
                p.write_text('#!' + sys.executable + '\n' + code)
                p.chmod(0o755)
            wrapper('ld', 'import subprocess,sys\nsys.exit(subprocess.call(' + repr([tools['ld'], '-m', 'elf_i386']) + '+sys.argv[1:]))\n')
            wrapper('objcopy', '''import os, subprocess, sys
from pathlib import Path
subprocess.run([os.environ['REAL_OBJCOPY']] + sys.argv[1:], check=True)
out = sys.argv[-1]
mode = os.environ['MUTATION']
if mode == 'payload':
    subprocess.run([os.environ['REAL_OBJCOPY'], '--update-section', '.text=' + os.environ['PAYLOAD'], out], check=True)
elif mode == 'addition':
    subprocess.run([os.environ['REAL_OBJCOPY'], '--add-symbol', 'surprise=.text:0,global', out], check=True)
elif mode == 'removal':
    subprocess.run([os.environ['REAL_OBJCOPY'], '--localize-symbol=el_coin_to_hex', out], check=True)
elif mode in ('missing', 'extra', 'reordered'):
    objects = {'missing':['first.o'], 'extra':['first.o','second.o','third.o'], 'reordered':['second.o','first.o']}[mode]
    Path(out).unlink()
    subprocess.run([os.environ['REAL_AR'], 'rcs', out] + objects, check=True)
    subprocess.run([os.environ['REAL_OBJCOPY'], '--localize-symbol=rust_begin_unwind', out], check=True)
elif mode == 'malformed':
    Path(out).write_bytes(b'not an archive')
''')
            def run(*args):
                subprocess.run(args, cwd=root, check=True, capture_output=True)
            for name, asm in {
                'first': '.globl rust_begin_unwind, el_coin_to_hex\nrust_begin_unwind:\nel_coin_to_hex:\n.byte 0x90\n',
                'second': '.byte 0x91\n', 'third': '.byte 0x92\n',
                'hex': '.globl rust_begin_unwind, el_hex_run\nrust_begin_unwind:\nel_hex_run:\n.byte 0x90\n',
            }.items():
                (root / (name + '.s')).write_text('.text\n' + asm)
                run(tools['as'], '--32', '-o', name + '.o', name + '.s')
            src = coin / 'libentropylab_coin_core.a'
            run(tools['ar'], 'rcs', str(src), 'first.o', 'second.o')
            run(tools['ar'], 'rcs', str(hexd / 'libentropylab_dice_core.a'), 'hex.o')
            (root / 'payload').write_bytes(b'\xcc')
            env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ['PATH'],
                       REAL_OBJCOPY=tools['objcopy'], REAL_AR=tools['ar'], PAYLOAD=str(root / 'payload'))
            self.assertFalse((firmware / 'rust/target').exists())
            # Omission and a nonexistent explicit runtime must fail closed.
            for args in ([], ['--runtime-archive', str(root / 'missing-runtime.a')]):
                result = subprocess.run([sys.executable, str(root / 'scripts/localize-coin.py')] + args,
                                        cwd=root, env=dict(env, MUTATION='unchanged'), capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('PASS', result.stdout)
            for mode in ('unchanged', 'payload', 'addition', 'removal', 'missing', 'extra', 'reordered', 'malformed'):
                with self.subTest(mode=mode):
                    result = subprocess.run([sys.executable, str(root / 'scripts/localize-coin.py'), '--runtime-archive', str(hexd / 'libentropylab_dice_core.a')],
                                            cwd=root, env=dict(env, MUTATION=mode), capture_output=True, text=True)
                    if mode == 'unchanged':
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertIn('PASS single adapter panic localization', result.stdout)
                    else:
                        self.assertNotEqual(result.returncode, 0, mode)
                        self.assertNotIn('PASS', result.stdout)
                        self.assertNotIn('Traceback', result.stderr)
                        if mode in ('payload', 'missing', 'extra', 'reordered'):
                            self.assertIn('allocated member payload changed', result.stderr)

if __name__ == '__main__':
    unittest.main()
