#!/usr/bin/env python3
"""Exercise the actual distribution without Git, including private-file denial."""
import pathlib, subprocess, tempfile, zipfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='public-zip-') as tmp:
    dst = pathlib.Path(tmp)
    with zipfile.ZipFile(root/'release/public-release.zip') as z:
        z.extractall(dst)
    assert not (dst/'.git').exists()
    (dst/'factory-backup.bin').write_bytes(b'PRIVATE TEST SENTINEL')
    (dst/'browser-review.png').write_bytes(b'PRIVATE TEST SENTINEL')
    subprocess.run(['python3', str(dst/'scripts/test-serve-public.py')], cwd=dst, check=True)
print('PASS actual extracted ZIP, no .git, private sentinels denied')
