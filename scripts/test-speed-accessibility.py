"""Keyboard + Chromium accessibility-tree gate; no physical device access."""
import tempfile
import functools, http.server, json, os, pathlib, threading
from playwright.sync_api import sync_playwright
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=pathlib.Path(os.environ['SPEED_GATE_OUTPUT']) if os.environ.get('SPEED_GATE_OUTPUT') else pathlib.Path(tempfile.mkdtemp(prefix='entropylab-keyboard-'))
OUT.mkdir(exist_ok=True)
class Quiet(http.server.SimpleHTTPRequestHandler):
 def log_message(self,format,*args): pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),functools.partial(Quiet,directory=str(ROOT)))
threading.Thread(target=server.serve_forever,daemon=True).start()
mock="""export const BOARD='public-fixture'; export function createSession(o){window.events=o;return {run:async a=>{window.runArgs=a;window.runs=(window.runs||0)+1;await new Promise((r,j)=>{window.finish=r;window.fail=j;});},checkDevice:async()=>{},cancel:async()=>{window.fail?.(Error('cancelled'));}};}"""
results=[]
try:
 with sync_playwright() as p:
  browser=p.chromium.launch(headless=True,executable_path=os.environ.get('CHROMIUM_PATH'),args=['--no-sandbox'])
  for width in [320,1280]:
   page=browser.new_page(viewport={'width':width,'height':900});errors=[];tabs=[]
   page.on('pageerror',lambda e:errors.append(str(e)))
   page.route('**/adapter/profiles.mjs',lambda route:route.fulfill(content_type='text/javascript',body=mock))
   page.add_init_script("Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>({})}})")
   page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
   def tab_to(element):
    for _ in range(15):
     page.keyboard.press('Tab')
     focused=page.evaluate('document.activeElement.id || document.activeElement.tagName')
     tabs.append(focused)
     if focused==element:return
    raise AssertionError({'unreachable':element,'tabs':tabs})
   assert page.locator('#flash').is_disabled()
   tab_to('board');page.keyboard.press('Space');assert page.locator('#board').is_checked()
   tab_to('consent');page.keyboard.press('Space');assert page.locator('#consent').is_checked()
   tab_to('prepare');page.keyboard.press('Enter');page.wait_for_function('!!window.events')
   tab_to('connect');page.keyboard.press('Enter');page.wait_for_function("!document.querySelector('#flash').disabled")
   tab_to('flash');page.keyboard.press('Enter');page.wait_for_function('!!window.finish')
   page.evaluate("events.onState({state:'changing-baud'});events.onProgress({phase:'writing',kind:'compressed-write',assetIndex:0,bytes:100,totalBytes:1000,elapsedMs:1000})")
   assert 'Compressed bytes acknowledged' in page.locator('#status').inner_text()
   cdp=page.context.new_cdp_session(page)
   nodes=cdp.send('Accessibility.getFullAXTree')['nodes']
   statuses=[n for n in nodes if n.get('role',{}).get('value')=='status']
   assert len(statuses)==1,statuses
   props={v['name']:v.get('value',{}).get('value') for v in statuses[0].get('properties',[])}
   assert props.get('live')=='polite',props
   assert props.get('atomic') is True,props
   boxes=[n for n in nodes if n.get('role',{}).get('value')=='checkbox']
   assert len(boxes)==2 and all(n.get('name',{}).get('value') for n in boxes)
   tab_to('cancel');page.keyboard.press('Enter');page.wait_for_function("document.querySelector('#status').textContent.startsWith('Cancelled')")
   final=page.locator('#status').inner_text()
   page.evaluate("events.onState({state:'complete'});events.onProgress({phase:'writing',kind:'compressed-write',assetIndex:0,bytes:1000,totalBytes:1000,elapsedMs:2000})")
   assert page.locator('#status').inner_text()==final
   assert page.evaluate('runs')==1 and page.evaluate('runArgs.resetOnSuccess') is False
   assert not errors,errors
   page.screenshot(path=str(OUT/f'keyboard-{width}.png'),full_page=True)
   results.append({'width':width,'keyboard_flow':'board/consent/verify/select/write/cancel PASS','tabs':tabs,'status_properties':props,'checkbox_names':[n['name']['value'] for n in boxes],'terminal_frozen':True,'page_errors':errors})
   page.close()
  report={'chromium':browser.version,'boundary':'Actual HTML/CSS/app with mocked session and serial picker; native accessibility tree and keyboard input. No physical screen-reader listening or hardware qualification.','results':results}
  (OUT/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));browser.close()
finally:server.shutdown()
