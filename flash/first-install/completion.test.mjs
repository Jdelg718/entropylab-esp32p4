import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const source=readFileSync(new URL('./app.mjs',import.meta.url),'utf8').replace(/^import .*;\n/gm,'');
for(const stage of ['prepare','connect','run','diagnostic'])for(const outcome of ['resolve','reject'])test(`cancel freezes UI during pending ${stage} then ${outcome}`,async()=>{
 const elements=Object.fromEntries(['status','flash','prepare','connect','check','board','consent','cancel'].map(id=>[id,{textContent:'',checked:true,disabled:false}]));
 let resolve,reject;const pending=new Promise((r,j)=>{resolve=r;reject=j;});
 vm.runInNewContext(source,{document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:()=>stage==='connect'?pending:Promise.resolve({})}},AbortController,downloadImages:()=>stage==='prepare'?pending:Promise.resolve([]),BOARD:{},createSession:()=>({run:()=>pending,checkDevice:()=>pending,cancel:async()=>{}}),ESPLoader:{},Transport:{}});
 let work=elements.prepare.onclick();if(stage!=='prepare'){await work;work=elements.connect.onclick();if(stage==='run'||stage==='diagnostic'){await work;work=elements[stage==='diagnostic'?'check':'flash'].onclick();}}
 await elements.cancel.onclick();const cancelled=elements.status.textContent;if(outcome==='resolve')resolve([]);else reject(Error('late private error'));await work;
 assert.equal(elements.status.textContent,cancelled);assert.equal(elements.flash.disabled,true);assert.doesNotMatch(elements.status.textContent,/RESET|verified/);
});
for(const outcome of ['success','failure','diagnostic'])test(`actual app handlers: ${outcome} terminal wording`,async()=>{
 const elements=Object.fromEntries(['status','flash','prepare','connect','check','board','consent','cancel'].map(id=>[id,{textContent:'',checked:true,disabled:false}]));
 let calls=0;
 const session={run:async args=>{calls++;assert.equal(args.resetOnSuccess,false);if(outcome==='failure')throw Error('readback');},checkDevice:async()=>{},cancel:async()=>{}};
 vm.runInNewContext(source,{document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>({})}},AbortController,downloadImages:async()=>({}),BOARD:{},createSession:options=>{options.onProgress({phase:'writing',kind:'compressed-write',assetIndex:2,bytes:123,totalBytes:456,elapsedMs:2000,phaseElapsedMs:1000});assert.match(elements.status.textContent,/Compressed bytes acknowledged: 123 \/ 456/);options.onState({state:'complete'});assert.doesNotMatch(elements.status.textContent,/not reported|undefined|; ;/);return session;},ESPLoader:{},Transport:{}});
 await elements.prepare.onclick();await elements.connect.onclick();await elements[outcome==='diagnostic'?'check':'flash'].onclick();
 const status=elements.status.textContent;
 if(outcome==='success'){assert.match(status,/Press the board RESET/);assert.match(status,/do not repeat installation just to boot/);assert.doesNotMatch(status,/fresh attempt/);}
 else {assert.doesNotMatch(status,/Press the board RESET/);assert.match(status,/fresh attempt/);assert.match(status,outcome==='failure'?/refused or failed/:/Diagnostic complete/);}
 assert.equal(elements.flash.disabled,true);assert.equal(calls,outcome==='diagnostic'?0:1);
 await elements.cancel.onclick();assert.equal(elements.status.textContent,status);
});
for(const outcome of ['false','reject'])test(`cancel cleanup ${outcome} stays actionable and one-shot`,async()=>{
 const elements=Object.fromEntries(['status','flash','prepare','connect','check','board','consent','cancel'].map(id=>[id,{textContent:'',checked:true,disabled:false}]));
 let settle,calls=0;const cleanup=new Promise((resolve,reject)=>{settle=()=>outcome==='false'?resolve(false):reject(Error('private cleanup error'));});
 vm.runInNewContext(source,{document:{getElementById:id=>elements[id]},AbortController,downloadImages:async()=>[],BOARD:{},createSession:()=>({cancel:()=>{calls++;return cleanup;}}),ESPLoader:{},Transport:{}});
 await elements.prepare.onclick();const cancelling=elements.cancel.onclick();
 assert.match(elements.status.textContent,/Cancelled/);assert.doesNotMatch(elements.status.textContent,/Images verified/);
 await elements.cancel.onclick();assert.equal(calls,1);settle();await cancelling;
 assert.match(elements.status.textContent,/cleanup incomplete.*[Uu]nplug/);assert.doesNotMatch(elements.status.textContent,/private cleanup error/);
 const status=elements.status.textContent;await elements.cancel.onclick();assert.equal(calls,1);assert.equal(elements.status.textContent,status);
});
