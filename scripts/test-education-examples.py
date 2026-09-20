#!/usr/bin/env python3
"""Bounded public lesson checks against the actual target C algorithms."""
import ast
import ctypes as C
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CONTENT = ROOT / 'fixture-firmware/app/main/education_content.inc'


def lesson(name):
    source = CONTENT.read_text()
    block = source.split('education_' + name + '={', 1)[1].split('\n};', 1)[0]
    return ''.join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', block))


class PublicExamples(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        binary = Path(cls.temp.name) / 'examples.so'
        subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(ROOT / 'fixture-firmware/input-methods/extra-dice/extra_dice.c'),
                        str(ROOT / 'fixture-firmware/integrated-lanes/playingcards/target-adapter/cards_target.c'),
                        '-lm', '-o', str(binary)], check=True)
        cls.core = C.CDLL(str(binary))
        for name in ('el_bitbox_indices', 'el_dplus_indices'):
            fn = getattr(cls.core, name)
            fn.argtypes = [C.c_char_p, C.c_size_t, C.POINTER(C.c_uint16), C.c_size_t,
                           C.POINTER(C.c_size_t)]
            fn.restype = C.c_int
        cls.core.el_dplus_final_index.argtypes = [C.c_char_p, C.c_size_t, C.c_uint,
                                                  C.POINTER(C.c_uint8)]

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def index(self, method, transcript):
        out = (C.c_uint16 * 1)()
        count = C.c_size_t()
        self.assertEqual(getattr(self.core, 'el_' + method + '_indices')(
            transcript.encode(), len(transcript), out, 1, C.byref(count)), 0)
        self.assertEqual(count.value, 1)
        return out[0]

    def test_all_six_lessons_fit_public_practice_buffer_and_ascii(self):
        source = CONTENT.read_text()
        names = re.findall(r'static const education_document education_(\w+)=', source)
        self.assertEqual(names, ['words', 'seed', 'cards', 'bases', 'bitbox', 'dplus'])
        for name in names:
            with self.subTest(lesson=name):
                block = source.split('education_' + name + '={', 1)[1].split('\n};', 1)[0]
                fields = re.findall(r'(?:"(?:\\.|[^"\\])*"\s*)+', block)
                values = [''.join(ast.literal_eval(s) for s in re.findall(
                    r'"(?:\\.|[^"\\])*"', field)) for field in fields]
                self.assertEqual(len(values), 4)
                self.assertTrue(''.join(values).isascii())
                rendered = '#ff9900 PUBLIC PRACTICE - NEVER FUND#\n' + values[2]
                self.assertLess(len(rendered.encode()), 1024)
                self.assertIn('never receive funds', values[2].lower())

    def test_physical_fairness_explanations_do_not_credit_encoding(self):
        self.assertIn('Among accepted D6 rolls, 1-4 have equal chances if the D6 is fair', lesson('bitbox'))
        self.assertIn('Each D16 face must map to exactly one distinct key', lesson('dplus'))
        self.assertIn('Replacement alone does not make draws independent', lesson('cards'))

    def test_bitbox_physical_recipe_and_mixed_public_example(self):
        text = lesson('bitbox')
        self.assertIn('PUBLIC mixed example: 1,2,3,4,1 then 6', text)
        self.assertIn('zero-based index 217 (word number 218)', text)
        self.assertEqual(self.index('bitbox', '123416'), 217)
        self.assertEqual(self.index('bitbox', '516235416'), 217)
        self.assertIn('heads -> key 1; tails -> key 4', text)
        self.assertIn('First tap Use final #1', text)
        self.assertIn('Final - / Final + change the selected candidate immediately', text)
        self.assertIn('Never reroll an accepted face just because you dislike it', text)

    def test_dplus_physical_faces_and_mixed_public_example(self):
        text = lesson('dplus')
        self.assertIn('faces 1-16 -> keys 0-9,A-F', text)
        self.assertIn('face 1 -> 0, face 10 -> 9, face 11 -> A, face 16 -> F', text)
        self.assertIn('PUBLIC mixed example: D8 face 3, D16 faces 11 and 16', text)
        self.assertIn('3AF gives zero-based index 687 (word number 688)', text)
        self.assertEqual(self.index('dplus', '3AF'), 687)
        self.assertIn('18-word final example: D16 face 11 -> A, D8 face 6 -> 6', text)
        for face in range(1, 9):
            transcript = ('A' + str(face)).encode()
            out = C.c_uint8(255)
            self.assertEqual(self.core.el_dplus_final_index(transcript, 2, 18, C.byref(out)), 0)
            self.assertEqual(out.value, 20 + (face >= 5))

    def test_cards_replacement_recipe_and_mixed_public_example(self):
        text = lesson('cards')
        self.assertIn('one card of each enabled rank', text)
        self.assertIn('Return the drawn card before every new draw', text)
        self.assertNotIn('replacing the drawn rank', text)
        self.assertIn('PUBLIC mixed example: A,2,8,3', text)
        self.assertIn('zero-based index 62 (word number 63)', text)
        class Result(C.Structure):
            _fields_ = [(name, C.c_uint32) for name in (
                'version', 'mode', 'words', 'context', 'entropy_len',
                'mnemonic_len', 'method_len', 'flags')] + [
                ('source_bits', C.c_double), ('entropy', C.c_uint8 * 32),
                ('mnemonic', C.c_uint8 * 256), ('method', C.c_uint8 * 32)]
        convert = self.core.cards_target_convert
        convert.argtypes = [C.c_uint32, C.c_uint32, C.c_uint32, C.c_char_p,
                            C.c_size_t, C.POINTER(Result)]
        for words, final in ((12, 'AA2'), (15, 'A8'), (18, 'A4'), (21, 'A2'), (24, '8')):
            raw = ('A283' + 'AAAA' * (words - 2) + final).encode()
            result = Result()
            self.assertEqual(convert(2, words, 0, raw, len(raw), C.byref(result)), 0)
            self.assertEqual((result.entropy[0] << 3) | (result.entropy[1] >> 5), 62)
            bad = b'A288' + raw[4:]
            self.assertEqual(convert(2, words, 0, bad, len(bad), C.byref(result)), 102)


if __name__ == '__main__':
    unittest.main(verbosity=2)
