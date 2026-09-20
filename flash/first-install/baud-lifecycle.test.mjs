import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {setImmediate as turn} from 'node:timers/promises';
import {GuardedSession, BOARD, FACTORY_PROFILE} from './adapter/adapter.mjs';
import {ESPLoader, Transport} from './vendor/esptool-js.mjs';

const assets = FACTORY_PROFILE.assets.map(p => ({offset:p.offset, bytes:new Uint8Array(readFileSync(new URL(`./firmware/${{boot:'bootloader.bin',table:'partition-table.bin',app:'entropylab.bin'}[p.role]}`, import.meta.url)))}));
const deferred = () => { let resolve; const promise = new Promise(r => { resolve=r; }); return {promise,resolve}; };
// Only the physical device/preflight are synthetic. changeBaud, command,
// readPacket, SLIP, Transport connect/disconnect/readLoop are pinned vendor code.
function fixture(mutate = p => p, closing) {
  const stats={opens:[], closes:0, writes:0, resets:0, baudCommands:0}, loaders=[];
  let controller, loader;
  const port={readable:null,writable:null,getInfo:()=>({}),async open({baudRate}) {
    assert.equal(this.readable,null); stats.opens.push(baudRate);
    this.readable=new ReadableStream({start(c){controller=c;}});
    this.writable=new WritableStream({write(wire){
      const data=[];
      for(let i=1;i<wire.length-1;i++) {if(wire[i]===0xdb) data.push(wire[++i]===0xdc?0xc0:0xdb);else data.push(wire[i]);}
      const request=new Uint8Array(data);
      assert.equal(request[1],loader.ESP_CHANGE_BAUDRATE);stats.baudCommands++;
      assert.deepEqual(Array.from(request.slice(8)),[0,8,7,0,0,194,1,0]);
      const response=mutate(new Uint8Array([1,request[1],2,0,0,0,0,0,0,0]));
      controller.enqueue(loader.transport.slipWriter(response));
    }});
  },async close(){assert.equal(this.readable?.locked??false,false);assert.equal(this.writable?.locked??false,false);stats.closes++;if(stats.closes===1&&closing){closing.entered.resolve();await closing.release.promise;}this.readable=null;this.writable=null;},async setSignals(){stats.resets++;}};
  class Loader extends ESPLoader {
    constructor(config){super(config);loader=this;loaders.push(this);}
    async connect(){await this.transport.connect();if(this.pause){await this.pause.promise;return;}this.transport.readLoop();}
    async getSecurityInfo(){return {chipId:18,flags:0,apiVersion:0,flashCryptCnt:0,parsedFlags:{SECURE_BOOT_EN:false,SECURE_DOWNLOAD_ENABLE:false}};}
    applyDetectedChip(){}
    async runStub(){this.IS_STUB=true;}
    async readFlashId(){return 0x1940ef;}
    async writeFlash(){stats.writes++;throw Error('unexpected firmware write');}
    async after(){stats.resets++;}
  }
  class P4 {IMAGE_CHIP_ID=18;async getChipRevision(){return 100;}}
  const args={port,assets,boardConfirmation:BOARD,firstInstallConsent:true};
  const create=(extra={})=>new GuardedSession({profile:FACTORY_PROFILE,Loader,Transport,ESP32P4ROM:P4,...extra});
  return {port,stats,args,create,Loader,loaders};
}

// Capture only the actual vendor's three 50ms transition waits. Releasing a
// gate runs its genuine continuation; no transition method is substituted.
async function transitionClock(body) {
  const original=globalThis.setTimeout, gates=[], waiters=[];
  globalThis.setTimeout=(fn,ms,...args)=>{
    if(ms!==50)return original(fn,ms,...args);
    const gate={release:()=>fn(...args)};const waiter=waiters.shift();if(waiter)waiter(gate);else gates.push(gate);return gate;
  };
  const next=()=>gates.length?Promise.resolve(gates.shift()):new Promise(r=>waiters.push(r));
  try {await body(next);} finally {globalThis.setTimeout=original;}
}

// Matching raw frames must be validated before vendor readPacket slices the
// header off. Every case below travels through real SLIP and readPacket.
const badFrames = {
  'declared ffff':p=>{p[2]=255;p[3]=255;return p;},
  'declared zero':p=>{p[2]=0;return p;},
  'declared one':p=>{p[2]=1;return p;},
  'declared three':p=>{p[2]=3;return p;},
  'short header':p=>p.slice(0,7),
  'header only':p=>p.slice(0,8),
  'short status':p=>{p[2]=1;return p.slice(0,9);},
  'long status':p=>{p[2]=3;return new Uint8Array([...p,0]);},
  'wrong direction':p=>{p[0]=0;return p;},
  'wrong opcode':p=>{p[1]++;return p;},
  'failure status':p=>{p[8]=1;return p;},
  'error status':p=>{p[9]=1;return p;},
};
for(const [name,mutate] of Object.entries(badFrames)) test(`raw baud ACK rejects ${name} before reopen or write`,async()=>{
  const f=fixture(mutate),states=[];
  const session=f.create({phaseTimeoutMs:600,onState:s=>states.push(s)});
  await assert.rejects(session.run(f.args),/SESSION_FAILED/);
  assert.equal(states.at(-1).errorCode,'BAUD_SWITCH_REFUSED');
  assert.deepEqual(f.stats.opens,[115200]);assert.equal(f.stats.baudCommands,1);
  assert.equal(f.stats.writes,0);assert.equal(f.stats.resets,0);
  assert.equal(f.port.readable,null);assert.equal(f.port.writable,null);
});
test('exact raw successful baud ACK permits the real transition',async()=>{
  const f=fixture();
  assert.equal((await f.create().checkDevice(f.args)).ok,true);
  assert.deepEqual(f.stats.opens,[115200,460800]);assert.equal(f.stats.baudCommands,1);
  assert.equal(f.stats.writes,0);assert.equal(f.stats.resets,0);
  assert.equal(f.port.readable,null);assert.equal(f.port.writable,null);
});

test('cleanup retains ownership until an already-started vendor close settles',async()=>{
  const closing={entered:deferred(),release:deferred()},f=fixture(undefined,closing);
  const session=f.create();const outcome=assert.rejects(session.run(f.args),/SESSION_FAILED/);
  await closing.entered.promise;
  let cleaned=false;const cleanup=session.cancel().then(()=>{cleaned=true;});
  for(let i=0;i<10;i++)await turn();
  try {assert.equal(cleaned,false,'cleanup must not release a port while its earlier close is pending');}
  finally {closing.release.resolve();await cleanup;await outcome;}
  assert.equal(f.stats.writes,0);assert.equal(f.stats.resets,0);
});

for(const interruption of ['cancel','deadline']) for(const delay of [1,2,3]) test(`${interruption} after ACK at vendor delay ${delay}: successor remains untouched`,async()=>{
  await transitionClock(async next=>{
    const f=fixture(), states=[], errors=[];
    const onUnhandled=e=>errors.push(e);process.on('unhandledRejection',onUnhandled);
    try {
      const session=f.create({phaseTimeoutMs:interruption==='deadline'?300:5000,onState:s=>states.push(s)});
      const outcome=assert.rejects(session.run(f.args),/SESSION_FAILED/);
      let gate;
      for(let i=1;i<=delay;i++){gate=await next();if(i<delay)gate.release();}
      assert.equal(f.stats.baudCommands,1);
      if(interruption==='cancel')await session.cancel();
      await outcome;
      assert.equal(states.at(-1).errorCode,interruption==='cancel'?'SESSION_CANCELLED':'OPERATION_TIMEOUT');
      assert.equal(states.at(-1).cleanupSuccess,true);
      // Start a real successor session, hold it in connect with live stream locks.
      const paused=deferred(), entered=deferred();
      const connect=f.Loader.prototype.connect;
      f.Loader.prototype.connect=async function(){this.pause=paused;entered.resolve();return connect.call(this);};
      const successor=f.create();const successorOutcome=assert.rejects(successor.run(f.args),/SESSION_FAILED/);
      await entered.promise;await turn();
      const readable=f.port.readable,writable=f.port.writable,closes=f.stats.closes,opens=f.stats.opens.length;
      gate.release();
      // Event-loop turns settle the released vendor continuation and rejection handlers.
      for(let i=0;i<10;i++)await turn();
      try {
        assert.equal(f.stats.closes,closes,'old transition must not close successor');
        assert.equal(f.stats.opens.length,opens,'old transition must not reopen');
        assert.equal(f.port.readable,readable);assert.equal(f.port.writable,writable);
        assert.equal(readable.locked,false,'old readLoop must not acquire successor stream');
        assert.equal(f.stats.writes,0);assert.equal(f.stats.resets,0);assert.deepEqual(errors,[]);
      } finally {
        // Also clean up when deliberately running RED against the old adapter.
        for(const l of f.loaders)try{await l.transport.reader?.cancel();}catch{}
        await successor.cancel();paused.resolve();await successorOutcome;
      }
    } finally {process.removeListener('unhandledRejection',onUnhandled);}
  });
});
