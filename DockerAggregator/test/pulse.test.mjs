import test from 'node:test';import assert from 'node:assert/strict';
import {PulseClient,mergePulse} from '../src/pulse.mjs';import {validateConfig} from '../src/config.mjs';import {buildHealth} from '../src/health.mjs';import {createAdminStore} from '../src/admin-store.mjs';import {MonitoringRuntime} from '../src/runtime.mjs';
import {mkdtemp,rm} from 'node:fs/promises';import {tmpdir} from 'node:os';import {join} from 'node:path';
const now=Date.now(),config=()=>({prtg_url:'https://prtg.example.org',poll_seconds:30,stale_seconds:300,request_timeout_seconds:5,entities:[],pulse:{enabled:true,url:'https://pulse.example.org',label:'Pulse',allowHttp:false}});
const alarm=(id='a',level='warning')=>({id,level,resourceName:'Demo Service',message:'Demo alarm <script>',startTime:new Date(now-86400000).toISOString(),acknowledged:true});
const client=(summary,alerts,extra={})=>new PulseClient(config(),'synthetic-token',{clock:()=>now,fetchImpl:async(url,options)=>{assert.equal(options.redirect,'error');assert.equal(options.headers['X-API-Token'],'synthetic-token');return new Response(JSON.stringify(url.pathname.endsWith('summary')?summary:alerts),{headers:{'Content-Type':'application/json'}});},...extra});
test('Pulse validates origins, opt-in HTTP, capacity and reserved identities',()=>{assert.doesNotThrow(()=>validateConfig(config()));for(const url of ['http://pulse.example.org','https://user:secret@pulse.example.org','https://pulse.example.org/path'])assert.throws(()=>validateConfig({...config(),pulse:{...config().pulse,url}}),/CONFIG_PULSE/);assert.doesNotThrow(()=>validateConfig({...config(),pulse:{...config().pulse,url:'http://pulse.example.org:7655',allowHttp:true}}));});
test('Pulse accepts current empty/active snapshots; rejects stale, bad shapes, unknown severity stays unknown',async()=>{
 assert.deepEqual((await client({activeAlerts:0,lastUpdate:now},[]).collect()).alerts,[]);
 const result=await client({activeAlerts:2,lastUpdate:new Date(now).toISOString()},[alarm(),alarm('b','critical')]).collect();assert.equal(result.alerts[0].status,'critical');assert.equal(result.alerts[1].acknowledged,true);
 assert.equal((await client({activeAlerts:1,lastUpdate:now},[alarm('x','surprise')]).collect()).alerts[0].status,'unknown');
 for(const lastUpdate of [now-301000,now+61000,null])assert.equal((await client({activeAlerts:0,lastUpdate},[]).collect()).error,'PULSE_STALE');
 assert.equal((await client({activeAlerts:1,lastUpdate:now},[]).collect()).error,'PULSE_SHAPE');
 assert.equal((await client({activeAlerts:2,lastUpdate:now},[alarm(),alarm()]).collect()).error,'PULSE_SHAPE');
 for(const status of [401,403,500])assert.equal((await client({},[],{fetchImpl:async()=>new Response('',{status})}).collect()).error,status===500?'PULSE_HTTP':'PULSE_AUTH');
 assert.equal((await client({},[],{fetchImpl:async()=>new Response('x'.repeat(524289),{headers:{'Content-Type':'application/json'}})}).collect()).error,'PULSE_SIZE');
});
test('Pulse merge: OK only on fresh empty snapshots, outage/expiry unknown, bounded cards and critical first',async()=>{
 const c=config(),base=()=>buildHealth(c,new Map(),{now,collectedAt:now});
 assert.equal(mergePulse(base(),c,null,now).overall,'unknown');
 const empty={observed:now,received:now,alerts:[]};assert.equal(mergePulse(base(),c,empty,now).overall,'ok');assert.equal(mergePulse(base(),c,empty,now).data_complete,true);
 assert.equal(mergePulse(base(),c,empty,now+301000).overall,'unknown');
 const r=await client({activeAlerts:10,lastUpdate:now},Array.from({length:10},(_,i)=>alarm('a'+i,i===9?'critical':'warning'))).collect();
 const h=mergePulse(base(),c,r,now);assert.equal(h.overall,'critical');assert.equal(h.categories.services.entities.length,7);assert.equal(h.categories.services.entities[1].status,'critical');assert.equal(h.categories.services.entities[0].sensors[0].metrics.more,4);
 const error=mergePulse(base(),c,{error:'PULSE_AUTH'},now);assert.equal(error.data_complete,false);assert.equal(error.categories.services.entities.length,1);
});
test('Pulse token is private, migrates old stores, survives restart, and cannot follow changed origins',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'pulse-store-'));try{const old=config();delete old.pulse;const s=await createAdminStore({directory:dir,config:old});await s.save({revision:0,config:old});
 const r=await createAdminStore({directory:dir,config:old});await assert.rejects(r.save({revision:1,config:config()}),/PULSE_TOKEN_REQUIRED/);await r.save({revision:1,config:config(),pulseToken:'synthetic-pulse-secret'});assert.equal(r.view().hasPulseToken,true);assert(!JSON.stringify(r.view()).includes('synthetic-pulse-secret'));
 const restored=await createAdminStore({directory:dir,config:old});assert.equal(restored.effective().pulseToken,'synthetic-pulse-secret');const changed=config();changed.pulse.url='https://other.example.org';await assert.rejects(restored.save({revision:2,config:changed}),/PULSE_TOKEN_REQUIRED/);
 }finally{await rm(dir,{recursive:true,force:true});}
});
test('Pulse runtime does not replace current config with late source results',async()=>{
 let finish;const r=new MonitoringRuntime({config:config(),pulseToken:'test',pulseFactory:()=>({collect:()=>new Promise(resolve=>finish=resolve)})});r.stopped=false;const pending=r.tick();const off=config();off.pulse.enabled=false;r.update({config:off});finish({observed:now,received:now,alerts:[]});r.stop();await pending;assert.equal(r.snapshot().categories.services.enabled,false);
});
