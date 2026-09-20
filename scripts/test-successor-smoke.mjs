import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash,webcrypto} from 'node:crypto';
import {IMAGES,downloadImages} from '../flash/first-install/assets.mjs';
import {FACTORY_PROFILE,GuardedSession,BOARD} from '../flash/first-install/adapter/adapter.mjs';
globalThis.crypto ??= webcrypto;
const root=new URL('../',import.meta.url);
const manifest=JSON.parse(await readFile(new URL('provenance.json',root)));
const data=await Promise.all(manifest.images.map(i=>readFile(new URL(i.path,root))));
test('candidate02 manifest, download and adapter pins all bind exact bytes',()=>{
 assert.equal(manifest.package_version,'education-candidate02-test-02');
 assert.equal(manifest.source_revision,'2b919dc73c9cbcbb5e845850650d71c6782ad68b');
 for(let i=0;i<data.length;i++){
  const p=manifest.images[i],digest=createHash('sha256').update(data[i]).digest('hex');
  assert.equal(digest,p.sha256);assert.equal(IMAGES[i].sha256,digest);assert.equal(FACTORY_PROFILE.assets[i].sha256,digest);
  assert.equal(p.bytes,data[i].length);assert.equal(IMAGES[i].size,data[i].length);assert.equal(FACTORY_PROFILE.assets[i].length,data[i].length);
 }
});
test('real downloader verifies fresh bytes and refuses modified application',async()=>{
 const oldFetch=globalThis.fetch,oldLocation=globalThis.location;
 globalThis.location={origin:'null'};let corrupt=false;
 globalThis.fetch=async url=>{
  const i=IMAGES.findIndex(p=>url.pathname.endsWith(p.url.slice(1)));
  const bytes=Uint8Array.from(data[i]);if(corrupt&&i===2)bytes[40]^=1;
  return new Response(bytes,{headers:{'content-length':String(bytes.length)}});
 };
 try{assert.equal((await downloadImages(undefined)).length,3);corrupt=true;await assert.rejects(()=>downloadImages(undefined),/hash/);}
 finally{globalThis.fetch=oldFetch;globalThis.location=oldLocation;}
});
test('package refuses both hardware entry points before transport construction',async()=>{
 let constructed=0;class NeverTransport{constructor(){constructed++;throw Error('must not construct')}}
 for(const method of ['run','checkDevice']){
  const states=[];const session=new GuardedSession({profile:FACTORY_PROFILE,Transport:NeverTransport,Loader:class{},onState:e=>states.push(e)});
  await assert.rejects(()=>session[method]({port:{},assets:data.map((b,i)=>({bytes:new Uint8Array(b),offset:IMAGES[i].offset})),boardConfirmation:BOARD,firstInstallConsent:true}),/SESSION_FAILED/);
  assert.equal(states.at(-1).errorCode,'SUCCESSOR_TEST_HOLD');assert.equal(states.at(-1).firmwareWriteEntered,false);
 }
 assert.equal(constructed,0);assert.equal(FACTORY_PROFILE.writeHold,true);
});
