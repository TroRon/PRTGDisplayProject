import {validateEnvelope,validatePackage} from './firmware.mjs';
const DEFAULT='https://raw.githubusercontent.com/TroRon/PRTGDisplayProject/main/Display/ota/catalog.json';
export function createGithubFirmware({url=DEFAULT,fetchImpl=fetch,key}={}) {
 const source=new URL(url);
 if(source.origin!=='https://raw.githubusercontent.com'||source.username||source.password||source.search||source.hash||!/^\/[\w.-]+\/[\w.-]+\/[\w./-]+\/catalog\.json$/.test(source.pathname))throw Error('GITHUB_SOURCE');
 let busy=false;
 async function bytes(address,limit){
  const response=await fetchImpl(address,{redirect:'error',signal:AbortSignal.timeout(45000)});
  if(!response.ok||Number(response.headers.get('content-length'))>limit){await response.body?.cancel();throw Error('GITHUB_HTTP');}
  let size=0;const chunks=[];for await(const chunk of response.body){size+=chunk.length;if(size>limit)throw Error('GITHUB_SIZE');chunks.push(Buffer.from(chunk));}return Buffer.concat(chunks);
 }
 async function catalog(){
  const data=JSON.parse(await bytes(source,16384));if(data.schema!==1||!Array.isArray(data.releases)||data.releases.length>8)throw Error('GITHUB_CATALOG');
  const seen=new Set();return data.releases.map(wrapper=>{const envelope=Buffer.from(JSON.stringify(wrapper));if(envelope.length>8192)throw Error('GITHUB_SIZE');const manifest=validateEnvelope(envelope,key);if(seen.has(manifest.sequence))throw Error('GITHUB_DUPLICATE');seen.add(manifest.sequence);return {envelope,manifest};}).sort((a,b)=>b.manifest.sequence-a.manifest.sequence);
 }
 async function exclusive(work){if(busy)throw Error('GITHUB_BUSY');busy=true;try{return await work();}finally{busy=false;}}
 return {
  list:()=>exclusive(async()=>(await catalog()).map(p=>p.manifest)),
  download:sha=>exclusive(async()=>{
   if(typeof sha!=='string'||! /^[a-f0-9]{64}$/.test(sha))throw Error('GITHUB_HASH');
   const release=(await catalog()).find(p=>p.manifest.sha256===sha);if(!release)throw Error('GITHUB_CHANGED');
   const image=await bytes(new URL(sha+'.bin',source),release.manifest.size),header=Buffer.alloc(4);header.writeUInt32BE(release.envelope.length);
   const data=Buffer.concat([header,release.envelope,image]);validatePackage(data,key);return data;
  })
 };
}
