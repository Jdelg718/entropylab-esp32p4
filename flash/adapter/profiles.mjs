import { BOARD, GuardedSession } from './adapter.mjs';
const app=Object.freeze({role:'app',fileKey:'app',offset:0x10000,length:1539952,sha256:'a02a2192b5c302390a916f9ee67961b699e5f2f9038135378eed0b19d792f33b'});
const boot=Object.freeze({role:'boot',offset:0x2000,length:21264,sha256:'268991f79279b07be7c7ea56f671a1dbd0730b53c4b97a2e6081028496f1b2d3'});
const table=Object.freeze({role:'table',offset:0x8000,length:3072,sha256:'d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb'});
export const PROFILE=Object.freeze({version:'d65d730162bc966e98c45868cd-r1',board:BOARD,appupdate:Object.freeze({board:BOARD,mode:'appupdate',assets:Object.freeze([app]),compatibility:Object.freeze([boot,table]),predecessor:app})});
export function createSession({mode,ESPLoader,Transport,ESP32P4ROM,onState}) {
 if(mode!=='appupdate')throw new Error('MODE_REFUSED');
 const session=new GuardedSession({profile:PROFILE.appupdate,Loader:ESPLoader,Transport,ESP32P4ROM,onState});
 // Explicit local-only HOLD. Removing this requires independent release review.
 return Object.freeze({run:async()=>{throw new Error('RELEASE_HOLD');},checkDevice:args=>session.checkDevice(args),cancel:()=>session.cancel()});
}
export { BOARD, availability } from './adapter.mjs';
