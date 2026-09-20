let MODEL_BAUD=115200;
import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {inflateSync} from 'node:zlib';
import {GuardedSession, BOARD, availability, FACTORY_PROFILE} from './adapter/adapter.mjs';
const source = await readFile(new URL('./vendor/esptool-js.mjs', import.meta.url));
const {ESPLoader,Transport} = await import('data:text/javascript;base64,' + source.toString('base64'));
const appBytes = await readFile(new URL('./firmware/entropylab.bin',import.meta.url));
const sha = b => createHash('sha256').update(b).digest('hex');
const u32 = (b,n=0) => new DataView(b.buffer,b.byteOffset,b.byteLength).getUint32(n,true);
const awaitBoot=await readFile(new URL('./firmware/bootloader.bin',import.meta.url));
const awaitTable=await readFile(new URL('./firmware/partition-table.bin',import.meta.url));
function fixture(options={}) {
  const bytes = {boot:new Uint8Array(awaitBoot),table:new Uint8Array(awaitTable),app:new Uint8Array(appBytes)};
  const pins = Object.entries(bytes).map(([role,b])=>({role,offset:{boot:0x2000,table:0x8000,app:0x10000}[role],length:b.length,sha256:sha(b)}));
  const profile=FACTORY_PROFILE;
  const memory=new Map();
  // Public destructive policy deliberately has no private predecessor fixtures.
  const stats={opens:0,closes:0,writes:0,resets:0,stub:0,security:0,readbacks:0,aborts:0};
  let loader, rejectWrite;
  const port={readable:null,writable:null,getInfo:()=>({}),async open(serial){stats.baudRates??=[];stats.baudRates.push(serial.baudRate);stats.opens++;this.writable={getWriter:()=>({async write(wire){ clock.advance(wire.length*10/MODEL_BAUD*1000); stats.wireBytes=(stats.wireBytes||0)+wire.length;
      const a=[]; for(let i=1;i<wire.length-1;i++){if(wire[i]===0xdb){i++;a.push(wire[i]===0xdc?0xc0:0xdb);}else a.push(wire[i]);}
      const packet=new Uint8Array(a); if(packet.length===4)return;
      if(options.hangWrite && packet[1]===loader.ESP_GET_SECURITY_INFO) await new Promise((_,reject)=>{rejectWrite=reject;});
      const [value,data]=await loader.emulateCommand(packet[1],packet.slice(8));
      const response=new Uint8Array(8+data.length);response[0]=1;response[1]=packet[1];new DataView(response.buffer).setUint16(2,data.length,true);new DataView(response.buffer).setUint32(4,value,true);response.set(data,8);loader.transport.queue.unshift(response);
    },async abort(){stats.aborts++;rejectWrite?.(Error('aborted writer'));},releaseLock(){}})};},async close(){stats.closes++;if(options.closeFails) throw Error('close failed');this.writable=null;},async setSignals(){stats.resets++;}};
  class WireTransport extends Transport {
    queue=[];
    async read(timeout){stats.timeouts??=[];stats.timeouts.push(timeout);if(!this.queue.length) throw Error('empty synthetic wire');const b=this.queue.shift();clock.advance((b.length+2+Array.from(b).filter(x=>x===0xc0||x===0xdb).length)*10/MODEL_BAUD*1000 + (b.length>1000 ? (options.slow?330:0):0));return b;}
    readLoop(){}
  }
  class WireLoader extends ESPLoader {
    constructor(options){super(options);loader=this;}
    async openAndSync(mode,attempts){assert.equal(mode,'default_reset');assert.equal(attempts,1);await this.transport.connect();this.syncStubDetected=!!options.existingStub;}
    async runSpiflashCommand(cmd){assert.equal(cmd,0x9f);return options.jedec ?? 0x1940ef;}
    async usesUsbOtg(){return false;}
    async emulateCommand(op,data=new Uint8Array()){
      let value=0, out=new Uint8Array(2);
      if(op===this.ESP_CHANGE_BAUDRATE){stats.switches=(stats.switches||0)+1;assert.equal(stats.stub,1);assert.equal(stats.writes,0);assert.equal(this.transport.baudrate,115200);assert.deepEqual([u32(data),u32(data,4)],[460800,115200]);if(options.baudRefused)out[0]=1;} else if(op===this.ESP_GET_SECURITY_INFO){
        stats.security++; if(options.unknown) throw Error('unknown');
        out=new Uint8Array(24);const d=new DataView(out.buffer);d.setUint32(0,options.flags||0,true);out[4]=options.crypt||0;d.setUint32(12,options.chip??18,true);d.setUint32(16,options.api??0,true);
      } else if(op===this.ESP_READ_REG){
        const addr=u32(data);value=addr===0x5012d04c ? ((options.revMajor??1)<<4) : 0;
      } else if(op===this.ESP_MEM_END){stats.stub++;this.transport.queue.push(new TextEncoder().encode('OHAI'));}
      else if(op===this.ESP_FLASH_DEFL_BEGIN){this.address=u32(data,12);this.parts=[];stats.writes++;}
      else if(op===this.ESP_FLASH_DEFL_DATA){stats.blocks=(stats.blocks||0)+1;if(options.blockFails)throw Error('synthetic write failure');this.parts.push(data.slice(16));}
      else if(op===this.ESP_FLASH_DEFL_END){assert.equal(u32(data),1);const written=new Uint8Array(inflateSync(Buffer.concat(this.parts)));memory.set(this.address,written);const pin=profile.assets.find(p=>p.offset===this.address);assert.equal(Math.ceil(written.length/4096)*4096+this.address,pin.eraseEnd);memory.set(this.address+written.length,new Uint8Array(pin.eraseEnd-this.address-written.length).fill(255));}
      else if(op===this.ESP_READ_FLASH){stats.readbacks++;let b=memory.get(u32(data)).slice(0,u32(data,4));if(options.mismatch===u32(data) && stats.writes) b[0]++;if(options.short) b=new Uint8Array(b.length+1);for(let i=0;i<b.length;i+=4096)this.transport.queue.push(b.slice(i,i+4096));this.transport.queue.push(new Uint8Array(createHash('md5').update(b).digest()));}
      return [value,out];
    }
  }
  const states=[],progress=[];
  const session=new GuardedSession({profile,Loader:WireLoader,Transport:WireTransport,onProgress:p=>progress.push(p),onState:s=>{states.push(s.state);options.onState?.(s,session);if(s.state===options.stallPhase)clock.advance(options.stallMs);},...(options.timeout?{phaseTimeoutMs:options.timeout}:{})});
  const args={port,assets:profile.assets.map(p=>({offset:p.offset,bytes:bytes[p.role]})),boardConfirmation:BOARD,firstInstallConsent:true,resetOnSuccess:false};
  return {session,args,stats,states,bytes,profile,progress};
}

const original={setTimeout:globalThis.setTimeout,clearTimeout:globalThis.clearTimeout,performance:globalThis.performance};
const clock={now:0,next:0,timers:new Map(),advance(ms){const end=this.now+ms;while(true){const entry=[...this.timers].filter(([,t])=>t.at<=end).sort((a,b)=>a[1].at-b[1].at)[0];if(!entry)break;this.now=entry[1].at;this.timers.delete(entry[0]);entry[1].fn();}this.now=end;}};
function setup(){clock.now=0;clock.timers.clear();globalThis.performance={now:()=>clock.now};globalThis.setTimeout=(fn,ms)=>{if(ms===50){queueMicrotask(()=>{clock.advance(ms);fn();});return -1;}const id=++clock.next;clock.timers.set(id,{at:clock.now+ms,fn});return id;};globalThis.clearTimeout=id=>clock.timers.delete(id);}
function restore(){Object.assign(globalThis,original);}


test('DEV TEST diagnostic ECO2 is reachable without flash or reset',async()=>{setup();try{const f=fixture({api:2});const r=await f.session.checkDevice(f.args);assert.equal(r.diagnostic,true);assert.equal(r.verified,false);assert.equal(f.stats.writes,0);assert.equal(f.stats.readbacks,0);assert.equal(f.stats.stub,1);assert.equal(f.stats.resets,0);assert.deepEqual(f.stats.baudRates,[115200,460800]);await assert.rejects(f.session.checkDevice(f.args),/SESSION_ALREADY_USED/);}finally{restore();}});
for(const method of ['run','checkDevice'])for(const opts of [{chip:9},{revMajor:2},{api:5},{flags:1},{flags:4},{flags:0x800},{crypt:1},{jedec:0x1840ef},{existingStub:true}])test('DEV TEST both routes reject unsafe target/security '+method+' '+JSON.stringify(opts),async()=>{setup();try{const f=fixture(opts);await assert.rejects(f.session[method](f.args));assert.equal(f.stats.writes,0);assert.equal(f.stats.resets,0);assert.equal(f.states.includes('complete'),false);assert.equal(f.states.includes('diagnostic-complete'),false);}finally{restore();}});
for(const field of ['boardConfirmation','firstInstallConsent'])test('DEV TEST explicit consent before transport '+field,async()=>{setup();try{const f=fixture();f.args[field]=false;await assert.rejects(f.session.run(f.args));assert.equal(f.stats.opens,0);assert.equal(f.stats.writes,0);}finally{restore();}});
test('DEV TEST ECO2 install writes all three and verifies all six',async()=>{setup();try{const f=fixture({api:2});assert.equal((await f.session.run(f.args)).verified,true);assert.equal(f.stats.writes,3);assert.equal(f.stats.readbacks,6);assert.equal(f.stats.resets,0);}finally{restore();}});
for(const index of [1,2])test('DEV TEST rejects corrupted tail '+index,async()=>{setup();try{const f=fixture({mismatch:FACTORY_PROFILE.tailReadbacks[index].offset});await assert.rejects(f.session.run(f.args));assert.equal(f.states.includes('complete'),false);assert.equal(f.stats.resets,0);}finally{restore();}});

test('shipped vendor: three writes and six image/tail transactions',async()=>{setup();try{const f=fixture();assert.equal((await f.session.run(f.args)).verified,true);assert.equal(f.stats.writes,3);assert.equal(f.stats.readbacks,6);assert.equal(f.stats.resets,0);await assert.rejects(f.session.run(f.args),/SESSION_ALREADY_USED/);}finally{restore();}});
for(const [name,opts] of Object.entries({chip:{chip:9},revision:{revMajor:2},eco:{api:5},flags:{flags:0x800},encryption:{crypt:2},size:{jedec:0x1840ef},block:{blockFails:true},tail:{mismatch:29456},expiry:{stallPhase:'writing',stallMs:900001},cancel:{onState:(s,session)=>{if(s.state==='writing')session.cancel();}}}))test('shipped vendor fail closed '+name,async()=>{setup();try{const f=fixture(opts);await assert.rejects(f.session.run(f.args));assert.equal(f.stats.resets,0);if(name==='block')assert.equal(f.stats.blocks,1);else if(name!=='tail')assert.equal(f.stats.writes,0);await assert.rejects(f.session.run(f.args),/SESSION_ALREADY_USED/);}finally{restore();}});

test('baud command ACK before reopening at selected rate',async()=>{setup();try{const f=fixture();await f.session.run(f.args);assert.equal(f.stats.switches,1);assert.deepEqual(f.stats.baudRates,[115200,460800]);}finally{restore();}});

test('refused baud ACK stops before reopen or firmware writes',async()=>{setup();try{const f=fixture({baudRefused:true});await assert.rejects(f.session.run(f.args));assert.equal(f.stats.writes,0);assert.deepEqual(f.stats.baudRates,[115200]);assert.equal(f.stats.switches,1);}finally{restore();}});

test('progress separates compressed acknowledgements from SHA verified images and tails',async()=>{setup();try{const f=fixture();await f.session.run(f.args);assert.equal(f.progress.filter(p=>p.kind==='image-verified').length,6);assert.ok(f.progress.some(p=>p.kind==='compressed-write'&&p.bytes===p.totalBytes));assert.ok(f.progress.every((p,i)=>p.bytes<=p.totalBytes&&p.elapsedMs>=0&&(!i||p.elapsedMs>=f.progress[i-1].elapsedMs)));}finally{restore();}});
