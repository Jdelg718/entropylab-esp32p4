import test from 'node:test';
import assert from 'node:assert/strict';
import { initialState, transition, hardwareAdapter, feedbackPreview } from '../../site/flash/model.mjs';
import { candidate, manifestSha256 } from '../../site/flash/candidate.mjs';

test('initial and confirmed states stay hard blocked with three separate pending phases', () => {
  let s=initialState();
  for (const key of ['board','revision','connector','practice','risk']) s=transition(s,{type:'confirm',key,value:true});
  assert.equal(s.mode,'install');
  assert.equal(s.canConnect,false);
  assert.deepEqual(s.phases,{preflight:'blocked',transfer:'pending',readback:'pending',bootConfirmation:'pending'});
  assert.ok(Object.values(s.confirmations).every(Boolean));
  assert.equal(transition(s,{type:'hardware-success'}),s);
});
test('mode change invalidates mode-specific data-loss consent only', () => {
  let s=transition(initialState(),{type:'confirm',key:'risk',value:true});
  s=transition(s,{type:'mode',value:'update'});
  assert.equal(s.mode,'update'); assert.equal(s.confirmations.risk,false);
  assert.equal(transition(s,{type:'mode',value:'other'}),s);
  assert.equal(transition(s,{type:'confirm',key:'__proto__',value:true}),s);
});
test('adapter denies every hardware operation even with forged approvals', () => {
  for(const method of ['connect','install','update']) assert.throws(()=>hardwareAdapter[method]({canConnect:true,approved:true}),/HARD_BLOCKED/);
  assert.ok(Object.isFrozen(hardwareAdapter));
});
test('feedback projection is opt-in, allowlisted, and contains no arbitrary text', () => {
  let s=initialState(); assert.equal(feedbackPreview(s),'');
  s=transition(s,{type:'feedback',value:true});
  const malicious='secret relative/private-path device-id<script>';
  s=transition(s,{type:'category',value:malicious});
  assert.equal(s.feedbackCategory,'layout');
  assert.equal(feedbackPreview({...s,mode:malicious,feedbackCategory:malicious,rawSerial:malicious}).includes(malicious),false);
  s=transition(s,{type:'feedback',value:false}); assert.equal(feedbackPreview(s),'');
});
test('candidate is an unapproved exact three-image metadata snapshot, not update authorization', () => {
 assert.equal(manifestSha256,'54ee881b7e581298bd922883f6c1cdcc0defd7d1c2dd78d2951ad7412e73d06b');
 assert.equal(candidate.status,'NOT APPROVED FOR FLASHING');
 assert.equal(candidate.first_install_exercised,false);
 assert.equal(candidate.app_only_bootloader_replacement_allowed,false);
 assert.equal(candidate.payloads.length,3);
 assert.equal(candidate.target.observed_revision,'1.3');
 assert.ok(Object.isFrozen(candidate.payloads[0]));
});
