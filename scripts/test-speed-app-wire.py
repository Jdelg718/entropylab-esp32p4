"""Actual app -> profiles -> GuardedSession -> pinned vendor -> synthetic port.
Only the app's vendor import is redirected to the existing synthetic-device subclasses.
No session/profiles/adapter method substitution, no physical WebSerial access.
"""
import tempfile
import functools,http.server,json,os,pathlib,threading
from playwright.sync_api import sync_playwright
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=pathlib.Path(os.environ['SPEED_GATE_OUTPUT']) if os.environ.get('SPEED_GATE_OUTPUT') else pathlib.Path(tempfile.mkdtemp(prefix='entropylab-app-wire-'));OUT.mkdir(exist_ok=True)
class Quiet(http.server.SimpleHTTPRequestHandler):
 def log_message(self,format,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),functools.partial(Quiet,directory=str(ROOT)))
threading.Thread(target=server.serve_forever,daemon=True).start()
app=(ROOT/'flash/first-install/app.mjs').read_text()
old="from './vendor/esptool-js.mjs'";assert app.count(old)==1
app=app.replace(old,"from './qa-device.mjs'")
fixture=(ROOT/'flash/first-install/speed-browser.mjs').read_text()
old='return {session,args,stats,states,bytes,profile,progress};';assert fixture.count(old)==1
fixture=fixture.replace(old,'return {session,args,stats,states,bytes,profile,progress,Loader:WireLoader,Transport:WireTransport};')+'\nexport {fixture};\n'
device="""import {fixture} from './speed-browser.mjs';const f=fixture(window.options);window.deviceFixture=f;Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>f.args.port}});export const ESPLoader=f.Loader;export const Transport=f.Transport;"""
results=[]
try:
 with sync_playwright() as p:
  browser=p.chromium.launch(headless=True,executable_path=os.environ.get('CHROMIUM_PATH'),args=['--no-sandbox'])
  for name,options in [('success',{}),('malformed-header',{'badHeader':True}),('reopen-refused',{'reopenFails':True}),('UI-cancel-during-baud',{'faultAt':'baud','fault':'stall'})]:
   page=browser.new_page();errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
   for pattern,body in [('**/first-install/app.mjs',app),('**/first-install/speed-browser.mjs',fixture),('**/first-install/qa-device.mjs',device)]:
    page.route(pattern,lambda route,request,body=body:route.fulfill(content_type='text/javascript',body=body))
   page.add_init_script('window.options='+json.dumps(options))
   page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
   assert page.locator('#flash').is_disabled()
   page.locator('#board').check();page.locator('#consent').check();page.locator('#prepare').click()
   page.wait_for_function("!document.querySelector('#connect').disabled")
   page.locator('#connect').click();page.locator('#flash').click()
   if name=='UI-cancel-during-baud':
    page.wait_for_function('deviceFixture.stats.faultReached===true');page.locator('#cancel').click()
    page.wait_for_function("document.querySelector('#status').textContent.startsWith('Cancelled')")
    page.wait_for_function('deviceFixture.args.port.readable===null && deviceFixture.args.port.writable===null')
   else:page.wait_for_function("/Press the board RESET|fresh attempt/.test(document.querySelector('#status').textContent)",timeout=60000)
   text=page.locator('#status').inner_text();stats=page.evaluate('deviceFixture.stats')
   assert stats['resets']==0 and page.locator('#flash').is_disabled()
   assert page.evaluate('deviceFixture.args.port.readable===null && deviceFixture.args.port.writable===null')
   if name=='success':
    assert stats['writes']==3 and stats['readbacks']==6 and stats['baudRates']==[115200,460800],stats
    assert 'All three images and erased tails verified' in text and 'Press the board RESET' in text
   else:
    assert stats['writes']==0 and stats.get('blocks',0)==0 and stats['readbacks']==0 and 'Press the board RESET' not in text,stats
    if name=='malformed-header':
     assert 'BAUD_SWITCH_REFUSED' in text and stats['switches']==1 and stats['opens']==1 and stats['baudRates']==[115200],stats
    if name=='reopen-refused':
     assert 'DEPENDENCY_ERROR' in text and stats['switches']==1 and stats['opens']==2 and stats['baudRates']==[115200,460800],stats
    if name=='UI-cancel-during-baud':assert text.startswith('Cancelled') and stats['faultReached'] and stats['targetOpcodes']==1 and stats['baudRates']==[115200],stats
   assert not errors,errors
   results.append({'scenario':name,'result':'PASS','stats':stats,'terminal':text,'page_errors':errors});page.close()
  result={'chromium':browser.version,'boundary':__doc__,'results':results}
  (OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));browser.close()
finally:server.shutdown()
