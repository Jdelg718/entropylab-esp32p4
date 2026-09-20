# Public fingerprint vectors

All inputs below are exactly four raw bytes. Expected values are SHA-256 of the 3,072 RGB888 output bytes from LifeHash version2, module size 1, alpha false, using `make_from_data` semantics.

| Raw bytes | Expected RGB SHA-256 | Origin |
|---|---|---|
| `73 c5 da 0a` | `09da10ffd57a4f58616a5eda313d3f0c861e79b93e1b609a012f9c3530b427b5` | EntropyLab public integration fixture |
| `00 00 00 00` | `9003d9fd366ec3aa06f54d6797485114ec00c61bf85c0efafa91bd2e40176d5b` | EntropyLab public integration fixture |
| `ff ff ff ff` | `e856f1b33dfd8eef83151de7407c3d4861581ce09f11f11f2dfc6b0219a1e51b` | EntropyLab public integration fixture |
| `b8 68 8d f1` | `d44ba038c1389003c955a6f17accfb87c98fce4e8c98c9e2a44c71067b6521fe` | EntropyLab public integration fixture |
| `de ad be ef` | `cb5c61fdbab952cd54b86824291d14e36255df58c80d25f7463db369e2d1ccf6` | bc-lifehash upstream raw-byte golden vector |

Origins:
- https://github.com/OogaBoogaX/entropylab/blob/7251f42cd67cb582b070c32863a0afba4b67f606/test/lifehash.test.mjs
- https://github.com/BlockchainCommons/bc-lifehash/blob/0444dbed5615fbc9a98163608c6499c025b7873b/test/test-vectors.json

These visual hashes are regression fixtures, not secret material and not proof of identity, security, correctness, or entropy quality.
