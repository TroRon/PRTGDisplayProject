import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,rm,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {createDisplayStore,validateDisplaySettings} from '../src/displays.mjs';
import {createApi} from '../src/server.mjs';
const settings={name:'Demo Display',rotate:false,rotateSeconds:20,night:false,nightStart:22,nightEnd:7,brightness:25,utcOffsetMinutes:60,interrupt:false,history:false,favoriteHome:false,favorites:[]};
const id='a'.repeat(32),key='b'.repeat(64),report=()=>({id,firmware:'1.1.0',revision:0,mode:'local',settings:structuredClone(settings),error:'',release:false});
test('display lifecycle: authentication, revision conflicts, ACK, offline, restart and local release',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'display-test-'));let at=1000;
 try{
 const store=await createDisplayStore(dir,{now:()=>at});let response=await store.sync(report(),key);assert.equal(response.mode,'local');
 assert.equal(store.list()[0].state,'applied');assert.ok(!JSON.stringify(store.list()).includes(key));assert.ok(!(await readFile(join(dir,'displays.json'),'utf8')).includes(key));
 await assert.rejects(store.sync(report(),'c'.repeat(64)),/DISPLAY_AUTH/);
 const next={...settings,name:'Zentrale',history:true,favorites:['pbs-1']};
 await store.update(id,{revision:0,mode:'central',settings:next});assert.equal(store.list()[0].state,'pending');
 await assert.rejects(store.update(id,{revision:0,mode:'central',settings:next}),/DISPLAY_CONFLICT/);
 response=await store.sync(report(),key);assert.equal(response.revision,1);assert.equal(response.settings.name,'Zentrale');
 await store.sync({...report(),revision:1,mode:'central',settings:next,error:'STORAGE'},key);assert.equal(store.list()[0].state,'error');
 await store.sync({...report(),revision:1,mode:'central',settings:next},key);assert.equal(store.list()[0].state,'applied');
 at+=181000;assert.equal(store.list()[0].state,'offline');
 const restored=await createDisplayStore(dir);assert.equal(restored.list()[0].state,'offline');assert.equal(restored.list()[0].revision,1);
 response=await restored.sync({...report(),release:true},key);assert.equal(response.mode,'local');assert.equal(response.revision,2);
 response=await restored.sync({...report(),release:true},key);assert.equal(response.revision,2);
 assert.equal((await restored.sync({...report(),id:'d'.repeat(32)},key)).revision,0);
 }finally{await rm(dir,{recursive:true,force:true});}
});
test('strict settings reject credentials, malformed types, long UTF8 and duplicate favorites',()=>{
 assert.deepEqual(validateDisplaySettings(settings),settings);
 for(const bad of [{...settings,token:'secret'},{...settings,name:'ä'.repeat(25)},{...settings,brightness:0},{...settings,night:'true'},{...settings,rotateSeconds:1.5},{...settings,favorites:['a','a']},{...settings,favorites:['../a']},{...settings,name:'a\nb'}])assert.throws(()=>validateDisplaySettings(bad),/DISPLAY_SETTINGS/);
});
test('sync API requires panel token and independent device key; GET health remains compatible',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'display-http-'));const store=await createDisplayStore(dir);const api=createApi({snapshot:()=>({schema_version:1})},'test-panel-token',null,null,null,store);
 await new Promise(resolve=>api.listen(0,'127.0.0.1',resolve));const url='http://127.0.0.1:'+api.address().port;
 try{const send=(headers={},body=report())=>fetch(url+'/api/v1/display/sync',{method:'POST',headers,body:JSON.stringify(body)});
 assert.equal((await send()).status,401);
 assert.equal((await send({Authorization:'Bearer test-panel-token'})).status,400);
 assert.equal((await send({Authorization:'Bearer test-panel-token','X-Display-Key':key})).status,200);
 assert.equal((await send({Authorization:'Bearer test-panel-token','X-Display-Key':'c'.repeat(64)})).status,403);
 assert.equal((await send({Authorization:'Bearer test-panel-token','X-Display-Key':key},{...report(),padding:'x'.repeat(9000)})).status,400);
 assert.equal((await fetch(url+'/api/v1/health',{headers:{Authorization:'Bearer test-panel-token'}})).status,200);
 }finally{await new Promise(resolve=>api.close(resolve));await rm(dir,{recursive:true,force:true});}
});
