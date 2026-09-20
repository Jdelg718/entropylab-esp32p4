#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
EXPECTED_VECTOR_SHA256 = "9ab0ec42afaa71e0a4a9540ef8e7fd34704a14bd4e27017f5c3162a9e06d923c"
EXPECTED_DEADBEEF_RGB_SHA256 = "cb5c61fdbab952cd54b86824291d14e36255df58c80d25f7463db369e2d1ccf6"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def verify_vendor_manifest() -> None:
    manifest = ROOT / "VENDOR-SHA256.txt"
    for line in manifest.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        expected, relative = line.split("  ", 1)
        path = ROOT / relative
        if not path.is_file():
            fail(f"missing vendored file: {relative}")
        actual = sha256(path.read_bytes())
        if actual != expected:
            fail(f"vendored file changed: {relative}: {actual}")


def verify_upstream_vector() -> None:
    vector_path = ROOT / "vectors/upstream-test-vectors.json"
    if sha256(vector_path.read_bytes()) != EXPECTED_VECTOR_SHA256:
        fail("upstream vector file checksum mismatch")
    vectors = json.loads(vector_path.read_text(encoding="utf-8"))
    matches = [v for v in vectors if v["input"] == "deadbeef"
               and v["input_type"] == "hex" and v["version"] == "version2"
               and v["module_size"] == 1 and v["has_alpha"] is False]
    if len(matches) != 1:
        fail(f"expected one upstream deadbeef/version2/RGB vector, got {len(matches)}")
    vector = matches[0]
    expected = bytes(vector["colors"])
    if vector["width"] != 32 or vector["height"] != 32 or len(expected) != 3072:
        fail("upstream vector dimensions or byte count changed")
    if sha256(expected) != EXPECTED_DEADBEEF_RGB_SHA256:
        fail("upstream pixel hash mismatch")
    actual = (ROOT / "evidence/lifehash-deadbeef.rgb").read_bytes()
    if actual != expected:
        fail("module pixels differ byte-for-byte from upstream vector")


def verify_ppm() -> None:
    ppm = (ROOT / "evidence/lifehash-73c5da0a.ppm").read_bytes()
    header = b"P6\n32 32\n255\n"
    if not ppm.startswith(header) or len(ppm) != len(header) + 3072:
        fail("PPM structure or size invalid")
    expected = "09da10ffd57a4f58616a5eda313d3f0c861e79b93e1b609a012f9c3530b427b5"
    if sha256(ppm[len(header):]) != expected:
        fail("PPM pixels do not match public fingerprint fixture")


verify_vendor_manifest()
verify_upstream_vector()
verify_ppm()
print("PASS: vendored checksums, upstream full-pixel vector, and PPM render verified")
