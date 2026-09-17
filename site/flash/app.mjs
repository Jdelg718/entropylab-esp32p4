import { initialState, transition, feedbackPreview } from './model.mjs';
import { candidate, manifestSha256 } from './candidate.mjs';

const $=id=>document.getElementById(id);
let state=initialState();
const confirmations=Object.keys(state.confirmations);
function dispatch(event){state=transition(state,event);render();}
function line(label,value){
 const dt=document.createElement('dt');dt.textContent=label;
 const dd=document.createElement('dd');const code=document.createElement('code');code.textContent=value;dd.append(code);
 $('provenance').append(dt,dd);
}
$('candidate-status').textContent=`Private candidate — ${candidate.status}`;
line('Candidate descriptor',candidate.descriptor);
line('Source commit',candidate.source_commit);
line('Source tree',candidate.source_tree);
line('Source manifest SHA-256',candidate.source_manifest_sha256);
line('Candidate manifest SHA-256',manifestSha256);
for(const [i,image] of candidate.payloads.entries()){
 const tr=document.createElement('tr');if(i<2)tr.className='install-image';
 const name=document.createElement('td');name.textContent=image.file;
 const detail=document.createElement('td');const hash=document.createElement('code');hash.textContent=image.sha256;
 const note=document.createElement('div');note.textContent=`${image.offset} · ${image.bytes} bytes`;
 detail.append(hash,note);tr.append(name,detail);$('images').append(tr);
}
function render(){
 const update=state.mode==='update';
 for(const mode of ['install','update']){
  $(mode).setAttribute('aria-selected',String(state.mode===mode));$(mode).tabIndex=state.mode===mode?0:-1;
 }
 $('mode').setAttribute('aria-labelledby',state.mode);
 $('mode-title').textContent=update?'Application-only update':'Fresh install or full replacement';
 $('mode-copy').textContent=update?'One image: application only. Existing partition layout, bootloader, flash size, silicon revision, and app compatibility must match an approved update manifest.':'Three images: bootloader, partition table, and application. Offsets below come from the private candidate manifest, not from device validation.';
 $('mode-warning').textContent=update?'Compatibility is unverified. Update remains blocked until the installed layout and approved target are validated. Data preservation is not guaranteed. Never substitute the first-install kit or replace the bootloader for an app-only update.':'Replacement can erase existing data. Treat the selected device’s contents as expendable. This is not a secure-erasure guarantee.';
 $('risk-copy').textContent=update?'I understand updates require proven compatibility and do not guarantee data preservation.':'I understand that replacement may destroy existing device data.';
 for(const key of confirmations)$(key).checked=state.confirmations[key];
 $('confirm-status').textContent=confirmations.every(k=>state.confirmations[k])?'All local confirmations recorded. Hardware remains blocked; these are not device checks.':'Confirmations are incomplete. No device has been inspected.';
 // HTML is disabled before JS loads; every render reasserts it. No hardware handler exists.
 $('connect').disabled=true;$('connect').textContent=`Connect & ${state.mode} — hardware blocked`;
 $('download').disabled=true;$('share').disabled=true;
 document.querySelectorAll('.install-image').forEach(row=>row.hidden=update);
 $('candidate-scope').textContent=update?'Application metadata shown for inspection only. This is a first-install candidate, not an approved app-only update manifest. The candidate bootloader differs from the historically preserved installed boot; bootloader replacement is forbidden for app-only updates.':'Three-image private first-install kit. No selected device or installed-layout validation. First-install and recovery rehearsal are pending.';
 $('feedback-opt').checked=state.feedbackEnabled;
 $('feedback-topic').disabled=!state.feedbackEnabled;$('feedback-topic').value=state.feedbackCategory;
 $('feedback').disabled=!state.feedbackEnabled;$('feedback').value=feedbackPreview(state);
 const phases=Object.values(state.phases);
 document.querySelectorAll('.steps .state').forEach((el,i)=>{el.textContent=phases[i].toUpperCase();el.dataset.phase=phases[i];});
}
for(const mode of ['install','update']){
 $(mode).addEventListener('click',()=>dispatch({type:'mode',value:mode}));
 $(mode).addEventListener('keydown',event=>{
  if(['ArrowLeft','ArrowRight','Home','End'].includes(event.key)){
   event.preventDefault();const next=event.key==='Home'?'install':event.key==='End'?'update':state.mode==='install'?'update':'install';
   dispatch({type:'mode',value:next});$(next).focus();
  }
 });
}
for(const key of confirmations)$(key).addEventListener('change',e=>dispatch({type:'confirm',key,value:e.target.checked}));
$('feedback-opt').addEventListener('change',e=>dispatch({type:'feedback',value:e.target.checked}));
$('feedback-topic').addEventListener('change',e=>dispatch({type:'category',value:e.target.value}));
function environment(){
 const narrow=matchMedia('(max-width:700px)').matches;
 const mobile=/Android|iPhone|iPad|iPod|Mobile/i.test(navigator.userAgent)||(navigator.maxTouchPoints>1 && /Mac/i.test(navigator.platform));
 const unavailable=!('serial' in navigator);
 $('environment').dataset.show=String(narrow||mobile||unavailable);
 if(narrow||mobile)$('environment').textContent='Mobile installation is blocked. Review only. Mobile Safari is not supported. Hardware actions are disabled everywhere; browser and OS acceptance remain unverified.';
 else if(unavailable)$('environment').textContent='This browser does not expose WebSerial. Review only; hardware actions are disabled everywhere. Browser and OS acceptance remain unverified.';
}
window.addEventListener('resize',environment);
environment();render();
