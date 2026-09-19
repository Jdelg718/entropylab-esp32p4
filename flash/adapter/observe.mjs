// Local review-only instrumentation. No logging, identifiers, messages, or bytes.
// Does not alter reset policy, retry counts, command payloads, or security gates.
export function observationClasses({Transport,ESPLoader,ESPError}) {
 const events=[];
 const names=new Set(['Error','TypeError','NetworkError','InvalidStateError','NotSupportedError','SecurityError','NotAllowedError','AbortError','BufferOverrunError','FramingError','BreakError','ParityError']);
 function record(stage,status,error){
  let errorName='UnknownError';
  if(error instanceof ESPError)errorName='ESPError'; // shipped minification names it A
  else if(names.has(error?.name))errorName=error.name;
  const codes={open:'PORT_OPEN_FAILED',reset:'RESET_SIGNAL_FAILED',read:'SERIAL_READ_FAILED',write:'SERIAL_WRITE_FAILED',sync:'ROM_SYNC_FAILED'};
  if(events.length===64)events.shift();
  events.push(Object.freeze({stage,status,...(status==='failed'?{errorName,failureCode:codes[stage]}:{})}));
 }
 async function watch(stage,fn){record(stage,'started');try{const r=await fn();record(stage,'ok');return r;}catch(e){record(stage,'failed',e);throw e;}}
 function wrapPort(port){return new Proxy(port,{get(target,key){
  if(key==='open')return (...a)=>watch('open',()=>target.open(...a));
  if(key==='setSignals')return (...a)=>watch('reset',()=>target.setSignals(...a));
  if(key==='readable'){
   const stream=Reflect.get(target,key,target);if(!stream)return stream;
   return new Proxy(stream,{get(s,k){if(k==='getReader')return (...args)=>{const reader=s.getReader(...args);return new Proxy(reader,{get(r,p){if(p==='read')return (...a)=>watch('read',()=>r.read(...a));const v=Reflect.get(r,p,r);return typeof v==='function'?v.bind(r):v;}});};const v=Reflect.get(s,k,s);return typeof v==='function'?v.bind(s):v;}});
  }
  const value=Reflect.get(target,key,target);return typeof value==='function'?value.bind(target):value;
 }});}
 class ObservedTransport extends Transport{constructor(port,...rest){super(wrapPort(port),...rest);this.trace=()=>{};}}
 class ObservedLoader extends ESPLoader{async sync(...a){return watch('sync',()=>super.sync(...a));}}
 return {Transport:ObservedTransport,ESPLoader:ObservedLoader,snapshot:()=>Object.freeze([...events])};
}
