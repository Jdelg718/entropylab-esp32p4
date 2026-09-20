"""Build browser wire harness from public Node fixture; no private inputs."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'flash/first-install/wire.test.mjs').read_text();s=s[s.index('function fixture('):s.index('\nconst original=')]
s=s.replace('sha(b)',"'unused'")
a=s.index('  const port=');b=s.index('  class WireLoader',a)
s=s[:a]+'''  let controller;
  const port={readable:null,writable:null,getInfo:()=>({}),async open(serial){
    stats.baudRates??=[];stats.baudRates.push(serial.baudRate);stats.opens++;
    if(options.reopenFails&&stats.opens===2)throw Error('reopen');
    this.readable=new ReadableStream({start(c){controller=c;}});
    this.writable=new WritableStream({async write(wire){
      const a=[];for(let i=1;i<wire.length-1;i++){if(wire[i]===0xdb){i++;a.push(wire[i]===0xdc?0xc0:0xdb);}else a.push(wire[i]);}
      const packet=new Uint8Array(a);if(packet.length===4)return;
      const target=options.faultAt==='write'?loader.ESP_FLASH_DEFL_DATA:options.faultAt==='read'?loader.ESP_READ_FLASH:loader.ESP_CHANGE_BAUDRATE;
      if(packet[1]===target){
        stats.targetOpcodes=(stats.targetOpcodes||0)+1;
        if(options.fault && (options.faultAt==='baud'||stats.targetOpcodes===2)){
          stats.faultReached=true;
          if(options.fault==='cancel'){setTimeout(()=>session.cancel(),0);return;}
          if(options.fault==='disconnect'){controller.error(new DOMException('disconnected'));return;}
          if(options.fault==='stall')return;
        }
      }
      const [value,data]=await loader.emulateCommand(packet[1],packet.slice(8));
      const response=new Uint8Array(8+data.length);response[0]=1;response[1]=packet[1];new DataView(response.buffer).setUint16(2,data.length,true);new DataView(response.buffer).setUint32(4,value,true);response.set(data,8);
      if(options.badHeader&&packet[1]===loader.ESP_CHANGE_BAUDRATE)new DataView(response.buffer).setUint16(2,0xffff,true);
      controller.enqueue(loader.transport.slipWriter(response));
      if(options.cancelAfterAck&&packet[1]===loader.ESP_CHANGE_BAUDRATE)setTimeout(()=>session.cancel(),10);
      for(const p of loader.transport.queue.splice(0))controller.enqueue(loader.transport.slipWriter(p));
    }});
  },async close(){stats.closes++;if(this.readable?.locked||this.writable?.locked)throw Error('locked');this.readable=null;this.writable=null;},async setSignals(){stats.resets++;}};
  class WireTransport extends Transport {queue=[];async read(timeout){return super.read(stats.faultReached&&options.fault==='stall'?150:timeout);}}
'''+s[b:]
s=s.replace('constructor(options){super(options);loader=this;}', 'constructor(config){super(config);loader=this;if(options.unsupported)this.changeBaud=undefined;}').replace('if(options.baudRefused)out[0]=1;', 'if(options.baudRefused)out[0]=1;if(options.malformed)out=new Uint8Array(1);')
s=s.replace('await this.transport.connect();','await this.transport.connect();this.transport.readLoop();').replace('new Uint8Array(inflateSync(Buffer.concat(this.parts)))','new Uint8Array(await new Response(new Blob(this.parts).stream().pipeThrough(new DecompressionStream("deflate"))).arrayBuffer())').replace("new Uint8Array(createHash('md5').update(b).digest())",'new Uint8Array(16)')
prefix='''import {GuardedSession,BOARD,FACTORY_PROFILE} from './adapter/adapter.mjs';
import {ESPLoader,Transport} from './vendor/esptool-js.mjs';
const assert={equal(a,b){if(a!==b)throw Error(`expected ${b}, got ${a}`)},deepEqual(a,b){if(JSON.stringify(a)!==JSON.stringify(b))throw Error('array mismatch')}};
const load=async n=>new Uint8Array(await(await fetch('./firmware/'+n)).arrayBuffer());
const [appBytes,awaitBoot,awaitTable]=await Promise.all(['entropylab.bin','bootloader.bin','partition-table.bin'].map(load));
const u32=(b,n=0)=>new DataView(b.buffer,b.byteOffset,b.byteLength).getUint32(n,true);
'''
suffix='''export async function run(){const results=[];
const scenarios={success:{},refused:{baudRefused:true},malformed:{malformed:true},'malformed-header':{badHeader:true},'cancel-after-ack':{cancelAfterAck:true},unsupported:{unsupported:true},reopen:{reopenFails:true}};
for(const faultAt of ['baud','write','read'])for(const fault of ['stall','cancel','disconnect'])scenarios[faultAt+'-'+fault]={faultAt,fault};
for(const [name,opts] of Object.entries(scenarios)){
 const f=fixture(opts);let ok=false;try{ok=(await f.session.run(f.args)).verified;}catch{}
 if(name==='success'){assert.equal(ok,true);assert.equal(f.stats.writes,3);assert.equal(f.stats.readbacks,6);assert.deepEqual(f.stats.baudRates,[115200,460800]);assert.equal(f.progress.filter(p=>p.kind==='image-verified').length,6);}
 else{
  assert.equal(ok,false);assert.equal(f.states.includes('complete'),false);
  const phase=opts.faultAt==='write'?'writing':opts.faultAt==='read'?'verifying-readback':'changing-baud';
  assert.equal(f.states.includes(phase),true);
  if(opts.fault){assert.equal(f.stats.faultReached,true);assert.equal(f.stats.targetOpcodes,opts.faultAt==='baud'?1:2);}
  if(opts.faultAt==='write'){assert.equal(f.stats.blocks,1);assert.equal(f.progress.some(p=>p.kind==='compressed-write'&&p.bytes>0),true);}
  else if(opts.faultAt==='read'){assert.equal(f.stats.writes,3);assert.equal(f.progress.filter(p=>p.kind==='image-verified').length,1);}
  else assert.equal(f.stats.writes,0);
  if(name==='refused'||name==='malformed'||opts.badHeader||opts.cancelAfterAck){assert.equal(f.stats.switches,1);assert.deepEqual(f.stats.baudRates,[115200]);}
  if(name==='unsupported'){assert.equal(f.stats.switches||0,0);assert.deepEqual(f.stats.baudRates,[115200]);}
  if(name==='reopen'){assert.equal(f.stats.switches,1);assert.deepEqual(f.stats.baudRates,[115200,460800]);}
 }
 assert.equal(f.args.port.readable,null);assert.equal(f.args.port.writable,null);assert.equal(f.stats.resets,0);
 if(opts.cancelAfterAck){
  await f.args.port.open({baudRate:115200});
  const readable=f.args.port.readable,writable=f.args.port.writable,closes=f.stats.closes;
  await new Promise(resolve=>setTimeout(resolve,200));
  assert.equal(f.args.port.readable,readable);assert.equal(f.args.port.writable,writable);
  assert.equal(readable.locked,false);assert.equal(writable.locked,false);assert.equal(f.stats.closes,closes);
  await f.args.port.close();f.stats.successorUnaffected=true;
 }
 results.push({name,ok,...f.stats,states:f.states,verifiedSpans:f.progress.filter(p=>p.kind==='image-verified').length});
}return results;}
'''
(r/'flash/first-install/speed-browser.mjs').write_text(prefix+s+suffix)
