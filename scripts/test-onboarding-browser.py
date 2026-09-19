#!/usr/bin/env python3
"""Local Chromium tests of served app; mocked transport, never hardware."""
import runpy,threading
from http.server import ThreadingHTTPServer
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
s=runpy.run_path(str(root/'scripts/serve-public.py'))
server=ThreadingHTTPServer(('127.0.0.1',0),s['Handler'])
threading.Thread(target=server.serve_forever,daemon=True).start()
try:
 with sync_playwright() as p:
  browser=p.chromium.launch(headless=True,args=['--no-sandbox'])
  for outcome in ['success','failure','diagnostic']:
   page=browser.new_page()
   page.add_init_script("Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>({})}});window.calls=[];")
   page.route('**/first-install/assets.mjs',lambda route:route.fulfill(content_type='text/javascript',body='export async function downloadImages(){return {}}'))
   body="export const BOARD={};export function createSession(){return {run:async a=>{window.calls.push(a);"+('throw Error("readback")' if outcome=='failure' else '')+"},checkDevice:async a=>{window.diag=a},cancel:async()=>{}}}"
   page.route('**/first-install/adapter/profiles.mjs',lambda route:route.fulfill(content_type='text/javascript',body=body))
   page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
   page.locator('#prepare').click()
   assert page.locator('#connect').is_disabled()
   page.locator('#board').check();page.locator('#connect').click()
   assert page.locator('#flash').is_disabled()
   assert page.locator('#check').is_enabled()
   if outcome!='diagnostic':page.locator('#consent').check()
   page.locator('#check' if outcome=='diagnostic' else '#flash').click()
   page.wait_for_function("document.getElementById('status').textContent.includes('complete') || document.getElementById('status').textContent.includes('failed')")
   text=page.locator('#status').inner_text()
   if outcome=='success':
    assert 'Press the board RESET' in text and 'do not repeat installation just to boot' in text and 'fresh attempt' not in text
   else:
    assert 'Press the board RESET' not in text and 'fresh attempt' in text
    assert ('Diagnostic complete' if outcome=='diagnostic' else 'refused or failed') in text
   assert page.locator('#flash').is_disabled()
   assert page.evaluate('window.calls.every(a=>a.resetOnSuccess===false)')
   assert page.evaluate('window.calls.length')==(0 if outcome=='diagnostic' else 1)
   print('PASS local Chromium served app:',outcome,'consent gates and terminal wording; simulated transport')
   page.close()
  browser.close()
finally:
 server.shutdown();server.server_close()
