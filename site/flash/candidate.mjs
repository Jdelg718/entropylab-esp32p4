// Exact private manifest metadata only. No BINs or download URLs.
const deepFreeze = o => {Object.values(o).forEach(v => {if(v && typeof v === "object")deepFreeze(v)}); return Object.freeze(o)};
export const manifestSha256 = "54ee881b7e581298bd922883f6c1cdcc0defd7d1c2dd78d2951ad7412e73d06b";
export const candidate = deepFreeze({
  "app_only_bootloader_replacement_allowed": false,
  "built_before_merge": true,
  "descriptor": "s902fa6672650aef4f829aeb9-h2",
  "first_install_exercised": false,
  "payloads": [
    {
      "bytes": 21264,
      "file": "bootloader.bin",
      "offset": "0x2000",
      "sha256": "268991f79279b07be7c7ea56f671a1dbd0730b53c4b97a2e6081028496f1b2d3"
    },
    {
      "bytes": 3072,
      "file": "partition-table.bin",
      "offset": "0x8000",
      "sha256": "d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb"
    },
    {
      "bytes": 1529840,
      "file": "entropylab_fixture.bin",
      "offset": "0x10000",
      "sha256": "ed2d2461133e8b463119ad42abb041b40a8fc1d916f5f3d5f400db4bc0bfeb05"
    }
  ],
  "private_candidate": true,
  "rebuilt_from_merge": false,
  "reproducible_build_verified": false,
  "schema": "entropylab-private-first-install-v1",
  "signed": false,
  "source_commit": "b1994755d17ad6b6d2dc7a12b2893ffcf4457fa5",
  "source_manifest_sha256": "902fa6672650aef4f829aeb9f8ef21c96fb00b35317c34403a0d9b2d3418bf23",
  "source_tree": "1c8158d3ced84aa1ebe6f110bafc5fe8a13edfe3",
  "status": "NOT APPROVED FOR FLASHING",
  "target": {
    "board": "Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3",
    "chip": "ESP32-P4",
    "connector": "USB TO UART",
    "display": "480x800",
    "flash_bytes": 33554432,
    "observed_revision": "1.3",
    "revision_range": "1.x"
  }
});
