import {BOARD, GuardedSession, FACTORY_PROFILE} from './adapter.mjs';
export function createSession({mode,ESPLoader,Transport,ESP32P4ROM,onState}) {
 if(mode!=='publicfirstinstall')throw new Error('MODE_REFUSED');
 const session=new GuardedSession({profile:FACTORY_PROFILE,Loader:ESPLoader,Transport,ESP32P4ROM,onState});
 return Object.freeze({run:args=>session.run(args),checkDevice:args=>session.checkDevice(args),cancel:()=>session.cancel()});
}
export {BOARD, FACTORY_PROFILE};
export {availability} from './adapter.mjs';
