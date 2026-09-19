#!/usr/bin/env python3
"""Loopback-only public preview. Never serve Git metadata/untracked files."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
from urllib.parse import unquote, urlsplit
ROOT = Path(__file__).resolve().parents[1]
ALLOWED = set(subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode().split('\0'))
class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)
    def send_head(self):
        path = unquote(urlsplit(self.path).path).lstrip('/')
        if path.endswith('/') or not path:
            path += 'index.html'
        target = ROOT / path
        if path not in ALLOWED or not target.is_file() or target.is_symlink() or not target.resolve().is_relative_to(ROOT):
            self.send_error(404)
            return None
        return super().send_head()
if __name__ == '__main__':
    print('Open http://localhost:8000/flash/first-install/', flush=True)
    ThreadingHTTPServer(('127.0.0.1', 8000), Handler).serve_forever()
