import {test} from 'node:test';
import {request as httpRequest} from 'node:http';
import assert from 'node:assert/strict';
import {mkdtemp,readFile,rm,mkdir,writeFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';
import {generateKeyPairSync,sign,createHash} from 'node:crypto';
import {createAdminStore} from '../src/admin-store.mjs';
import {createAdmin} from '../src/admin.mjs';
import {MonitoringRuntime} from '../src/runtime.mjs';
import {createFirmwareService} from '../src/firmware.mjs';
import {createApi} from '../src/server.mjs';
import {validateConfig} from '../src/config.mjs';

const config=()=>({prtg_url:'https://prtg.example.org',poll_seconds:30,stale_seconds:300,request_timeout_seconds:10,entities:[{id:'pbs-zurich',label:'PBS Zürich',category:'backup',sensors:[{key:'health',id:101,metrics:{usage:'Datastore Usage'}}]}]});
const adminToken='synthetic-admin-'+ 'a'.repeat(32),panelToken='synthetic-panel-'+'b'.repeat(32),secret='synthetic-prtg-secret';
async function folder(t){const dir=await mkdtemp(join(tmpdir(),'prtg-admin-test-'));t.after(async()=>{assert.equal(resolve(dir,'..'),resolve(tmpdir()));await rm(dir,{recursive:true,force:true});});return dir;}

test('config persists atomically, survives restart, keeps secrets private, detects stale writes and host changes',async t=>{
 const directory=await folder(t),applied=[];
 const store=await createAdminStore({directory,config:config(),prtgToken:secret,apply:s=>applied.push(s)});
 assert(!JSON.stringify(store.view()).includes(secret));
 const next=config();next.entities[0].label='PBS Bern';next.entities.push({...structuredClone(next.entities[0]),id:'pbs-other',label:'PBS Aus',enabled:false,sensors:[{key:'health',id:102,metrics:{}}]});
 await store.save({revision:0,config:next});assert.equal(applied.length,1);assert.equal(applied[0].prtgToken,secret);
 const restart=await createAdminStore({directory,config:config(),prtgToken:secret});assert.equal(restart.view().config.entities[0].label,'PBS Bern');
 await assert.rejects(store.save({revision:0,config:config()}),/CONFLICT/);
 const changed={...next,prtg_url:'https://other.example.org'};
 await assert.rejects(store.save({revision:1,config:changed}),/TOKEN_REQUIRED/);
 await store.save({revision:1,config:changed,prtgToken:'synthetic-replacement'});
 assert(!JSON.stringify(store.view()).includes('synthetic-replacement'));
 const after=await createAdminStore({directory,config:config(),prtgToken:secret});assert.equal(after.effective().prtgToken,'synthetic-replacement');
 const simultaneous=await Promise.allSettled([store.save({revision:2,config:changed}),store.save({revision:2,config:changed})]);assert.equal(simultaneous.filter(r=>r.status==='fulfilled').length,1);
 assert.equal(JSON.parse(await readFile(join(directory,'settings.json'))).revision,3);
});
test('storage failures preserve active config and damaged files never fall back silently',async t=>{
 const directory=await folder(t);let applied=0;
 const store=await createAdminStore({directory,config:config(),apply:()=>applied++});
 await mkdir(join(directory,'settings.json'));
 await assert.rejects(store.save({revision:0,config:config()}));assert.equal(store.view().revision,0);assert.equal(applied,0);
 await rm(join(directory,'settings.json'),{recursive:true});await writeFile(join(directory,'settings.json'),'{invalid');
 await assert.rejects(createAdminStore({directory,config:config()}));
});
test('config rejects ambiguous/unsafe input; disabled systems stop polling and disappear, empty configuration remains unknown',async()=>{
 const c=config();c.entities[0].enabled=false;let calls=0;
 const r=new MonitoringRuntime({config:c,prtgToken:secret,clientFactory:()=>({collect:async()=>{calls++;}})});
 await r.current.collector.poll();assert.equal(calls,0);assert.equal(r.snapshot().categories.backup.enabled,false);assert.equal(r.snapshot().overall,'unknown');
 validateConfig({...c,entities:[]});
 for(const mutate of [c=>c.entities[0].label='x'.repeat(64),c=>c.entities[0].sensors[0].id=-1,c=>c.entities[0].enabled='false',c=>c.prtg_url='https://user:secret@example.org',c=>c.entities[0].sensors[0].metrics={'constructor':'bad'},c=>c.extra='secret']){const bad=config();mutate(bad);assert.throws(()=>validateConfig(bad));}
});
test('reload during in-flight polling never reintroduces old healthy results',async t=>{
 let complete;const result=new Promise(resolve=>complete=resolve);const c=config();
 const runtime=new MonitoringRuntime({config:c,prtgToken:secret,clientFactory:()=>({collect:()=>result})});t.after(()=>runtime.stop());
 runtime.stopped=false;const pending=runtime.tick();
 const next=config();next.entities[0].label='Umbenannt';runtime.update({config:next,prtgToken:secret});
 assert.equal(runtime.snapshot().categories.backup.entities[0].label,'Umbenannt');assert.equal(runtime.snapshot().overall,'unknown');
 complete({row:{status_raw:3,lastcheck_raw:(Date.now()-Date.UTC(1899,11,30))/86400000},channels:[{name:'Datastore Usage',lastvalue_raw:50}]});await pending;
 assert.equal(runtime.snapshot().overall,'unknown');assert.equal(runtime.snapshot().updated_at,null);
});

test('real HTTP admin enforces origin/auth/peer, redacts secrets, persists edits, uploads signed firmware and serves the panel',async t=>{
 const directory=await folder(t),keys=generateKeyPairSync('rsa',{modulusLength:2048}),logs=[];
 const runtime=new MonitoringRuntime({config:config(),prtgToken:secret,demo:true});
 const store=await createAdminStore({directory:join(directory,'admin'),config:config(),prtgToken:secret,apply:s=>runtime.update(s)});
 const firmware=createFirmwareService({directory:join(directory,'firmware'),adminToken,panelToken,key:keys.publicKey});
 let admin;const server=createApi(runtime,panelToken,new Set(['127.0.0.1']),firmware,(...args)=>admin(...args));
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 t.after(async()=>{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));});
 const base=`http://127.0.0.1:${server.address().port}`;
 admin=createAdmin({origin:base,token:adminToken,panelToken,store,runtime,firmware,log:(...args)=>logs.push(args.join(' '))});
 const req=(path,method='GET',body,overrides={})=>fetch(base+path,{method,headers:{Authorization:'Bearer '+adminToken,Origin:base,...(body?{'Content-Type':'application/json'}:{}),...overrides},body:body===undefined?undefined:JSON.stringify(body)});
 assert.equal((await fetch(base+'/admin')).status,200);
 assert.equal((await req('/api/admin/config','GET',undefined,{Authorization:'Bearer '+panelToken})).status,401);
 assert.equal((await req('/api/admin/config','GET',undefined,{Origin:'https://evil.example'})).status,403);
 assert.equal(await new Promise((resolve,reject)=>{const q=httpRequest(base+'/api/admin/config',{headers:{Host:'evil.example',Authorization:'Bearer '+adminToken}},r=>{r.resume();resolve(r.statusCode);});q.on('error',reject);q.end();}),403);
 assert.equal((await req('/api/admin/config','PUT',{revision:0,config:config()},{Origin:''})).status,403);
 const response=await req('/api/admin/config');const view=await response.json();assert.equal(view.hasPrtgToken,true);assert(!JSON.stringify(view).includes(secret));
 view.config.entities[0].label='PBS Zweigstelle';assert.equal((await req('/api/admin/config','PUT',{revision:0,config:view.config})).status,200);
 assert.equal((await req('/api/admin/config','PUT',{revision:0,config:view.config})).status,409);
 const panel=await fetch(base+'/api/v1/health',{headers:{Authorization:'Bearer '+panelToken}});assert.equal((await panel.json()).categories.backup.entities[0].label,'PBS Zweigstelle');
 const image=Buffer.alloc(2048);image[0]=0xe9;image.writeUInt16LE(9,12);image.writeUInt32LE(0xabcd5432,32);image.write('1.0.0',48);image.write('eaglenet_lcd5b',80);
 const digest=createHash('sha256').update(image).digest('hex'),manifest={schema:1,board:'waveshare-lcd5b-28151',layout:'eaglenet-ota-v1',version:'1.0.0',sequence:10000,size:image.length,sha256:digest},payload=Buffer.from(JSON.stringify(manifest));
 const envelope=Buffer.from(JSON.stringify({payload:payload.toString('base64'),signature:sign('RSA-SHA256',payload,keys.privateKey).toString('base64')}));const prefix=Buffer.alloc(4);prefix.writeUInt32BE(envelope.length);const pack=Buffer.concat([prefix,envelope,image]);
 const upload=await fetch(base+'/api/admin/firmware',{method:'PUT',headers:{Authorization:'Bearer '+adminToken,Origin:base,'Content-Type':'application/octet-stream'},body:pack});assert.equal(upload.status,200);
 const status=await (await req('/api/admin/status')).json();assert.equal(status.versions[0].sha256,digest);
 const catalog=await fetch(base+'/api/v1/firmware/catalog.json',{headers:{Authorization:'Bearer '+panelToken}});assert.equal((await catalog.json()).releases.length,1);
 assert.deepEqual(Buffer.from(await (await fetch(base+'/api/v1/firmware/'+digest+'.bin',{headers:{Authorization:'Bearer '+panelToken}})).arrayBuffer()),image);
 assert.equal((await fetch(base+'/api/v1/firmware/'+digest+'.bin')).status,401);
 assert.equal((await req('/api/admin/config','PUT',{revision:1,config:config()},{'Content-Type':'text/plain'})).status,400);
 assert(!logs.join().includes(secret));assert(!logs.join().includes(adminToken));
 for(let i=0;i<10;i++)assert.equal((await req('/api/admin/config','GET',undefined,{Authorization:'Bearer wrong'})).status,401);
 assert.equal((await req('/api/admin/config')).status,429);
});
