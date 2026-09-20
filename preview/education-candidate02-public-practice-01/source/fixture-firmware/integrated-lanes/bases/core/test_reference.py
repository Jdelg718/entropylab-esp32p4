#!/usr/bin/env python3
import unittest
import number_bases_ref as nb

WORDS = (12, 15, 18, 21, 24)
BASES = (4, 8, 32, 64)

class NumberBases(unittest.TestCase):
    def test_all_vectors(self):
        for words in WORDS:
            raw = bytes(range(nb.entropy_bytes(words)))
            for base in BASES:
                text = nb.encode(raw, base, words)
                self.assertEqual(nb.decode(text, base, words), raw)
                self.assertEqual(nb.decode(nb.encode(bytes(len(raw)), base, words), base, words), bytes(len(raw)))
                self.assertEqual(nb.decode(nb.encode(bytes([255])*len(raw), base, words), base, words), bytes([255])*len(raw))
                self.assertEqual(nb.decode(" \t\n".join(text), base, words), raw)

    def test_lengths_and_known(self):
        expected={12:(64,43,26,23),15:(80,54,32,30),18:(96,64,39,32),21:(112,75,45,39),24:(128,86,52,46)}
        for w,row in expected.items():
            self.assertEqual(tuple(nb.digit_count(b,w) for b in BASES),row)
        raw=bytes(range(16))
        self.assertTrue(nb.encode(raw,32,12).startswith("qqqsyqcy"))
        self.assertEqual(nb.encode(raw,64,12)[-2:], "11")

    def test_validation(self):
        good=nb.encode(bytes(16),8,12)
        for bad in ("", good[:-1], good+"0", good[:-1]+"4", good[:-1]+"=", "x"+good[1:]):
            with self.assertRaises(nb.NumberBaseError): nb.decode(bad,8,12)
        b32=nb.encode(bytes(16),32,12)
        self.assertEqual(nb.decode(b32.upper(),32,12),bytes(16))
        with self.assertRaises(nb.NumberBaseError): nb.decode("Q"*25+"G",32,12)
        b64=nb.encode(bytes(16),64,12)
        with self.assertRaises(nb.NumberBaseError): nb.decode(b64+"=",64,12)
        self.assertNotEqual(nb.decode(b64.swapcase(),64,12), bytes(16))
        for base, words in ((3,12),(4,13)):
            with self.assertRaises(nb.NumberBaseError): nb.digit_count(base,words)

if __name__ == "__main__": unittest.main()
