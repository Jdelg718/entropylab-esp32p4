"""Actual bundled transport in Chromium Web Streams; no hardware access."""
import http.server, threading, functools, pathlib, json
from playwright.sync_api import sync_playwright
root = pathlib.Path(__file__).resolve().parents[1]
handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(root))
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
try:
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, executable_path=__import__('os').environ.get('CHROMIUM_PATH'), args=['--no-sandbox'])
        page = browser.new_page()
        page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
        result = page.evaluate("async () => {const m=await import('./speed-browser.mjs'); return await m.run();}")
        print(json.dumps({'chromium':browser.version, 'results':result}, indent=2))
        browser.close()
finally:
    server.shutdown()
