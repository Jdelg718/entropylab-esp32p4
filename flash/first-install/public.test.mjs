import {test} from 'node:test';
import assert from 'node:assert/strict';
import {GuardedSession,FACTORY_PROFILE} from './adapter/adapter.mjs';
test('public destructive profile is explicit, exact, and contains no private predecessor authority',()=>{
 assert.equal(FACTORY_PROFILE.mode,'publicfirstinstall');
 assert.equal(FACTORY_PROFILE.profileId,'waveshare-p4-4.3-rev1.3-32mib-destructive-v1');
 assert.equal(FACTORY_PROFILE.factoryGuard,undefined);
 assert.equal(FACTORY_PROFILE.regionContract,undefined);
 assert.equal(FACTORY_PROFILE.assets.length,3);
 for(const profile of [{...FACTORY_PROFILE,profileId:'unknown'},{...FACTORY_PROFILE,mode:'firstinstall'},{...FACTORY_PROFILE,assets:[]}]) assert.throws(()=>new GuardedSession({profile}),/PROFILE_REFUSED/);
});
