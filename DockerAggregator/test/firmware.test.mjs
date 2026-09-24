import {test} from 'node:test';
import assert from 'node:assert/strict';
import {generateKeyPairSync,sign,createHash} from 'node:crypto';
import {mkdtemp,rm,writeFile,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {createFirmwareService,validatePackage} from '../src/firmware.mjs';
import {createApi} from '../src/server.mjs';
const keys=generateKeyPairSync('rsa',{modulusLength:2048});
function pack(changes={},imageVersion='0.6.1',marker=0){
 const image=Buffer.alloc(2048,0);image[1024]=marker;image[0]=0xe9;image.writeUInt16LE(9,12);image.writeUInt32LE(0xabcd5432,32);image.write(imageVersion,48);image.write('eaglenet_lcd5b',80);
 const manifest={schema:1,board:'waveshare-lcd5b-28151',layout:'eaglenet-ota-v1',version:'0.6.1',sequence:601,size:image.length,sha256:createHash('sha256').update(image).digest('hex'),...changes};
 const payload=Buffer.from(JSON.stringify(manifest));
 const envelope=Buffer.from(JSON.stringify({payload:payload.toString('base64'),signature:sign('RSA-SHA256',payload,keys.privateKey).toString('base64')}));
 const header=Buffer.alloc(4);header.writeUInt32BE(envelope.length);return Buffer.concat([header,envelope,image]);
}
test('OTA verifies signature, board/layout, image descriptor, size and hash',()=>{
 assert.equal(validatePackage(pack(),keys.publicKey).manifest.version,'0.6.1');
 for(const changes of [{board:'simulator'},{layout:'old'},{size:4096},{sha256:'0'.repeat(64)},{version:'0.6.2'},{sequence:-1},{sequence:2**32}])assert.throws(()=>validatePackage(pack(changes),keys.publicKey));
 const bad=pack();bad[bad.length-1]^=1;assert.throws(()=>validatePackage(bad,keys.publicKey));
 assert.throws(()=>validatePackage(pack().subarray(0,100),keys.publicKey));
 assert.throws(()=>validatePackage(pack())); // Production key rejects the ephemeral test signer.
 const sig=pack();sig[100]^=1;assert.throws(()=>validatePackage(sig,keys.publicKey));
});
test('OTA routes separate admin/panel access, atomic publish, health remains available',async()=>{
 const directory=await mkdtemp(join(tmpdir(),'eagle-ota-'));
 const adminToken='a'.repeat(40),panelToken='p'.repeat(32),events=[];
 const firmware=createFirmwareService({directory,adminToken,panelToken,key:keys.publicKey,log:(...args)=>events.push(args.join(' '))});
 const server=createApi({snapshot:()=>({status:'test'})},panelToken,new Set(['127.0.0.1']),firmware);
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 const base=`http://127.0.0.1:${server.address().port}`;
 const request=(path,token,method='GET',body)=>fetch(base+path,{method,headers:token?{Authorization:`Bearer ${token}`}:{},body});
 try{
  assert.equal((await request('/api/v1/firmware/manifest.json',panelToken)).status,404);
  assert.equal((await request('/api/v1/firmware/upload',panelToken,'PUT',pack())).status,401);
  assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',pack({board:'wrong'}))).status,400);
  assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',pack())).status,200);
  assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',pack())).status,200);
  assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',pack({},'0.6.1',1))).status,409);
  assert.equal((await request('/api/v1/firmware/catalog.json')).status,401);
  const older=pack({version:'0.6.0',sequence:600},'0.6.0');
  assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',older)).status,200);
  const catalog=await (await request('/api/v1/firmware/catalog.json',panelToken)).json();
  assert.deepEqual(catalog.releases.map(e=>JSON.parse(Buffer.from(e.payload,'base64')).version),['0.6.1','0.6.0']);
  const oldManifest=validatePackage(older,keys.publicKey).manifest;
  assert.equal((await request(`/api/v1/firmware/${oldManifest.sha256}.bin`,panelToken)).status,200);
  assert.equal((await request('/api/v1/firmware/manifest.json')).status,401);
  const m=await request('/api/v1/firmware/manifest.json',panelToken);assert.equal(m.status,200);
  const payload=JSON.parse(Buffer.from((await m.json()).payload,'base64'));
  assert.equal(payload.version,'0.6.1'); // Older uploads never downgrade the legacy latest endpoint.
  const image=await request(`/api/v1/firmware/${payload.sha256}.bin`,panelToken);assert.equal(image.status,200);
  assert.equal(createHash('sha256').update(Buffer.from(await image.arrayBuffer())).digest('hex'),payload.sha256);
  assert.equal((await request('/api/v1/health',panelToken)).status,200);
  assert.equal((await request('/api/v1/firmware/'+'0'.repeat(64)+'.bin',panelToken)).status,404);
  assert.equal((await request('/updates')).status,200);
  assert(!events.join().includes(adminToken));assert(!events.join().includes(panelToken));
  for(let patch=2;patch<8;patch++)assert.equal((await request('/api/v1/firmware/upload',adminToken,'PUT',pack({version:`0.6.${patch}`,sequence:600+patch},`0.6.${patch}`))).status,200);
  const full=await request('/api/v1/firmware/upload',adminToken,'PUT',pack({version:'0.6.8',sequence:608},'0.6.8'));
  assert.equal(full.status,409);assert.equal((await full.json()).error,'CATALOG_FULL');
 }finally {server.closeAllConnections();await new Promise(resolve=>server.close(resolve));await rm(directory,{recursive:true,force:true});}
});
test('OTA cannot reuse panel token as administrator token',()=>{
 assert.throws(()=>createFirmwareService({directory:'unused',adminToken:'a'.repeat(40),panelToken:'a'.repeat(40)}));
});

test('legacy single-package store survives catalog migration and remains downloadable',async()=>{
 const directory=await mkdtemp(join(tmpdir(),'ota-migrate-'));
 const old=pack(),oldMeta=validatePackage(old,keys.publicKey).manifest;
 await writeFile(join(directory,'current.eagleota'),old);
 const adminToken='a'.repeat(40),panelToken='p'.repeat(32);
 const server=createApi({snapshot:()=>({})},panelToken,new Set(['127.0.0.1']),createFirmwareService({directory,adminToken,panelToken,key:keys.publicKey}));
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
 const url=`http://127.0.0.1:${server.address().port}/api/v1/firmware/`;
 try{
  const response=await fetch(url+'upload',{method:'PUT',headers:{Authorization:`Bearer ${adminToken}`},body:pack({version:'0.8.0',sequence:800},'0.8.0')});
  assert.equal(response.status,200);
  assert.deepEqual(await readFile(join(directory,'releases',oldMeta.sha256+'.eagleota')),old);
  const catalog=await (await fetch(url+'catalog.json',{headers:{Authorization:`Bearer ${panelToken}`}})).json();
  assert.deepEqual(catalog.releases.map(e=>JSON.parse(Buffer.from(e.payload,'base64')).version),['0.8.0','0.6.1']);
 }finally{server.closeAllConnections();await new Promise(resolve=>server.close(resolve));await rm(directory,{recursive:true,force:true});}
});
