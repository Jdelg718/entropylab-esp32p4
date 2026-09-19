import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const source=readFileSync(new URL('./app.mjs',import.meta.url),'utf8').replace(/^import .*;\n/gm,'');
for(const outcome of ['success','failure','diagnostic'])test(`actual app handlers: ${outcome} terminal wording`,async()=>{
 const elements=Object.fromEntries(['status','flash','prepare','connect','check','board','consent','cancel'].map(id=>[id,{textContent:'',checked:true,disabled:false}]));
 let calls=0;
 const session={run:async args=>{calls++;assert.equal(args.resetOnSuccess,false);if(outcome==='failure')throw Error('readback');},checkDevice:async()=>{},cancel:async()=>{}};
 vm.runInNewContext(source,{document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>({})}},AbortController,downloadImages:async()=>({}),BOARD:{},createSession:()=>session,ESPLoader:{},Transport:{}});
 await elements.prepare.onclick();await elements.connect.onclick();await elements[outcome==='diagnostic'?'check':'flash'].onclick();
 const status=elements.status.textContent;
 if(outcome==='success'){assert.match(status,/Press the board RESET/);assert.match(status,/do not repeat installation just to boot/);assert.doesNotMatch(status,/fresh attempt/);}
 else {assert.doesNotMatch(status,/Press the board RESET/);assert.match(status,/fresh attempt/);assert.match(status,outcome==='failure'?/refused or failed/:/Diagnostic complete/);}
 assert.equal(elements.flash.disabled,true);assert.equal(calls,outcome==='diagnostic'?0:1);
});
