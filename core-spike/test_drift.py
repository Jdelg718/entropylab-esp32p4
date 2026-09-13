"""Drift regression tests use disposable copies, never alter trusted inputs."""
from pathlib import Path
import hashlib, shutil, subprocess, sys, tempfile, unittest, os
ROOT = Path(__file__).resolve().parent
UPSTREAM = Path(os.environ.get('ENTROPYLAB_UPSTREAM', ROOT/'upstream'))

class DriftTests(unittest.TestCase):
    def check_drift(self, script, relative):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)
            for f in ('extract.py','prepare_vectors.py','integrity.py'):
                if (ROOT/f).exists(): shutil.copy2(ROOT/f,p/f)
            shutil.copytree(ROOT/'vectors',p/'vectors')
            shutil.copytree(ROOT/'native/src',p/'native/src')
            if relative.startswith('upstream/'):
                if not (UPSTREAM/'.git').exists():
                    self.skipTest('optional pinned upstream checkout not configured')
                (p/'upstream/entropylab-wasm/src').mkdir(parents=True)
                (p/'upstream/.git').symlink_to(UPSTREAM/'.git',target_is_directory=True)
                shutil.copy2(UPSTREAM/'entropylab-wasm/src/lib.rs',p/'upstream/entropylab-wasm/src/lib.rs')
            target=p/relative
            target.write_bytes(target.read_bytes()+b'\n// drift\n' if relative.endswith('.rs') else target.read_bytes()+b'\n')
            before={str(f.relative_to(p)):f.read_bytes() for sub in ['vectors','native/src'] for f in (p/sub).rglob('*') if f.is_file()}
            env=dict(os.environ); env.pop('ENTROPYLAB_UPSTREAM', None)
            r=subprocess.run([sys.executable,str(p/script)],capture_output=True,text=True,env=env)
            self.assertNotEqual(r.returncode,0, r.stdout+r.stderr)
            self.assertIn('drift',r.stderr.lower())
            for f,data in before.items(): self.assertEqual((p/f).read_bytes(),data,f+' changed on rejection')
    def test_upstream_worktree_drift(self): self.check_drift('extract.py','upstream/entropylab-wasm/src/lib.rs')
    def test_fixture_drift_before_extract(self): self.check_drift('extract.py','vectors/bip39.json')
    def test_fixture_source_drift(self): self.check_drift('prepare_vectors.py','vectors/bip39.json')
    def test_generated_fixture_drift(self): self.check_drift('prepare_vectors.py','vectors/bip32.tsv')
    def test_extraction_manifest_drift(self): self.check_drift('extract.py','vectors/extraction.json')
    def test_trust_manifest_drift(self): self.check_drift('prepare_vectors.py','vectors/sha256.json')

if __name__=='__main__': unittest.main()
