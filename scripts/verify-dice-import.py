#!/usr/bin/env python3
"""Verify portable import bytes, public fixture counts and header separation."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parents[1]
def verify_identities(root):
 combined_bytes=(root/'docs/combined-import-successors.json').read_bytes()
 assert hashlib.sha256(combined_bytes).hexdigest()=='6175826b90922c266f8fa59940436cf814162213fb38b0f06c656f4fdd902480', 'unreviewed combined successor manifest'
 combined=json.loads(combined_bytes)
 ui_bytes=(root/'docs/ui-repair-successors.json').read_bytes()
 assert hashlib.sha256(ui_bytes).hexdigest()=='5770532b54d8c6e9f3cc43515158c9a98b4e3d06254c722e82ffe0f550141513', 'unreviewed UI repair manifest'
 ui=json.loads(ui_bytes)
 assert ui['schema']=='ui-repair-successors-v1'
 assert ui['prior_manifest']=='docs/combined-import-successors.json'
 assert ui['prior_manifest_sha256']==hashlib.sha256(combined_bytes).hexdigest()
 checkout_bytes=(root/'docs/public-checkout-successors.json').read_bytes()
 assert hashlib.sha256(checkout_bytes).hexdigest()=='1e2206cd513084284192a93e615bfe0bdf9e63db875517fc18a7b48ed18a5140', 'unreviewed checkout successor manifest'
 checkout=json.loads(checkout_bytes)
 assert checkout['schema']=='public-checkout-successors-v1'
 assert checkout['prior_manifest']=='docs/ui-repair-successors.json'
 assert checkout['prior_manifest_sha256']==hashlib.sha256(ui_bytes).hexdigest()
 expected_classifications={
  'fixture-firmware/app/main/gui.c':'production-source',
  'fixture-firmware/app/main/gui08_native.c':'production-source',
  'fixture-firmware/app/main/mnemonic_editor.inc':'production-source',
  'fixture-firmware/app/main/navigation.inc':'production-source',
  'fixture-firmware/tests/gui_host.c':'host-test-only',
  'fixture-firmware/tests/layout_tests.inc':'host-test-only',
  'fixture-firmware/tests/modal_touch_tests.inc':'host-test-only',
  'fixture-firmware/tests/explanation_tests.inc':'host-test-only',
 }
 expected_order=tuple(expected_classifications)
 actual_order=tuple(x['destination'] for x in checkout['entries'])
 assert actual_order==expected_order, 'checkout successor destination/order mismatch'
 for entry in checkout['entries']:
  path=entry['destination']
  if path.startswith('fixture-firmware/app/main/'):
   required='production-source'
  elif path.startswith('fixture-firmware/tests/'):
   required='host-test-only'
  else:
   raise AssertionError('checkout successor path outside classified roots')
  assert entry.get('classification')==required==expected_classifications[path], f'classification/path mismatch: {path}'
 release_bytes=(root/'docs/d6-release-successors.json').read_bytes()
 assert hashlib.sha256(release_bytes).hexdigest()=='92c9b8343f023ace5b5d29e685b334f209993bb2b1ba50a3c67accd24e58ed5a', 'unreviewed D6 release successor manifest'
 release=json.loads(release_bytes)
 assert release['schema']=='d6-release-successors-v1'
 assert release['prior_manifest_sha256']==hashlib.sha256(checkout_bytes).hexdigest()
 expected_release={'fixture-firmware/app/main/gui.c': 'production-source', 'fixture-firmware/app/main/navigation.inc': 'production-source', 'fixture-firmware/tests/cleanup_tests.inc': 'host-test-only', 'fixture-firmware/tests/coin_tests.inc': 'host-test-only', 'fixture-firmware/tests/dice_tests.inc': 'host-test-only', 'fixture-firmware/tests/gui_acceptance.inc': 'host-test-only', 'fixture-firmware/tests/gui_host.c': 'host-test-only', 'fixture-firmware/tests/passphrase_integration_tests.inc': 'host-test-only', 'fixture-firmware/tests/seed_panel_tests.inc': 'host-test-only'}
 assert tuple(e['destination'] for e in release['entries'])==tuple(expected_release), 'D6 successor destination/order mismatch'
 release_updates={e['destination']:e for e in release['entries']}
 # Independently checked public-checkout predecessor identities for every D6 entry.
 expected_predecessors={'fixture-firmware/app/main/gui.c': '498e4b8b85849dfef163d412d165e1dcce6e4363502413a47c5fd8a0e8da64f1', 'fixture-firmware/app/main/navigation.inc': '5d167b51f152bf2c90db32652acd928622971f5eb9d0b687f149bf474d4dbb36', 'fixture-firmware/tests/cleanup_tests.inc': 'ccab617cd8cebc1c9c6aa02b53625d2632cdb213ede23c76f2da1d910a18d068', 'fixture-firmware/tests/coin_tests.inc': '93958a966a9d1545675ffbdb7b5b9f52d8e52d9c21092e1d61b5dd37d657208c', 'fixture-firmware/tests/dice_tests.inc': '72c1a5b3518088e78c98b5a3fbb25aa2dd8d031fd5dde5d1872515c63848e8ae', 'fixture-firmware/tests/gui_acceptance.inc': 'cf9977a3786425b8acc33693afe99adf5e446a8ac04965bc7194eb03e1418ab9', 'fixture-firmware/tests/gui_host.c': '029a73f700f99d7c943199ad85feae26413be87c089db102520c4091400ad756', 'fixture-firmware/tests/passphrase_integration_tests.inc': '7f8d347a07520c6dd9ab41674df651bea33ab672d0eaba42f64c6bb92ce14ea5', 'fixture-firmware/tests/seed_panel_tests.inc': 'b55f3478b566d4a24938b9a1b02b1d801e52e27c3e3c5502854401655f629c31'}
 chain_validated=set()
 for path,e in release_updates.items():
  assert e['classification']==expected_release[path], f'classification/path mismatch: {path}'
  assert e['reason']
  assert e['before_sha256']==expected_predecessors[path], f'D6 successor chain discontinuity: {path}'
  chain_validated.add(path)
 assert chain_validated==set(release_updates)==set(expected_predecessors), 'D6 chain coverage mismatch'
 repair_bytes=(root/'docs/review-repair-successors.json').read_bytes()
 assert hashlib.sha256(repair_bytes).hexdigest()=='7b027a35dbed6e6324229e98369ba4e96a293638213b2235c6b27b28fa7afbb1', 'unreviewed review-repair manifest'
 repair_manifest=json.loads(repair_bytes)
 assert repair_manifest['schema']=='review-repair-successors-v1'
 assert repair_manifest['prior_manifest']=='docs/d6-release-successors.json'
 assert repair_manifest['prior_manifest_sha256']==hashlib.sha256(release_bytes).hexdigest(), 'review-repair prior manifest mismatch'
 repair_predecessors={'fixture-firmware/app/main/gui.c': '057b9b07d25eff22db9b0c40ad6eecd2ba5f635e1567bed565fcd6b8adf363ac', 'fixture-firmware/app/main/saver.inc': 'b331649aa1898d73e623b78e27e6d158a41a82ce4cdac01ac892a1963a2ff34d', 'fixture-firmware/tests/saver_tests.inc': 'c0210718bb6223e81c7ea2150bba482658b7dbac15230d915f6d89d2230e83d0'}
 repair_classifications={'fixture-firmware/app/main/gui.c': 'production-source', 'fixture-firmware/app/main/saver.inc': 'production-source', 'fixture-firmware/tests/saver_tests.inc': 'host-test-only'}
 assert tuple(e['destination'] for e in repair_manifest['entries'])==tuple(repair_predecessors), 'review-repair destination/order mismatch'
 repair_updates={e['destination']:e for e in repair_manifest['entries']}
 for path,e in repair_updates.items():
  assert e['classification']==repair_classifications[path], f'classification/path mismatch: {path}'
  assert e['reason'], 'review-repair reason required'
  assert e['before_sha256']==repair_predecessors[path], f'review-repair chain discontinuity: {path}'
  assert hashlib.sha256((root/path).read_bytes()).hexdigest()==e['candidate_sha256'], f'review-repair current mismatch: {path}'
 modal_bytes=(root/'docs/modal-successors.json').read_bytes()
 assert hashlib.sha256(modal_bytes).hexdigest()=='9e1d69da912c3dc592419a77cab6179f5876efe2eb1abe4ce6946a834a553c9c', 'unreviewed modal successor manifest'
 modal=json.loads(modal_bytes)
 assert modal['schema']=='modal-successors-v1'
 assert modal['prior_manifest']=='docs/review-repair-successors.json', 'modal prior manifest mismatch'
 assert modal['prior_manifest_sha256']==hashlib.sha256(repair_bytes).hexdigest(), 'modal prior manifest mismatch'
 assert modal['predecessor_package_manifest_sha256']=='cda11abec9a33c49044bf7baa52fea5ec1298cf3d5c884a9cd8d2542121ad82f', 'modal predecessor package mismatch'
 modal_predecessors={'fixture-firmware/app/main/navigation.inc': 'c76e36c9774766a6f8318fe92e605e7a91b016b9724d0d646df42a0fb6e030dd', 'fixture-firmware/tests/modal_touch_tests.inc': '7ef627450bae0b17cdaceecd93745d7ee57e6bfd3004a917e08518abb8a84e55'}
 modal_classes={'fixture-firmware/app/main/navigation.inc': 'production-source', 'fixture-firmware/tests/modal_touch_tests.inc': 'host-test-only'}
 assert tuple(e['destination'] for e in modal['entries'])==tuple(modal_predecessors), 'modal destination/order mismatch'
 modal_updates={e['destination']:e for e in modal['entries']}
 for path,e in modal_updates.items():
  assert e['before_sha256']==modal_predecessors[path], f'modal chain discontinuity: {path}'
  assert e['classification']==modal_classes[path], f'classification/path mismatch: {path}'
  assert e['reason'], 'modal reason required'
  assert hashlib.sha256((root/path).read_bytes()).hexdigest()==e['candidate_sha256'], f'modal current mismatch: {path}'
 def modal_hash(path,expected):
  if path in modal_updates:
   e=modal_updates[path]
   assert e['before_sha256']==expected, f'modal chain discontinuity: {path}'
   return e['candidate_sha256']
  return expected
 def current_hash(path,expected):
  if path in repair_updates:
   e=repair_updates[path]
   assert e['before_sha256']==expected, f'review-repair chain discontinuity: {path}'
   return modal_hash(path,e['candidate_sha256'])
  return modal_hash(path,expected)
 for path,e in release_updates.items():
  assert hashlib.sha256((root/path).read_bytes()).hexdigest()==current_hash(path,e['candidate_sha256']), path
 def release_hash(path,expected):
  if path in release_updates:
   e=release_updates[path]
   assert e['before_sha256']==expected, 'D6 successor chain discontinuity'
   return current_hash(path,e['candidate_sha256'])
  return current_hash(path,expected)
 latest={x['destination']:x for x in checkout['entries']}
 assert len(latest)==len(checkout['entries'])==8
 repairs={x['destination']:x for x in ui['entries']}
 assert len(repairs)==len(ui['entries'])==15
 assert set(latest)<=set(repairs)|{'fixture-firmware/tests/explanation_tests.inc'}
 def final_hash(path,expected):
  if path in latest:
   entry=latest[path]
   assert entry['before_sha256']==expected, 'checkout successor chain discontinuity'
   assert entry['reason']
   return release_hash(path,entry['candidate_sha256'])
  return release_hash(path,expected)
 for path,repair in repairs.items():
  assert repair['reason']
  assert hashlib.sha256((root/path).read_bytes()).hexdigest()==final_hash(path,repair['candidate_sha256']),path
 assert combined['schema']=='combined-import-successors-v1'
 updates={x['destination']:x for x in combined['entries']}
 assert len(updates)==len(combined['entries'])==5
 # Pin both reviewed records: a successor is not permission to rebaseline files.
 historical=(root/'docs/dice-import-manifest.json').read_bytes()
 successor=(root/'docs/words-import-manifest.json').read_bytes()
 assert hashlib.sha256(historical).hexdigest()=='df76366c5bc512994f8cb993d584eb918464d6c5f2c62766bb7774e873e75a5e', 'historical manifest changed'
 assert hashlib.sha256(successor).hexdigest()=='3cb686840b5a8805afd80ddd3fb3d3520bdba90e9ec3ee2087a0facffd20b091', 'unreviewed successor manifest'
 native_bytes=(root/'docs/input-explanations-native-edit.json').read_bytes()
 assert hashlib.sha256(native_bytes).hexdigest()=='dcd864bb86f0778b275e84379334c248eb4012bdd1c5467929ed1a05f8cc2599', 'unreviewed native edit manifest'
 native=json.loads(native_bytes)
 assert native['schema']=='native-edit-successors-v1'
 assert native['prior_manifest']=='docs/words-import-manifest.json'
 assert native['prior_manifest_sha256']==hashlib.sha256(successor).hexdigest()
 assert len(native['entries'])==1
 edit=native['entries'][0]
 assert edit['destination']=='fixture-firmware/tests/gui_host.c'
 assert edit['reason']
 assert native['test_include']['destination']=='fixture-firmware/tests/explanation_tests.inc'
 assert checkout['native_manifest_sha256']==hashlib.sha256(native_bytes).hexdigest()
 assert hashlib.sha256((root/native['test_include']['destination']).read_bytes()).hexdigest()==final_hash(native['test_include']['destination'],native['test_include']['sha256']), 'native test include changed'
 m=json.loads(historical)
 words=json.loads(successor)
 assert words['historical_manifest_sha256']==hashlib.sha256(historical).hexdigest()
 assert words['source_manifest_sha256']=='e317b078cbea59ffb533e0d7c025a844e39e960a981ec0b77fa12a9cfc25e92e'
 successors={x['destination']:x for x in words['entries']}
 assert len(successors)==len(words['entries'])
 assert set(successors)<=set(x['destination'] for x in m['entries'])
 for x in m['entries']:
  expected=x['candidate_sha256']
  if x['destination'] in successors:
   update=successors[x['destination']]
   assert update['before_candidate_sha256']==expected
   assert update['source']==x['destination'].removeprefix('fixture-firmware/')
   assert update['candidate_sha256']==update['source_sha256']
   assert update['reason']
   expected=update['candidate_sha256']
  if x['destination']==edit['destination']:
   assert x['destination'] in successors
   assert edit['before_candidate_sha256']==expected
   expected=edit['candidate_sha256']
  if x['destination'] in updates:
   update=updates[x['destination']]
   assert update['before_sha256']==expected
   expected=update['candidate_sha256']
  if x['destination'] in repairs:
   repair=repairs[x['destination']]
   assert repair['before_sha256']==expected, 'UI repair chain discontinuity'
   expected=repair['candidate_sha256']
  assert hashlib.sha256((root/x['destination']).read_bytes()).hexdigest()==final_hash(x['destination'],expected),x['destination']
  assert x['exact'] or x['destination'] in m['exceptions']
verify_identities(r)
rows=[x.split('\t') for x in (r/'fixture-firmware/dice/vectors/dice.tsv').read_text().splitlines()]
assert len(rows)==20 and sum(int(x[4])>=0 for x in rows)==14 and sum(int(x[4])<0 for x in rows)==6
for bits,count in zip([128,160,192,224,256],[50,62,75,87,100]):assert 6**(count-1)<2**bits<=6**count
assert 'lvgl.h' not in (r/'fixture-firmware/app/main/gui.h').read_text()
assert (r/'fixture-firmware/dice/dice_core.h').read_bytes()==(r/'fixture-firmware/app/main/dice_core.h').read_bytes()
print('PASS import identities, seven documented exceptions, upstream20/native14accept6reject, all exact integer thresholds, LVGL-free worker header')
