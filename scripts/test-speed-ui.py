"""Real app/HTML/CSS in Chromium; mocked session boundary, no WebSerial IO."""
import functools, http.server, json, os, pathlib, threading
from playwright.sync_api import sync_playwright
root = pathlib.Path(__file__).resolve().parents[1]
class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args): pass
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Quiet, directory=str(root)))
threading.Thread(target=server.serve_forever, daemon=True).start()
mock = '''export const BOARD='public-fixture';
export function createSession(o){window.events=o;return {run:async a=>{if(a.resetOnSuccess!==false)throw Error('reset');window.runs=(window.runs||0)+1;await new Promise((resolve,reject)=>{window.finish=resolve;window.fail=reject;});},checkDevice:async()=>{},cancel:async()=>{window.fail?.(Error('cancelled'));}};}'''
results=[]
try:
 with sync_playwright() as p:
  browser=p.chromium.launch(headless=True, executable_path=os.environ.get('CHROMIUM_PATH'),args=['--no-sandbox'])
  for width,zoom in [(320,1),(390,1),(1280,1),(1280,2)]:
   page=browser.new_page(viewport={'width':width,'height':900})
   errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
   page.route('**/adapter/profiles.mjs',lambda route:route.fulfill(content_type='text/javascript',body=mock))
   page.add_init_script("Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>({})}})")
   page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
   if zoom==2: page.evaluate("document.documentElement.style.zoom='2'")
   page.locator('#board').check();page.locator('#consent').check();page.locator('#prepare').click()
   page.wait_for_function('!!window.events');page.locator('#connect').click();page.locator('#flash').click();page.wait_for_function('!!window.finish')
   page.evaluate('''() => {
    const check=(b,m)=>{if(!b)throw Error(m)}, text=()=>document.querySelector('#status').textContent;
    const emit=(bytes,elapsedMs,extra={})=>events.onProgress({phase:'writing',kind:'compressed-write',assetIndex:0,bytes,totalBytes:1000,elapsedMs,phaseElapsedMs:elapsedMs,...extra});
    for(const [state,label] of Object.entries({'verifying-assets':'downloaded','connecting-rom':'115200','security-preflight':'security','official-stub':'RAM stub','changing-baud':'460800','jedec-preflight':'capacity','writing':'not yet verified','verifying-readback':'SHA-256'})){events.onState({state});check(text().includes(label),state);}
    emit(0,0);let before=text();emit(100,100);check(text().includes('acknowledged: 100 / 1000'),'immediate bytes');emit(200,200);check(text().includes('acknowledged: 200 / 1000'),'immediate bytes');emit(300,300);check(text().includes('acknowledged: 300 / 1000'),'continuous bytes');
    before=text();for(const bad of [null,{}, {kind:'compressed-write',bytes:NaN}])events.onProgress(bad);check(text()===before,'malformed');
    emit(400,600,{totalBytes:null});check(text().endsWith('acknowledged: 400'),'unknown total');
    emit(500,900,{phase:'verifying-readback',kind:'image-read',assetIndex:3});check(text().includes('erased tail')&&text().includes('SHA-256 pending'),'read');
    emit(1000,901,{phase:'verifying-readback',kind:'image-verified',assetIndex:3});check(text().includes('SHA-256 verified bytes'),'verified');check(!text().includes('%'),'fake percent');
    window.uiEmit=emit;
   }''')
   layout=page.evaluate('''() => ({width:innerWidth,zoom:getComputedStyle(document.documentElement).zoom,scroll:document.documentElement.scrollWidth,client:document.documentElement.clientWidth,statusRole:document.querySelector('#status').getAttribute('role')})''')
   assert layout['scroll']<=layout['client'],layout
   assert layout['statusRole']=='status'
   if os.environ.get('SPEED_UI_SCREENSHOTS'):
    out=pathlib.Path(os.environ['SPEED_UI_SCREENSHOTS']);out.mkdir(parents=True,exist_ok=True)
    page.screenshot(path=str(out/f'{width}-{zoom}x.png'),full_page=True)
   page.evaluate("events.onState({state:'complete'});finish();")
   page.wait_for_function("document.querySelector('#status').textContent.includes('Press the board RESET')")
   final=page.locator('#status').inner_text();page.evaluate('uiEmit(999,9999)');assert page.locator('#status').inner_text()==final
   assert page.locator('#flash').is_disabled();assert page.evaluate('runs')==1
   assert not errors,errors
   results.append({'width':width,'zoom':zoom,'layout':layout,'progress':'PASS','terminal':'success PASS','pageErrors':errors})
   page.close()
  for outcome in ['failure','diagnostic','cancel','cleanup-incomplete']:
   page=browser.new_page();page.route('**/adapter/profiles.mjs',lambda route:route.fulfill(content_type='text/javascript',body=mock))
   page.add_init_script("Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>({})}})")
   page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/');page.locator('#board').check();page.locator('#consent').check();page.locator('#prepare').click();page.wait_for_function('!!window.events');page.locator('#connect').click()
   if outcome=='diagnostic':page.locator('#check').click();page.wait_for_function("document.querySelector('#status').textContent.includes('no firmware writes')")
   else:
    page.locator('#flash').click();page.wait_for_function('!!window.fail')
    if outcome=='cancel':page.locator('#cancel').click()
    else:
     page.evaluate('''outcome=>{events.onState({state:outcome==='failure'?'failed-disconnected':'failed-cleanup-incomplete',phase:'security-preflight',errorCode:'SECURITY_REFUSED',securityDiagnostic:{apiVersionNumber:5,reason:'API_VERSION_REFUSED',secureBoot:'false',secureDownload:'false',flashCryptCnt:'zero',raw:'<img src=x onerror="window.pwned=1">'},message:'<script>window.pwned=1</script>'});fail(Error('private-message'));}''',outcome)
     page.wait_for_function("document.querySelector('#status').textContent.includes('fresh attempt')")
     text=page.locator('#status').inner_text();assert 'ROM ECO: 5' in text and 'API_VERSION_REFUSED' in text and 'Secure boot: false' in text
     assert 'private-message' not in text and '<script>' not in text
    assert not page.evaluate('!!window.pwned');assert page.locator('#status img, #status script').count()==0
   assert 'Press the board RESET' not in page.locator('#status').inner_text();assert page.locator('#flash').is_disabled()
   before=page.locator('#status').inner_text();page.evaluate("events.onProgress({phase:'writing',kind:'compressed-write',assetIndex:0,bytes:100,totalBytes:100,elapsedMs:90000});events.onState({state:'writing'})");assert page.locator('#status').inner_text()==before
   results.append({'outcome':outcome,'result':'PASS'});page.close()
  report={'chromium':browser.version,'boundary':'Actual app/HTML/CSS; simulated session only; real downloaded image verification; CSS 200% layout zoom (not browser toolbar zoom)','results':results}
  (root/'evidence'/'speed-ui.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));browser.close()
finally:server.shutdown()
