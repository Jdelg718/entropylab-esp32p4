#!/usr/bin/env node
import assert from "node:assert/strict"; import {readFileSync} from "node:fs";
const app=readFileSync(process.argv[2],"utf8");
function fn(n){const s=app.indexOf(`function ${n}(`);assert(s>=0);let d=0;for(let i=app.indexOf("{",s);i<app.length;i++){if(app[i]==="{")d++;else if(app[i]==="}"&&!--d)return app.slice(s,i+1);}throw Error(n)}
const a=app.search(/var\s+hodlSeedLengths\s*=/),z=app.search(/var\s+hodlBip39WordSet\s*=/);assert(a>=0&&z>a);
const api=new Function(`var hodlHexFormatLabels={bin:{},base4:{},base8:{},hex:{},base32:{},base64:{}};${app.slice(a,z)};var hodlTargetWordCount=24;${fn("hodlSeedConfig")}${fn("hodlNormalizeEntropyFormat")}${fn("hodlEntropyFormatConfig")}${fn("hodlNormalizeEntropyCharacter")}${fn("hodlEntropyDigitEntries")}${fn("hodlEntropyDigits")}${fn("hodlNumberBaseBits")}${fn("hodlNumberBaseValueFromBytes")}${fn("hodlAnalyzeEntropyInput")}return {hodlEntropyFormatConfig,hodlNumberBaseBits,hodlNumberBaseValueFromBytes,hodlAnalyzeEntropyInput}`)();
const rows={12:[64,43,26,23],15:[80,54,32,30],18:[96,64,39,32],21:[112,75,45,39],24:[128,86,52,46]};
for(const [w,row] of Object.entries(rows))assert.deepEqual(["base4","base8","base32","base64"].map(x=>api.hodlEntropyFormatConfig(x,+w).digits),row);
for(const w of [12,15,18,21,24]){const bytes=Uint8Array.from({length:w*4/3},(_,i)=>i);for(const b of ["base4","base8","base32","base64"]){const v=api.hodlNumberBaseValueFromBytes(bytes,b,w);assert(api.hodlAnalyzeEntropyInput(v,b,w).ready);assert.equal(api.hodlNumberBaseBits(v,b,w),[...bytes].map(x=>x.toString(2).padStart(8,"0")).join(""));}}
assert(api.hodlAnalyzeEntropyInput("0".repeat(42)+"4","base8",12).finalInvalid);
assert(api.hodlAnalyzeEntropyInput("q".repeat(51)+"z","base32",24).finalInvalid);
assert(api.hodlAnalyzeEntropyInput("A".repeat(42)+"000A","base64",24).finalInvalid);
console.log("upstream source-slice oracle: PASS");
