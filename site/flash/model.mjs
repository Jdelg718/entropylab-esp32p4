// Preview UI-only state. No hardware driver is imported or selected here.
const keys=['board','revision','connector','practice','risk'];
const categories=['layout','wording','eligibility'];
const freeze = state => Object.freeze({...state,
  confirmations:Object.freeze({...state.confirmations}),
  phases:Object.freeze({preflight:'blocked',transfer:'pending',readback:'pending',bootConfirmation:'pending'}),
  canConnect:false});
export function initialState(){return freeze({mode:'install',confirmations:Object.fromEntries(keys.map(k=>[k,false])),feedbackEnabled:false,feedbackCategory:'layout'});}
export function transition(state,event){
  if(event.type==='mode' && ['install','update'].includes(event.value)){
    if(event.value===state.mode)return state;
    return freeze({...state,mode:event.value,confirmations:{...state.confirmations,risk:false}});
  }
  if(event.type==='confirm' && keys.includes(event.key))return freeze({...state,confirmations:{...state.confirmations,[event.key]:event.value===true}});
  if(event.type==='feedback')return freeze({...state,feedbackEnabled:event.value===true,feedbackCategory:'layout'});
  if(event.type==='category' && categories.includes(event.value))return freeze({...state,feedbackCategory:event.value});
  return state;
}
const deny=()=>{throw new Error('HARD_BLOCKED: no hardware adapter; encryption/restricted-download proof and hardware acceptance pending');};
export const hardwareAdapter=Object.freeze({available:false,connect:deny,install:deny,update:deny});
// Allowlist projection rather than attempted secret redaction. Never accepts free text,
// user agent, device identifiers, paths, raw serial, or browser exception details.
export function feedbackPreview(state){
 if(state.feedbackEnabled!==true)return '';
 const mode=state.mode==='update'?'update':'install';
 const category=categories.includes(state.feedbackCategory)?state.feedbackCategory:'layout';
 return `EntropyLab UI feedback preview\nScreen: ${mode}\nTopic: ${category}\nHardware: unavailable; not connected\nPreflight: blocked\nTransfer: pending, not run\nReadback: pending, not run\nBoot confirmation: pending, not run\nNo submission or persistence implemented.`;
}
