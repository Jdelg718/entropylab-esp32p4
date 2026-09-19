import {GuardedSession,BOARD,FACTORY_PROFILE} from './adapter/adapter.mjs';
import {ESPLoader,Transport} from './vendor/esptool-js.mjs';
const assert={equal(a,b){if(a!==b)throw Error(`expected ${b}, got ${a}`)},deepEqual(a,b){if(JSON.stringify(a)!==JSON.stringify(b))throw Error('array mismatch')}};
const load=async n=>new Uint8Array(await(await fetch('./firmware/'+n)).arrayBuffer());
const [appBytes,awaitBoot,awaitTable]=await Promise.all(['entropylab.bin','bootloader.bin','partition-table.bin'].map(load));
const u32=(b,n=0)=>new DataView(b.buffer,b.byteOffset,b.byteLength).getUint32(n,true);
function fixture(options={}) {
  const bytes = {boot:new Uint8Array(awaitBoot),table:new Uint8Array(awaitTable),app:new Uint8Array(appBytes)};
  const pins = Object.entries(bytes).map(([role,b])=>({role,offset:{boot:0x2000,table:0x8000,app:0x10000}[role],length:b.length,sha256:'unused'}));
  const profile=FACTORY_PROFILE;
  const memory=new Map();
  // Public destructive policy deliberately has no private predecessor fixtures.
  const stats={opens:0,closes:0,writes:0,resets:0,stub:0,security:0,readbacks:0,aborts:0};
  let loader, rejectWrite;
  let controller;
  const port={readable:null,writable:null,getInfo:()=>({}),async open(serial){
    stats.baudRates??=[];stats.baudRates.push(serial.baudRate);stats.opens++;
    if(options.reopenFails&&stats.opens===2)throw Error('reopen');
    this.readable=new ReadableStream({start(c){controller=c;}});
    this.writable=new WritableStream({async write(wire){
      const a=[];for(let i=1;i<wire.length-1;i++){if(wire[i]===0xdb){i++;a.push(wire[i]===0xdc?0xc0:0xdb);}else a.push(wire[i]);}
      const packet=new Uint8Array(a);if(packet.length===4)return;
      if(options.noAck&&packet[1]===loader.ESP_CHANGE_BAUDRATE)return;
      if(options.disconnect&&packet[1]===loader.ESP_CHANGE_BAUDRATE){controller.error(new DOMException('disconnected'));return;}
      const [value,data]=await loader.emulateCommand(packet[1],packet.slice(8));
      const response=new Uint8Array(8+data.length);response[0]=1;response[1]=packet[1];new DataView(response.buffer).setUint16(2,data.length,true);new DataView(response.buffer).setUint32(4,value,true);response.set(data,8);
      controller.enqueue(loader.transport.slipWriter(response));
      for(const p of loader.transport.queue.splice(0))controller.enqueue(loader.transport.slipWriter(p));
    }});
  },async close(){stats.closes++;if(this.readable?.locked||this.writable?.locked)throw Error('locked');this.readable=null;this.writable=null;},async setSignals(){stats.resets++;}};
  class WireTransport extends Transport {queue=[];}
  class WireLoader extends ESPLoader {
    constructor(config){super(config);loader=this;if(options.unsupported)this.changeBaud=undefined;}
    async openAndSync(mode,attempts){assert.equal(mode,'default_reset');assert.equal(attempts,1);await this.transport.connect();this.transport.readLoop();this.syncStubDetected=!!options.existingStub;}
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
      else if(op===this.ESP_FLASH_DEFL_END){assert.equal(u32(data),1);const written=new Uint8Array(await new Response(new Blob(this.parts).stream().pipeThrough(new DecompressionStream("deflate"))).arrayBuffer());memory.set(this.address,written);const pin=profile.assets.find(p=>p.offset===this.address);assert.equal(Math.ceil(written.length/4096)*4096+this.address,pin.eraseEnd);memory.set(this.address+written.length,new Uint8Array(pin.eraseEnd-this.address-written.length).fill(255));}
      else if(op===this.ESP_READ_FLASH){stats.readbacks++;let b=memory.get(u32(data)).slice(0,u32(data,4));if(options.mismatch===u32(data) && stats.writes) b[0]++;if(options.short) b=new Uint8Array(b.length+1);for(let i=0;i<b.length;i+=4096)this.transport.queue.push(b.slice(i,i+4096));this.transport.queue.push(new Uint8Array(16));}
      return [value,out];
    }
  }
  const states=[],progress=[];
  const session=new GuardedSession({profile,Loader:WireLoader,Transport:WireTransport,onProgress:p=>progress.push(p),onState:s=>{states.push(s.state);options.onState?.(s,session);if(s.state===options.stallPhase)clock.advance(options.stallMs);},...(options.timeout?{phaseTimeoutMs:options.timeout}:{})});
  const args={port,assets:profile.assets.map(p=>({offset:p.offset,bytes:bytes[p.role]})),boardConfirmation:BOARD,firstInstallConsent:true,resetOnSuccess:false};
  return {session,args,stats,states,bytes,profile,progress};
}
export async function run(){const results=[];
for(const [name,opts] of Object.entries({success:{},refused:{baudRefused:true},unsupported:{unsupported:true},reopen:{reopenFails:true},timeout:{noAck:true,timeout:350},cancel:{onState:(s,session)=>{if(s.state==='changing-baud')session.cancel();}},disconnect:{disconnect:true,timeout:400}})){
 const f=fixture(opts);let ok=false;try{ok=(await f.session.run(f.args)).verified;}catch{}
 if(name==='success'){assert.equal(ok,true);assert.equal(f.stats.writes,3);assert.equal(f.stats.readbacks,6);assert.deepEqual(f.stats.baudRates,[115200,460800]);assert.equal(f.progress.filter(p=>p.kind==='image-verified').length,6);}
 else{assert.equal(ok,false);assert.equal(f.stats.writes,0);}
 assert.equal(f.args.port.readable,null);assert.equal(f.args.port.writable,null);assert.equal(f.stats.resets,0);results.push({name,ok,...f.stats});
}return results;}
