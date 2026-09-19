import {IMAGES,downloadImages} from './assets.mjs';
import {observationClasses} from './adapter/observe.mjs';
let observation;
function observationSummary(){
 const events=observation?.snapshot()||[];
 const describe=(stage,label)=>{const seen=events.filter(e=>e.stage===stage);const failed=seen.find(e=>e.status==='failed');return `${label}: ${failed?(stage==='sync'?'failed':failed.errorName):stage==='read'?'no error observed':seen.some(e=>e.status==='ok')?'OK':'not observed'}`;};
 return ' Observation: '+[['open','port open'],['reset','reset signals'],['read','serial read'],['sync','ROM sync']].map(([stage,label])=>describe(stage,label)).join('; ')+'. Bounded observations only; no read error does not establish absence of bytes or ROM mode.';
}
const $=id=>document.getElementById(id);
let mode='appupdate',busy=false,terminal=false,ready=false,connected=false,verified=false,session,modules,selectedPort,assets;
let safeOutcome=false;
const abort=new AbortController();
const confirmations=['board','revision','connector','practice','risk'];
const eligible=()=>confirmations.every(id=>$(id).checked);
const mobile=/Android|iPhone|iPad|iPod/i.test(navigator.userAgent)||(navigator.platform==='MacIntel'&&navigator.maxTouchPoints>1);
const supported=isSecureContext&&!!navigator.serial?.requestPort&&!mobile;
function status(text){$('gate-status').textContent=text;}
function render(){
 $('prepare').disabled=!modules||!supported||busy||terminal||ready;
 $('connect').disabled=!ready||!eligible()||busy||terminal||connected;
 $('flash').disabled=true||!connected||!eligible()||!$('destructive').checked||busy||terminal;
 $('check-device').disabled=!connected||!eligible()||busy||terminal;
 $('destructive').disabled=!connected||busy||terminal;
 $('cancel').disabled=terminal||!modules;
 for(const id of ['install','update',...confirmations])$(id).disabled=busy||connected||terminal;
}
const labels={
 'verifying-assets':'Verifying pinned image bytes', 'prepared':'Images verified. Confirm target, then Connect to choose a port.',
 'connecting-rom':'Connecting to ROM', 'security-preflight':'Checking chip, revision and security restrictions',
 'official-stub':'Loading reviewed upstream stub', 'jedec-preflight':'Checking physical flash capacity',
 'compatibility-readback':'Checking installed bootloader and partition compatibility',
 'connected':'Preflight passed. Review destructive consent, then use the separate Install button.',
 'ready-to-flash':'Preflight passed. Review destructive consent, then use the separate Install button.',
 'writing':'Writing selected images — do not disconnect', 'verifying-readback':'Verifying device readback',
 'resetting-verified':'Resetting verified image', 'complete':'Write and verification complete. Boot is NOT yet confirmed.',
 'failed-disconnected':'Attempt ended. Reload for a new attempt. Device contents may have changed.',
 'failed-cleanup-incomplete':'Cleanup incomplete. Unplug the device before reloading. Device contents may have changed.'
};
function securitySummary(d){
 if(!d || typeof d!=='object')return '';
 const pick=(value,allowed)=>allowed.includes(value)?value:'unknown';
 const reasons=['CHIP_REFUSED','FLAGS_INVALID','FLAGS_UNKNOWN_BITS','API_VERSION_REFUSED','SECURE_BOOT_NOT_FALSE','SECURE_DOWNLOAD_NOT_FALSE','FLASH_CRYPT_COUNT_NOT_ZERO','SECURITY_CHECKS_CLEAR'];
 const present=value=>value===true?'present':value===false?'absent':'unknown';
 const fields=[['flags','Flags','flagsPresent',['invalid','known-bits-only','unknown-bits']],['apiVersion','API version','apiVersionPresent',['zero','nonzero','invalid']],['secureBoot','Secure boot','secureBootPresent',['false','true','invalid']],['secureDownload','Secure download','secureDownloadPresent',['false','true','invalid']],['flashCryptCnt','Flash crypt count','flashCryptCntPresent',['zero','nonzero','invalid']]];
 return ` Security diagnostic: ${pick(d.reason,reasons)}; chip matches: ${d.chipIdMatches===true?'yes':d.chipIdMatches===false?'no':'unknown'}; `+fields.map(([key,label,has,values])=>`${label}: ${pick(d[key],values)} (${present(d[has])})`).join('; ')+'. Diagnostic categories only; policy unchanged.';
}
function onState(event){
 const state=typeof event==='string'?event:event.state;
 status(labels[state]||'Adapter operation in progress');
 document.querySelector('.offline').textContent=labels[state]||'Device operation in progress';
 const steps=document.querySelectorAll('.steps .state');
 if(state==='diagnostic-complete'){
  safeOutcome=true;terminal=true;steps[0].textContent='PASSED';steps[1].textContent='NOT STARTED';
  status('Diagnostic passed — no firmware write. Reset requested and port closed. Confirm the app on-device. Reload for a new explicit write attempt.');
 }
 if(state?.startsWith('failed') && event.phase){
  safeOutcome=true;
  steps[0].textContent=event.firmwareWriteEntered?'PASSED':'FAILED';
  steps[1].textContent=event.firmwareWriteEntered?'UNCONFIRMED':'NOT STARTED';
  if(event.firmwareWriteEntered)steps[2].textContent='UNCONFIRMED';
  status(`Stopped at ${event.phase} (${event.phaseElapsedMs} ms): ${event.errorCode} / ${event.errorClass}. Firmware write phase entered: ${event.firmwareWriteEntered?'YES — contents may have changed':'NO'}. Cleanup: ${event.cleanupSuccess?'closed':'INCOMPLETE — unplug before reloading'}. ${!event.firmwareWriteEntered&&event.cleanupSuccess?'Press board Reset to return to the existing app. ':''}Reload for a new explicit attempt; no automatic retry.`);
 }
 if(state?.startsWith('failed') && event.phase==='security-preflight')status($('gate-status').textContent+securitySummary(event.securityDiagnostic));
 if(state==='diagnostic-complete'||state?.startsWith('failed'))status($('gate-status').textContent+observationSummary());
 if(['connecting-rom','security-preflight','official-stub','jedec-preflight','compatibility-readback'].includes(state))steps[0].textContent='RUNNING';
 if(['connected','ready-to-flash'].includes(state))steps[0].textContent='PASSED';
 if(state==='writing'){steps[0].textContent='PASSED';steps[1].textContent='RUNNING';}
 if(state==='verifying-readback'){steps[1].textContent='WRITTEN';steps[2].textContent='RUNNING';}
 if(state==='complete'){verified=true;terminal=true;steps[2].textContent='VERIFIED';steps[3].textContent='MANUAL';$('boot-label').hidden=false;}
 if(state?.startsWith('failed'))terminal=true;
 render();
}
function onProgress(event){
 const value=typeof event==='number'?event:event?.percent;
 if(!Number.isFinite(value)||value<0||value>100)return;
 $('progress').hidden=false;$('progress').value=value;
}
async function fail(message){terminal=true;abort.abort();try{if(await session?.cancel()===false)message+=' Cleanup incomplete: unplug device before reloading.';}catch{message+=' Unplug device: cleanup could not be confirmed.';}status(message);busy=false;render();}
for(const image of IMAGES){const row=document.createElement('tr');for(const text of [image.name,`${image.sha256} / 0x${image.offset.toString(16)} / ${image.size}`]){const cell=document.createElement('td');cell.textContent=text;row.append(cell);}$('images').append(row);}
function choose(next){if(next!=='appupdate')return;if(busy||connected||terminal)return;mode=next;ready=false;session=undefined;$('destructive').checked=false;for(const [id,value] of [['install','firstinstall'],['update','appupdate']])$(id).setAttribute('aria-selected',String(next===value));$('mode').setAttribute('aria-labelledby',next==='firstinstall'?'install':'update');$('mode-title').textContent=next==='firstinstall'?'Fresh install or full replacement':'Application-only update';$('mode-copy').textContent=next==='firstinstall'?'Writes bootloader, partition table and app. Factory-install acceptance remains pending.':'Writes app only. Requires an adapter-allowlisted installed bootloader and pinned partition table; mismatch blocks writing. This is not a generic updater.';$('flash').textContent=next==='firstinstall'?'Install selected images':'Update application only';status('Download and verify images for the selected mode.');render();}
$('install').onclick=()=>choose('firstinstall');$('update').onclick=()=>choose('appupdate');
for(const id of [...confirmations,'destructive'])$(id).onchange=render;
$('prepare').onclick=async()=>{if(busy||terminal||ready||!modules)return;busy=true;render();status('Downloading bounded same-origin images and checking SHA-256…');try{
 assets=await downloadImages(abort.signal,mode);
 if(terminal)return;
 observation=observationClasses(modules);
 session=modules.createSession({ESPLoader:observation.ESPLoader,Transport:observation.Transport,mode,onState});
 ready=true;status('Pinned images verified. Connect selects a port only; the separate Install button starts guarded preflight and writing.');
 }catch{await fail('Image verification or preparation failed. Nothing can be connected from this attempt. Reload to retry.');}finally{busy=false;render();}};
$('connect').onclick=async()=>{if($('connect').disabled||busy)return;busy=true;render();try{
 // requestPort MUST remain before the first await and inside this click gesture.
 const selected=navigator.serial.requestPort();
 const port=await selected;
 if(terminal){try{await port.close();}catch{}return;}
 selectedPort=port;connected=true;status('Port selected, not opened or preflighted. Authorize the selected write, then Install to run guarded preflight and flashing.');
 }catch{await fail('Connection cancelled or preflight refused. Reload for a new explicit attempt; no automatic retry.');}finally{busy=false;render();}};
$('flash').onclick=async()=>{if($('flash').disabled||busy)return;busy=true;render();try{await session.run({port:selectedPort,assets,boardConfirmation:modules.BOARD,firstInstallConsent:mode==='firstinstall',resetOnSuccess:true});if(!verified&&!terminal)await fail('Adapter did not report verified completion. Do not assume the device is unchanged.');}catch{if(!safeOutcome)await fail('Write attempt ended without confirmed completion. Device contents may have changed; consult recovery documentation.');}finally{busy=false;render();}};
$('check-device').onclick=async()=>{if($('check-device').disabled||busy)return;busy=true;render();try{await session.checkDevice({port:selectedPort,assets,boardConfirmation:modules.BOARD,resetOnSuccess:true});}catch{if(!safeOutcome)await fail('Diagnostic failed without a confirmed safe outcome. Unplug before reloading.');}finally{busy=false;render();}};
$('cancel').onclick=async()=>{if(terminal)return;terminal=true;busy=true;abort.abort();render();try{const result=await session?.cancel();status(result===false?'Cleanup incomplete. Unplug device before reloading.':'Attempt cancelled. Reload for a new explicit attempt. Cancellation does not guarantee unchanged device data.');}catch{status('Cleanup incomplete. Unplug device before reloading.');}finally{busy=false;render();}};
$('boot').onchange=()=>{if(!verified)return;$('boot-status').textContent=$('boot').checked?'Boot manually confirmed by you. This does not establish wallet security or qualify other hardware.':'Boot is not confirmed.';document.querySelectorAll('.steps .state')[3].textContent=$('boot').checked?'OBSERVED':'MANUAL';};
choose('appupdate');
$('install').hidden=true;
$('flash').textContent='Release HOLD — writes disabled';
render();
if(!supported){$('environment').textContent='Installation unavailable: use a secure HTTPS page in desktop Chrome or Edge with Web Serial. Mobile browsers are unsupported. No browser/OS combination is claimed qualified.';status('Unsupported browser or insecure context. Hardware actions are disabled.');}
else{$('environment').textContent='Web Serial is available. Desktop Chrome or Edge is proposed; browser/OS qualification and factory-install hardware acceptance remain pending. Mobile installation is unsupported.';try{
 const [adapter,vendor]=await Promise.all([import('./adapter/profiles.mjs'),import('./vendor/esptool-js.mjs')]);
 if(typeof adapter.createSession!=='function')throw new Error('Guarded API required');
 modules={...adapter,...vendor};status('Select mode, then download and verify the pinned images.');
 }catch{status('Guarded adapter/dependency unavailable or incompatible. Hardware actions remain disabled.');}
 render();}
