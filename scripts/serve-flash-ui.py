"""Loopback-only allowlisted static preview. No dependency, directory listing or BIN serving.
Run from any cwd: python3 scripts/serve-flash-ui.py [--port 8765]
"""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import cast

ROOT = Path(__file__).resolve().parents[1] / 'site/flash'
FILES = {'/': ('index.html', 'text/html; charset=utf-8'), '/flash/': ('index.html', 'text/html; charset=utf-8')}
for name, mime in [('index.html','text/html; charset=utf-8'),('style.css','text/css; charset=utf-8'),('app.mjs','text/javascript; charset=utf-8'),('model.mjs','text/javascript; charset=utf-8'),('candidate.mjs','text/javascript; charset=utf-8')]:
    FILES['/'+name] = FILES['/flash/'+name] = (name,mime)
CSP = "default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'none'; img-src 'none'; object-src 'none'; base-uri 'none'; form-action 'none'; frame-ancestors 'none'"
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        host = self.headers.get('Host', '')
        port = cast(ThreadingHTTPServer, self.server).server_port
        if host not in (f'127.0.0.1:{port}', f'localhost:{port}'):
            self.reply(403, b'Forbidden', 'text/plain'); return
        spec = FILES.get(self.path)
        if spec is None:
            self.reply(404,b'Not found','text/plain'); return
        path=ROOT/spec[0]
        if path.is_symlink() or not path.is_file():
            self.reply(404,b'Not found','text/plain'); return
        self.reply(200,path.read_bytes(),spec[1])
    def reply(self, code, body, mime):
        self.send_response(code)
        for name,value in {'Content-Type':mime,'Content-Length':str(len(body)), 'Cache-Control':'no-store', 'Content-Security-Policy':CSP, 'X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer', 'Permissions-Policy':'serial=(), usb=(), hid=(), camera=(), microphone=(), geolocation=()', 'Cross-Origin-Resource-Policy':'same-origin'}.items():
            self.send_header(name,value)
        self.end_headers(); self.wfile.write(body)
    def log_message(self, format, *args):
        pass  # Do not record request paths or client identifiers.
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--port',type=int,default=8765)
    args=parser.parse_args()
    server=ThreadingHTTPServer(('127.0.0.1',args.port),Handler)
    print(f'Source UI preview only: http://127.0.0.1:{server.server_port}/flash/',flush=True)
    try: server.serve_forever()
    except KeyboardInterrupt: pass
    finally: server.server_close()
