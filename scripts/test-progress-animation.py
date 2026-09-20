"""Actual installer presentation with synthetic session events; no hardware IO."""
import functools
import http.server
import json
import os
import pathlib
import tempfile
import threading
from playwright.sync_api import sync_playwright

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = pathlib.Path(os.environ.get('PROGRESS_OUTPUT') or tempfile.mkdtemp(prefix='progress-animation-'))
OUT.mkdir(parents=True, exist_ok=True)

class Quiet(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Quiet, directory=str(ROOT)))
threading.Thread(target=server.serve_forever, daemon=True).start()
MOCK = """export const BOARD='public-fixture'; export function createSession(o){window.events=o;return {run:async()=>{await new Promise((r,j)=>{window.finish=r;window.fail=j;});},cancel:async()=>{window.fail?.(Error('cancelled'));},checkDevice:async()=>{}};}"""
results = []
try:
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, executable_path=os.environ.get('CHROMIUM_PATH'), args=['--no-sandbox'])
        for width, motion in [(320, 'no-preference'), (390, 'reduce'), (1280, 'no-preference')]:
            page = browser.new_page(viewport={'width': width, 'height': 950}, reduced_motion=motion)
            errors = []
            page.on('pageerror', lambda e: errors.append(str(e)))
            page.route('**/adapter/profiles.mjs', lambda r: r.fulfill(content_type='text/javascript', body=MOCK))
            page.add_init_script("Object.defineProperty(navigator,'serial',{value:{requestPort:async()=>({})}})")
            page.goto(f'http://127.0.0.1:{server.server_port}/flash/first-install/')
            assert page.locator('#image-progress').is_hidden()
            page.locator('#board').check()
            page.locator('#consent').check()
            page.locator('#prepare').click()
            page.wait_for_function('!!window.events')
            page.locator('#connect').click()
            page.locator('#flash').click()
            page.wait_for_function('!!window.finish')
            page.evaluate("""() => {
                window.emit=(bytes,elapsedMs,extra={})=>events.onProgress({phase:'writing',kind:'compressed-write',assetIndex:2,bytes,totalBytes:1000,elapsedMs,...extra});
                window.fill=document.querySelector('#image-progress-fill');
                window.track=document.querySelector('#image-progress');
                window.check=(v,m)=>{if(!v)throw Error(m)};
                emit(250,10);
                check(fill.style.width==='25%','immediate width');
                check(document.querySelector('#status').textContent.endsWith('250 / 1000'),'exact text');
                const a=fill.getAnimations();
                check(a.length===(matchMedia('(prefers-reduced-motion: reduce)').matches?0:1),'motion preference');
                if(a.length){check(a[0].effect.getTiming().iterations===1,'finite');check(a[0].effect.getTiming().duration===180,'bounded');}
            }""")
            page.wait_for_function('fill.getAnimations().length===0')
            # A quiet event stream must not advance width or elapsed labels.
            before = page.locator('#status').inner_text()
            page.wait_for_timeout(250)
            assert page.locator('#status').inner_text() == before
            assert page.locator('#image-progress-fill').evaluate("e=>e.style.width") == '25%'
            page.screenshot(path=str(OUT / f'writing-{width}.png'), full_page=True)
            page.evaluate("""() => {
                emit(500,20);check(fill.style.width==='50%','next real acknowledgement');
                emit(500,21);check(fill.getAnimations().length===0,'duplicate does not pulse');
                emit(400,22);check(fill.style.width==='50%','regression rejected');
                emit(600,23,{totalBytes:null});check(track.hidden,'unknown total not indeterminate');
                events.onState({state:'verifying-readback'});check(track.hidden,'phase hides stale image');
                emit(750,24,{phase:'verifying-readback',kind:'image-read',assetIndex:2});
                check(track.dataset.kind==='image-read','read style');
                check(document.querySelector('#status').textContent.includes('SHA-256 pending'),'read not verified');
            }""")
            page.screenshot(path=str(OUT / f'readback-{width}.png'), full_page=True)
            page.evaluate("""() => {
                emit(1000,25,{phase:'verifying-readback',kind:'image-read',assetIndex:2});
                check(document.querySelector('#status').textContent.includes('SHA-256 pending'),'full read still pending');
                emit(1000,26,{phase:'verifying-readback',kind:'image-verified',assetIndex:2});
                check(track.dataset.kind==='image-verified'&&fill.getAnimations().length===0,'verified static');
                check(document.querySelector('#status').textContent.includes('SHA-256 verified bytes: 1000 / 1000'),'verified text');
            }""")
            page.screenshot(path=str(OUT / f'verified-{width}.png'), full_page=True)
            layout = page.evaluate("({width:innerWidth,scroll:document.documentElement.scrollWidth,client:document.documentElement.clientWidth})")
            assert layout['scroll'] <= layout['client'], layout
            nodes = page.context.new_cdp_session(page).send('Accessibility.getFullAXTree')['nodes']
            statuses = [n for n in nodes if n.get('role', {}).get('value') == 'status']
            assert len(statuses) == 1
            props = {v['name']: v.get('value', {}).get('value') for v in statuses[0].get('properties', [])}
            assert props['live'] == 'polite' and props['atomic'] is True
            assert not [n for n in nodes if n.get('role', {}).get('value') == 'progressbar']
            # Changing preference mid-pulse cancels motion without losing true bytes.
            page.evaluate("""() => {
                emit(100,27,{phase:'verifying-readback',kind:'image-read',assetIndex:3});
                window.preferencePulse=null;
                if(!matchMedia('(prefers-reduced-motion: reduce)').matches){
                    const animations=fill.getAnimations();
                    check(animations.length===1,'live preference pulse');
                    window.preferencePulse=animations[0];
                    check(preferencePulse.playState==='running','preference pulse has not ended');
                    // Freeze a live pulse: natural expiry cannot satisfy cancellation.
                    preferencePulse.pause();
                    check(preferencePulse.playState==='paused','preference pulse paused');
                }
            }""")
            page.emulate_media(reduced_motion='reduce')
            page.wait_for_function('fill.getAnimations().length===0 && (!preferencePulse || preferencePulse.playState==="idle")')
            assert page.locator('#image-progress-fill').evaluate('e=>e.style.width') == '10%'
            page.emulate_media(reduced_motion='no-preference')
            page.wait_for_function("!matchMedia('(prefers-reduced-motion: reduce)').matches")
            # Emit, prove a live pulse, and terminate in one JS turn (no expiry race).
            outcome = {320: 'cancel', 390: 'failed-cleanup-incomplete', 1280: 'complete'}[width]
            page.evaluate("""outcome=>{
                emit(200,28,{phase:'verifying-readback',kind:'image-read',assetIndex:3});
                const animations=fill.getAnimations();
                check(animations.length===1,'live terminal pulse same turn');
                const pulse=animations[0];
                check(pulse.playState==='running','terminal pulse has not ended');
                if(outcome==='cancel')document.querySelector('#cancel').onclick();
                else {events.onState({state:outcome});if(outcome==='complete')finish();else fail(Error('fixture'));}
                check(track.hidden&&fill.getAnimations().length===0&&pulse.playState==='idle','terminal stops motion immediately');
            }""", outcome)
            page.wait_for_function('document.querySelector("#flash").disabled')
            final = page.locator('#status').inner_text()
            page.evaluate("emit(1000,30,{phase:'verifying-readback',kind:'image-verified',assetIndex:3});events.onState({state:'writing'});")
            assert page.locator('#status').inner_text() == final
            assert page.locator('#image-progress').is_hidden()
            assert not errors, errors
            page.screenshot(path=str(OUT / f'{outcome}-{width}.png'), full_page=True)
            results.append({'width': width, 'motion': motion, 'outcome': outcome, 'layout': layout, 'status': props, 'errors': errors, 'result': 'PASS'})
            page.close()
        report = {'chromium': browser.version, 'boundary': 'Real app/CSS with synthetic session events, no hardware; no screen-reader listening', 'results': results}
        (OUT / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report, indent=2))
        browser.close()
finally:
    server.shutdown()
