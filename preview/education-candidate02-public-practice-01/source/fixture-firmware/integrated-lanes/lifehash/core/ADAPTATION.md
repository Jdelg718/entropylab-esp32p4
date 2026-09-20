# Local adaptation revision-05 — NOT pristine upstream
Independent re-review REQUIRED; no integration/release approval.
Base: frozen linux-recovery/snapshot-4, upstream commit 0444dbed5615fbc9a98163608c6499c025b7873b.
Only modified upstream file: vendor/bc-lifehash/src/lifehash.cpp
Before SHA-256: 3bcb665ce4386df263e3cf35af66a7c35deb43bd58c5ccc8d3d4ed2782671219
After SHA-256: 1de25a1180078459938df3724e9ff3be4c7c084e501034c3f33e2ad4da3d24ed
Four raw grid owners replaced by std::unique_ptr via make_unique, preserving allocation order and pointer swaps. Manual deletes removed; all exceptional exits reclaim grid storage. No algorithm, wrapper ABI or pixel math change. Original source notices retained unchanged.
VENDOR-SHA256.txt describes this ADAPTED tree, not pristine upstream. ORIGINAL-VENDOR-SHA256.txt is historical baseline only.
License inventory: Blockchain Commons BSD-2-Clause-Patent applies except sha256.cpp/sha256.hpp, which carry Gifford/Rusnak three-condition BSD notices. Exact binary accompanying notices are bundled in binary-notices/. No JS copied. Patent grant is limited as written, not general third-party clearance.
Scope: bounded fingerprint C wrapper only. Other upstream C exports are not newly hardened or approved. Host-only public-fixture assurance, not MCU resource bounds or UI/device acceptance.
