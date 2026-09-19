import {downloadImages} from './assets.mjs';
import {BOARD,createSession} from './adapter/profiles.mjs';
import {ESPLoader,Transport} from './vendor/esptool-js.mjs';
const $=id=>document.getElementById(id); const abort=new AbortController();
let assets,port,session,ended=false,busy=false;
const status=s=>{$('status').textContent=s;};
function render(){$('flash').disabled=busy||ended||!assets||!port||!$('board').checked||!$('consent').checked;$('prepare').disabled=busy||ended||!!assets;$('connect').disabled=busy||ended||!assets||!$('board').checked;$('check').disabled=busy||ended||!port||!$('board').checked;}
$('board').onchange=render;$('consent').onchange=render;
$('prepare').onclick=async()=>{if($('prepare').disabled)return;busy=true;render();try{assets=await downloadImages(abort.signal);session=createSession({mode:'publicfirstinstall',ESPLoader,Transport,onState:e=>{const d=e.securityDiagnostic;status(`${e.state}; ${e.errorCode||''}; ROM ECO: ${d?.apiVersionNumber??'not reported'}; ${d?.reason||''}. No physical authentication or boot qualification.`);}});status('Images verified. Select the authorized port; diagnostic or explicit destructive write are separate one-shot attempts.');}catch{ended=true;status('Asset preparation refused. Reload for a fresh session.');}finally{busy=false;render();}};
$('connect').onclick=async()=>{if($('connect').disabled)return;busy=true;render();try{port=await navigator.serial.requestPort();status('Port selected, NOT opened. No automatic write.');}catch{ended=true;status('Selection cancelled; no retry. Reload for a fresh session.');}finally{busy=false;render();}};
async function execute(diagnostic){const button=$(diagnostic?'check':'flash');if(button.disabled)return;busy=true;render();try{const args={port,assets,boardConfirmation:BOARD,firstInstallConsent:$('consent').checked,resetOnSuccess:false};if(diagnostic)await session.checkDevice(args);else await session.run(args);status($('status').textContent+(diagnostic?' Diagnostic complete; no firmware writes.':' All three images and FF tails verified; no success reset.'));}catch{status($('status').textContent+' Attempt refused or failed; no automatic retry or success reset.');}finally{ended=true;busy=false;status($('status').textContent+' Session ended. Reload, reverify images and reselect port for a fresh attempt.');render();}}
$('check').onclick=()=>execute(true);
$('flash').onclick=()=>execute(false);
$('cancel').onclick=async()=>{ended=true;abort.abort();render();await session?.cancel();status('Cancelled. No retry. If cleanup is uncertain, unplug before reloading for a new attempt.');};render();
