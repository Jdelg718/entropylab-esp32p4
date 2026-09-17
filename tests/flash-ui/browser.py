"""Render and exercise the real allowlisted loopback site; existing Playwright environment.
No connected hardware, WebSerial driver, firmware, or OS acceptance tests.
"""
import importlib.util
import json
import os
import shutil
import tempfile
import threading
import urllib.error
import urllib.request
from pathlib import Path
from playwright.sync_api import sync_playwright

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(os.environ['FLASH_UI_EVIDENCE']) if os.environ.get('FLASH_UI_EVIDENCE') else Path(tempfile.mkdtemp(prefix='entropylab-ui-'))
OUT.mkdir(parents=True, exist_ok=True)
# Explicit override, installed Chromium/Chrome, then Playwright-managed Chromium.
CHROME = os.environ.get('FLASH_UI_CHROME') or next((path for name in ('chromium', 'chromium-browser', 'google-chrome', 'google-chrome-stable') if (path := shutil.which(name))), None)
spec=importlib.util.spec_from_file_location('preview',ROOT/'scripts/serve-flash-ui.py')
preview=importlib.util.module_from_spec(spec);spec.loader.exec_module(preview)
server=preview.ThreadingHTTPServer(('127.0.0.1',0),preview.Handler)
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
base=f'http://127.0.0.1:{server.server_port}'
report={'scope':'Source-preview browser UI only; no hardware or OS/browser acceptance claim','cases':[]}
checks=['board','revision','connector','practice','risk']

def geometry(page,width):
    result=page.evaluate(r'''() => ({width:innerWidth,scroll:document.documentElement.scrollWidth,body:document.body.scrollWidth,undersized:[...document.querySelectorAll('button,summary,label.check,select,textarea')].filter(e=>e.getBoundingClientRect().width && (e.getBoundingClientRect().height<44||e.getBoundingClientRect().width<44)).map(e=>e.id||e.textContent),privatePath:/\/(?:home|Users|opt)\//.test(document.body.innerText)})''')
    assert result['scroll']==width and result['body']==width,result
    assert not result['undersized'] and not result['privatePath'],result
    return result

try:
    with urllib.request.urlopen(base+'/flash/') as r:
        assert r.status==200
        assert "connect-src 'none'" in r.headers['Content-Security-Policy']
        assert 'serial=()' in r.headers['Permissions-Policy']
    rejected=[]
    for path in ['/../AGENTS.md','/%2e%2e/AGENTS.md','/.git/config','/packaging/candidate/manifest.json','/flash/bootloader.bin','/flash/entropylab_fixture.bin','/candidate.zip','/flash/','/flash/?x=1','/not-allowlisted/']:
        if path=='/flash/': continue
        try: urllib.request.urlopen(base+path);raise AssertionError(path+' unexpectedly served')
        except urllib.error.HTTPError as e: assert e.code==404
        rejected.append(path)
    req=urllib.request.Request(base+'/flash/',headers={'Host':'external.invalid'})
    try: urllib.request.urlopen(req);raise AssertionError('Host not rejected')
    except urllib.error.HTTPError as e: assert e.code==403
    report['server']={'root':'explicit five-file allowlist only','rejectedPaths':rejected,'foreignHost':403,'serialPolicy':'denied','connectCSP':'none'}
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=CHROME,headless=True,args=['--no-sandbox'])
        report['chromium']=browser.version
        for name,width,height,mobile in [('desktop',1440,1150,False),('mobile',390,844,True),('narrow',320,800,True)]:
            context=browser.new_context(viewport={'width':width,'height':height},is_mobile=mobile,device_scale_factor=1)
            context.add_init_script('''window.__portCalls=0;Object.defineProperty(navigator,'serial',{value:{requestPort(){window.__portCalls++;throw Error('Unexpected serial request')},getPorts(){window.__portCalls++;throw Error('Unexpected serial enumeration')}}});''')
            page=context.new_page();errors=[];requests=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.on('console',lambda m:errors.append(m.text) if m.type=='error' else None)
            page.on('request',lambda r:requests.append({'path':r.url.replace(base,''),'method':r.method}))
            page.goto(base+'/flash/');page.wait_for_function("document.querySelector('#images').children.length === 3")
            assert page.locator('h1').inner_text()=='Install EntropyLab'
            assert page.locator('#environment').is_visible()==mobile
            assert page.locator('#images tr').count()==3
            assert '54ee881b' in page.locator('#provenance').inner_text()
            assert page.locator('#connect').is_disabled() and page.locator('#download').is_disabled()
            assert [e.inner_text() for e in page.locator('.steps .state').all()]==['BLOCKED','PENDING','PENDING','PENDING']
            for key in checks: page.locator('#'+key).check()
            assert 'All local confirmations recorded' in page.locator('#confirm-status').inner_text()
            assert page.locator('#connect').is_disabled()
            for key in checks: page.locator('#'+key).uncheck()
            dom_install=geometry(page,width)
            page.evaluate('scrollTo(0,0)');page.screenshot(path=str(OUT/f'{name}-install.png'),full_page=True)
            page.screenshot(path=str(OUT/f'{name}-preview.png'),full_page=False)
            page.locator('#board').check();page.locator('#risk').check()
            page.locator('#update').click()
            assert page.locator('#mode-title').inner_text()=='Application-only update'
            assert not page.locator('#risk').is_checked() and page.locator('#board').is_checked()
            assert page.locator('#images tr:visible').count()==1
            assert 'not an approved app-only update manifest' in page.locator('#candidate-scope').inner_text()
            for key in checks: page.locator('#'+key).check()
            assert page.locator('#connect').is_disabled()
            dom_update=geometry(page,width)
            page.evaluate('scrollTo(0,0)');page.screenshot(path=str(OUT/f'{name}-update.png'),full_page=True)
            page.locator('#update').focus();page.keyboard.press('ArrowLeft')
            assert page.locator('#install').get_attribute('aria-selected')=='true'
            assert not page.locator('#risk').is_checked()
            page.keyboard.press('End');assert page.locator('#update').get_attribute('aria-selected')=='true'
            page.get_by_text('Feedback stays in your hands',exact=False).click()
            assert page.locator('#feedback').is_disabled()
            page.locator('#feedback-opt').check();page.locator('#feedback-topic').select_option('wording')
            assert 'Topic: wording' in page.locator('#feedback').input_value()
            assert page.locator('#feedback').get_attribute('readonly') is not None
            assert page.locator('#share').is_disabled()
            geometry(page,width)
            page.screenshot(path=str(OUT/f'{name}-feedback.png'),full_page=True)
            page.locator('#feedback-opt').uncheck();assert page.locator('#feedback').input_value()==''
            page.locator('#feedback-opt').check()
            # DOM tampering cannot cause a hardware action: buttons have no hardware handler.
            page.evaluate("for(const id of ['connect','download','share']){let e=document.getElementById(id);e.disabled=false;e.click()}")
            denial=page.evaluate("""async()=>{const {hardwareAdapter}=await import('./model.mjs');return ['connect','install','update'].map(k=>{try{hardwareAdapter[k]({canConnect:true});return 'FAIL'}catch(e){return e.message.split(':')[0]}})}""")
            assert denial==['HARD_BLOCKED']*3
            assert page.evaluate('window.__portCalls')==0
            assert page.evaluate('localStorage.length+sessionStorage.length')==0
            assert context.cookies()==[]
            assert page.evaluate('async()=>(await navigator.serviceWorker.getRegistrations()).length')==0
            expected={'/flash/','/flash/style.css','/flash/app.mjs','/flash/model.mjs','/flash/candidate.mjs'}
            assert all(r['method']=='GET' and r['path'] in expected for r in requests),requests
            assert len(requests)==5,requests
            page.reload();page.wait_for_function("document.querySelector('#images').children.length===3")
            assert page.locator('#connect').is_disabled()
            assert not page.locator('#feedback-opt').is_checked()
            assert not page.locator('#board').is_checked()
            assert page.locator('#feedback').input_value()==''
            assert not errors,errors
            report['cases'].append({'name':name,'installGeometry':dom_install,'updateGeometry':dom_update,'pageAndConsoleErrors':errors,'initialRequests':requests[:5],'portCalls':0,'checks':['all confirmations cannot unlock','mode-switch consent reset','keyboard tabs','three images vs app-only','pending phases distinct','feedback opt-in/allowlist/discard','adapter and tampered DOM deny hardware','no browser persistence','reload resets all state','44px targets','no overflow']})
            context.close()
        for name,mobileUA in [('unsupported-desktop',False),('mobile-ua-wide',True)]:
            options={'viewport':{'width':1440,'height':1150}}
            if mobileUA: options['user_agent']='Mozilla/5.0 (iPad; CPU OS 17_0 like Mac OS X) AppleWebKit/605.1.15 Mobile/15E148 Safari/604.1'
            context=browser.new_context(**options)
            context.add_init_script('delete Navigator.prototype.serial')
            page=context.new_page();page.goto(base+'/flash/');page.wait_for_function("document.querySelector('#images').children.length===3")
            assert page.locator('#environment').is_visible()
            assert ('Mobile installation' if mobileUA else 'does not expose WebSerial') in page.locator('#environment').inner_text()
            assert page.locator('#connect').is_disabled()
            page.screenshot(path=str(OUT/f'{name}.png'),full_page=True)
            report['cases'].append({'name':name,'result':'review-only banner, hardware disabled; UA simulation, not Safari acceptance'})
            context.close()
        context=browser.new_context(java_script_enabled=False);page=context.new_page();page.goto(base+'/flash/')
        assert page.locator('noscript').is_visible()
        assert page.locator('#connect').is_disabled() and page.locator('#download').is_disabled()
        report['cases'].append({'name':'javascript-disabled','result':'fail closed'})
        context.close()
        context=browser.new_context();page=context.new_page();page.route('**/candidate.mjs',lambda route:route.abort());page.goto(base+'/flash/')
        assert page.locator('#connect').is_disabled() and page.locator('#download').is_disabled()
        assert 'metadata unavailable' in page.locator('#candidate-status').inner_text()
        report['cases'].append({'name':'metadata-module-failed','result':'fail closed; expected network error injected'})
        context.close();browser.close()
    report['result']='PASS'
finally:
    server.shutdown();server.server_close();thread.join()
    (OUT/'browser-results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
