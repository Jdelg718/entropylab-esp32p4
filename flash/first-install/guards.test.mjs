import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {GuardedSession,BOARD,FACTORY_PROFILE} from './adapter/adapter.mjs';
const names=['bootloader.bin','partition-table.bin','entropylab.bin'];
const assets=await Promise.all(FACTORY_PROFILE.assets.map(async(p,i)=>({offset:p.offset,bytes:new Uint8Array(await readFile(new URL('./firmware/'+names[i],import.meta.url)))})));
function fixture(opts={}) {
 const stats={writes:0,opens:0};let current;
 const port={writable:null,async close(){this.writable=null;}};
 class Transport {constructor(p){this.port=p;}trace(){}setDeviceLostCallback(){}async read(){const b=new Uint8Array(32);b[0]=1;b[1]=0x14;b[2]=24;return b;}}
 class P4 {IMAGE_CHIP_ID=18;async getChipRevision(){return opts.rev??100;}}
 class Loader {
 constructor({transport}){this.transport=transport;}
 async connect(){stats.opens++;}
 async getSecurityInfo(){await this.transport.read();return {chipId:opts.chip??18,flags:opts.flags??0,apiVersion:opts.eco??2,parsedFlags:{SECURE_BOOT_EN:opts.secure??false,SECURE_DOWNLOAD_ENABLE:false},flashCryptCnt:opts.crypt??0};}
 async changeBaud(){this.transport.baudrate=460800;}applyDetectedChip(){}async runStub(){this.IS_STUB=true;}async readFlashId(){return opts.jedec??0x1940ef;}
 async writeFlash(o){assert.equal(o.eraseAll,false);assert.equal(o.flashSize,'keep');assert.equal(o.flashMode,'keep');assert.equal(o.flashFreq,'keep');assert.equal(o.fileArray.length,3);stats.writes++;}
 async readFlash(offset,length){current=assets.find(a=>a.offset===offset);this.transport.read=async()=>new Uint8Array(16);return current?current.bytes:new Uint8Array(length).fill(255);}
 }
 const session=new GuardedSession({profile:FACTORY_PROFILE,Loader,Transport,ESP32P4ROM:P4});
 return {stats,session,args:{port,assets,boardConfirmation:BOARD,firstInstallConsent:true}};
}
test('exact shipped images write only fixed regions and verify image plus FF tails',async()=>{const f=fixture();assert.equal((await f.session.run(f.args)).verified,true);assert.equal(f.stats.writes,1);});
for(const [name,opts] of Object.entries({chip:{chip:9},revision:{rev:200},eco:{eco:5},flags:{flags:0x800},secure:{secure:true},encryption:{crypt:2},size:{jedec:0x1840ef}}))test('refuse '+name,async()=>{const f=fixture(opts);await assert.rejects(f.session.run(f.args));assert.equal(f.stats.writes,0);});
test('consent required before opening',async()=>{const f=fixture();f.args.firstInstallConsent=false;await assert.rejects(f.session.run(f.args));assert.equal(f.stats.opens,0);});
test('board confirmation required before opening',async()=>{const f=fixture();f.args.boardConfirmation='unknown';await assert.rejects(f.session.run(f.args));assert.equal(f.stats.opens,0);});
