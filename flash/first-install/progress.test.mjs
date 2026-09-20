import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const source=readFileSync(new URL('./app.mjs',import.meta.url),'utf8').replace(/^import .*;\n/gm,'');
async function fixture(){
 const elements=Object.fromEntries(['status','flash','prepare','connect','check','board','consent','cancel'].map(id=>[id,{textContent:'',checked:true,disabled:false}]));
 let callbacks;
 vm.runInNewContext(source,{document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>({})}},AbortController,downloadImages:async()=>[],BOARD:{},createSession:o=>{callbacks=o;return {cancel:async()=>{}};},ESPLoader:{},Transport:{}});
 await elements.prepare.onclick();
 return {elements,emit:callbacks.onProgress,state:callbacks.onState,text:()=>elements.status.textContent};
}
test('malformed, stale and unknown-total events cannot invent verification',async()=>{
 const f=await fixture();f.emit(event(100,100));const before=f.text();
 for(const p of [null,undefined,{},event(-1,200),event(101,NaN),event(10001,200),event(1,200),event(200,90),event(100,200,{assetIndex:4}),event(100,200,{kind:'image-verified',phase:'verifying-readback',totalBytes:null})])assert.doesNotThrow(()=>f.emit(p));
 assert.equal(f.text(),before);
 f.emit(event(200,400,{totalBytes:null}));assert.match(f.text(),/acknowledged: 200$/);
 f.emit(event(300,700,{kind:'image-read',phase:'verifying-readback',assetIndex:3,totalBytes:500}));assert.match(f.text(),/erased tail.*bootloader/);assert.match(f.text(),/SHA-256 pending/);
 f.emit(event(500,710,{kind:'image-verified',phase:'verifying-readback',assetIndex:3,totalBytes:500}));assert.match(f.text(),/SHA-256 verified bytes/);
 f.emit(event(400,800,{kind:'image-read',phase:'verifying-readback',assetIndex:3,totalBytes:500}));assert.match(f.text(),/SHA-256 verified bytes/);
 f.state({state:'complete'});const done=f.text();f.emit(event(600,900));assert.equal(f.text(),done);
});
test('phase labels and finite security diagnostics survive without raw messages',async()=>{
 const f=await fixture();f.state({state:'changing-baud'});assert.match(f.text(),/460800/);
 f.state({state:'failed-disconnected',phase:'security-preflight',errorCode:'SECURITY_REFUSED',securityDiagnostic:{apiVersionNumber:5,reason:'API_VERSION_REFUSED',secureBoot:'false',secureDownload:'false',flashCryptCnt:'zero'},message:'PRIVATE',raw:'PRIVATE'});
 assert.match(f.text(),/ROM ECO: 5/);assert.match(f.text(),/API_VERSION_REFUSED/);assert.match(f.text(),/Secure boot: false/);assert.doesNotMatch(f.text(),/PRIVATE/);
});
test('hostile diagnostic strings are omitted and incomplete cleanup is actionable',async()=>{
 const f=await fixture();f.state({state:'failed-cleanup-incomplete',errorCode:'PRIVATE_TOKEN',securityDiagnostic:{apiVersionNumber:'<img>',reason:'<img src=x onerror=alert(1)>',secureBoot:'PRIVATE',raw:'PRIVATE'}});
 assert.doesNotMatch(f.text(),/PRIVATE|<img/);assert.match(f.text(),/unplug/i);
});
const event=(bytes,elapsedMs,extra={})=>({phase:'writing',kind:'compressed-write',assetIndex:0,bytes,totalBytes:10000,elapsedMs,phaseElapsedMs:elapsedMs,...extra});
test('continuous progress is throttled from last display, never starved',async()=>{
 const f=await fixture(); f.emit(event(0,0)); const first=f.text();
 f.emit(event(100,100));f.emit(event(200,200));assert.equal(f.text(),first);
 f.emit(event(300,300));assert.match(f.text(),/300 \/ 10000/);
 assert.match(f.text(),/Compressed bytes acknowledged/);assert.doesNotMatch(f.text(),/%/);
});
