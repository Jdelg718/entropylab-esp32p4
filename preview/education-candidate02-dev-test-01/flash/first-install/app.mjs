import {downloadImages} from './assets.mjs';
import {BOARD,createSession} from './adapter/profiles.mjs';
import {ESPLoader,Transport} from './vendor/esptool-js.mjs';
const $=id=>document.getElementById(id); const abort=new AbortController();
let assets,port,session,ended=false,busy=false;
// Decorative per-image track: the polite status remains the single accessible source.
// Never interpolate byte counts/width or run autonomous activity indicators.
let progressAnimation;
const reducedMotion=globalThis.matchMedia?.('(prefers-reduced-motion: reduce)');
function stopProgressVisual(){
 progressAnimation?.cancel();progressAnimation=undefined;
 const track=$('image-progress');if(track)track.hidden=true;
}
reducedMotion?.addEventListener('change',()=>{progressAnimation?.cancel();progressAnimation=undefined;});
function drawProgress(p,total,previous){
 const track=$('image-progress'),fill=$('image-progress-fill');
 if(!track||!fill||total===null)return;
 track.hidden=false;track.dataset.kind=p.kind;
 fill.style.width=`${p.bytes/total*100}%`;
 const advanced=p.bytes>0&&(!previous||previous.order!==p.order||p.bytes>previous.bytes);
 if(advanced&&p.kind!=='image-verified'&&!reducedMotion?.matches){
  progressAnimation=fill.animate?.([{opacity:0.6},{opacity:1}],{duration:180,iterations:1});
 }
}
const status=s=>{stopProgressVisual();$('status').textContent=s;};
let lastProgress, lastDisplayed, progressEnded=false;
const phaseLabels={'verifying-assets':'Verifying downloaded images','connecting-rom':'Connecting ROM at 115200','security-preflight':'Checking ROM security','official-stub':'Loading official RAM stub at 115200','changing-baud':'Switching stub transport to 460800','jedec-preflight':'Checking flash capacity','writing':'Writing compressed images (not yet verified)','verifying-readback':'Reading back images and erased tails for SHA-256 verification','complete':'Complete','diagnostic-complete':'Diagnostic complete','failed-disconnected':'Failed; disconnected','failed-cleanup-incomplete':'Failed; cleanup incomplete. Unplug before reloading for a new attempt'};
function state(e){
 if(ended||progressEnded||!e||typeof e.state!=='string')return;
 if(e.state==='complete'||e.state==='diagnostic-complete'||e.state.startsWith('failed-'))progressEnded=true;
 const parts=[phaseLabels[e.state]||'Unknown phase'];
 if(['OPERATION_TIMEOUT','SESSION_CANCELLED','DEPENDENCY_ERROR','SECURITY_REFUSED','SECURITY_FRAME_REFUSED','CHIP_REFUSED','REVISION_REFUSED','JEDEC_REFUSED','STUB_REFUSED','EXISTING_STUB_REFUSED','BAUD_SWITCH_UNSUPPORTED','BAUD_SWITCH_REFUSED','CLEANUP_INCOMPLETE','READBACK_MISMATCH','READBACK_TRAILER_REFUSED','ASSET_REFUSED','ASSET_HASH_MISMATCH','BOARD_CONFIRMATION_REQUIRED','FIRSTINSTALL_CONSENT_REQUIRED','PORT_BUSY','IO_CLOSED'].includes(e.errorCode))parts.push(e.errorCode);
 if(phaseLabels[e.phase])parts.push('Phase: '+phaseLabels[e.phase]);
 const d=e.securityDiagnostic;
 if(d&&typeof d==='object'){
  if(Number.isInteger(d.apiVersionNumber)&&d.apiVersionNumber>=0&&d.apiVersionNumber<=0xffffffff)parts.push('ROM ECO: '+d.apiVersionNumber);
  if(['CHIP_REFUSED','FLAGS_INVALID','FLAGS_UNKNOWN_BITS','API_VERSION_REFUSED','SECURE_BOOT_NOT_FALSE','SECURE_DOWNLOAD_NOT_FALSE','FLASH_CRYPT_COUNT_NOT_ZERO','SECURITY_CHECKS_CLEAR'].includes(d.reason))parts.push(d.reason);
  for(const [key,label,allowed] of [['secureBoot','Secure boot',['true','false','invalid']],['secureDownload','Secure download',['true','false','invalid']],['flashCryptCnt','Flash encryption count',['zero','nonzero','invalid']],['flags','Flags',['known-bits-only','unknown-bits','invalid']]])if(allowed.includes(d[key]))parts.push(label+': '+d[key]);
  parts.push('No physical authentication or boot qualification.');
 }
 status(parts.join('; '));
}
function progress(p){
 if(ended||progressEnded||!p||!['compressed-write','image-read','image-verified'].includes(p.kind)||!Number.isInteger(p.assetIndex)||p.assetIndex<0||p.assetIndex>(p.kind==='compressed-write'?2:5)||!Number.isSafeInteger(p.bytes)||p.bytes<0||!Number.isFinite(p.elapsedMs)||p.elapsedMs<0)return;
 if(p.phase!==(p.kind==='compressed-write'?'writing':'verifying-readback'))return;
 const total=Number.isSafeInteger(p.totalBytes)&&p.totalBytes>0?p.totalBytes:null;
 if((total!==null&&p.bytes>total)||(p.kind==='image-verified'&&(total===null||p.bytes!==total)))return;
 const order=p.kind==='compressed-write'?p.assetIndex:p.assetIndex*2+3+(p.kind==='image-verified'?1:0);
 if(lastProgress&&(p.elapsedMs<lastProgress.elapsedMs||order<lastProgress.order||(order===lastProgress.order&&p.bytes<lastProgress.bytes)))return;
 p={...p,order};
 const previous=lastDisplayed;lastProgress={...p};
 // Exact bytes update synchronously; animation never delays the status.
 lastDisplayed={...p};
 const name=['bootloader','partition table','application'][p.assetIndex%3];
 const phase=p.kind==='compressed-write'?'Writing '+name:p.assetIndex>2?'Verifying erased tail for '+name:'Verifying '+name;
 const label=p.kind==='compressed-write'?'Compressed bytes acknowledged':p.kind==='image-verified'?'SHA-256 verified bytes':'Bytes read (SHA-256 pending)';
 status(`${phase} · ${Math.floor(p.elapsedMs/1000)}s elapsed\n${label}: ${p.bytes}${total===null?'':' / '+total}`);
 drawProgress(p,total,previous);
}
function render(){$('flash').disabled=busy||ended||!assets||!port||!$('board').checked||!$('consent').checked;$('prepare').disabled=busy||ended||!!assets;$('connect').disabled=busy||ended||!assets||!$('board').checked;$('check').disabled=busy||ended||!port||!$('board').checked;}
$('board').onchange=render;$('consent').onchange=render;
$('prepare').onclick=async()=>{if($('prepare').disabled)return;busy=true;render();try{const downloaded=await downloadImages(abort.signal);if(ended)return;assets=downloaded;session=createSession({mode:'publicfirstinstall',ESPLoader,Transport,onProgress:progress,onState:state});status('Images verified. Select the authorized port; diagnostic or explicit destructive write are separate one-shot attempts.');}catch{if(ended)return;ended=true;status('Asset preparation refused. Reload for a fresh session.');}finally{busy=false;render();}};
$('connect').onclick=async()=>{if($('connect').disabled)return;busy=true;render();try{const selected=await navigator.serial.requestPort();if(ended)return;port=selected;status('Port selected, NOT opened. No automatic write.');}catch{if(ended)return;ended=true;status('Selection cancelled; no retry. Reload for a fresh session.');}finally{busy=false;render();}};
async function execute(diagnostic){const button=$(diagnostic?'check':'flash');if(button.disabled)return;busy=true;render();let completed=false;try{const args={port,assets,boardConfirmation:BOARD,firstInstallConsent:$('consent').checked,resetOnSuccess:false};if(diagnostic)await session.checkDevice(args);else await session.run(args);if(ended)return;completed=!diagnostic;status($('status').textContent+(diagnostic?' Diagnostic complete; no firmware writes.':' All three images and erased tails verified. Press the board RESET button now, then confirm boot/display/touch and the public D6 test: Back, reopen Test results without recalculating. Readback is not boot or recovery qualification. Installation session is complete; do not repeat installation just to boot.'));}catch{if(ended)return;status($('status').textContent+' Attempt refused or failed; no automatic retry or success reset.');}finally{busy=false;if(!ended&&!completed)status($('status').textContent+' Session ended. Reload, reverify images and reselect port for a fresh attempt.');ended=true;render();}}
$('check').onclick=()=>execute(true);
$('flash').onclick=()=>execute(false);
$('cancel').onclick=async()=>{if(ended)return;ended=true;abort.abort();render();status('Cancelled. No retry. If cleanup is uncertain, unplug before reloading for a new attempt.');try{if(await session?.cancel()===false)status('Cancelled; cleanup incomplete. Unplug before reloading for a new attempt.');}catch{status('Cancelled; cleanup incomplete. Unplug before reloading for a new attempt.');}};render();
