import {readFile,open,rename,unlink} from 'node:fs/promises';
import {join} from 'node:path';
import {createHash,randomUUID,timingSafeEqual} from 'node:crypto';
const hash=v=>createHash('sha256').update(v).digest();
const keys=['name','rotate','rotateSeconds','night','nightStart','nightEnd','brightness','utcOffsetMinutes','interrupt','history','favoriteHome','favorites'];
export function validateDisplaySettings(s){
 if(!s||typeof s!=='object'||Array.isArray(s)||Object.keys(s).length!==keys.length||keys.some(k=>!Object.hasOwn(s,k)))throw Error('DISPLAY_SETTINGS');
 if(typeof s.name!=='string'||!s.name.trim()||Buffer.byteLength(s.name)>48||/[\x00-\x1f\x7f]/.test(s.name))throw Error('DISPLAY_SETTINGS');
 for(const k of ['rotate','night','interrupt','history','favoriteHome'])if(typeof s[k]!=='boolean')throw Error('DISPLAY_SETTINGS');
 for(const [k,min,max] of [['rotateSeconds',10,60],['nightStart',0,23],['nightEnd',0,23],['brightness',10,100],['utcOffsetMinutes',-720,840]])if(!Number.isInteger(s[k])||s[k]<min||s[k]>max)throw Error('DISPLAY_SETTINGS');
 if(!Array.isArray(s.favorites)||s.favorites.length>24||new Set(s.favorites).size!==s.favorites.length||s.favorites.some(x=>typeof x!=='string'||!/^[-a-zA-Z0-9_]{1,63}$/.test(x)))throw Error('DISPLAY_SETTINGS');
 return Object.fromEntries(keys.map(k=>[k,structuredClone(s[k])]));
}
export async function readDisplayBody(req){const chunks=[];let size=0;for await(const chunk of req){size+=chunk.length;if(size>8192)throw Error('DISPLAY_REQUEST');chunks.push(Buffer.from(chunk));}try{return JSON.parse(Buffer.concat(chunks).toString('utf8'));}catch{throw Error('DISPLAY_REQUEST');}}
export async function createDisplayStore(directory,{now=()=>Date.now()}={}){
 const path=join(directory,'displays.json');let rows=[];const reports=new Map();let busy=false;
 try{const saved=JSON.parse(await readFile(path,'utf8'));if(saved.schema!==1||!Array.isArray(saved.displays)||saved.displays.length>32)throw Error('DISPLAY_STORAGE');rows=saved.displays;
 for(const d of rows){if(!/^[a-f0-9]{32}$/.test(d.id)||!/^[a-f0-9]{64}$/.test(d.keyHash)||!Number.isSafeInteger(d.revision)||d.revision<0||!['local','central'].includes(d.mode))throw Error('DISPLAY_STORAGE');validateDisplaySettings(d.settings);}
 if(new Set(rows.map(d=>d.id)).size!==rows.length)throw Error('DISPLAY_STORAGE');
 }catch(e){if(e.code!=='ENOENT')throw e;}
 async function transaction(fn){if(busy)throw Error('DISPLAY_BUSY');busy=true;let temp;try{const next=structuredClone(rows);const result=fn(next);temp=path+'.'+randomUUID()+'.tmp';const f=await open(temp,'wx',0o660);try{await f.writeFile(JSON.stringify({schema:1,displays:next})+'\n');await f.sync();}finally{await f.close();}await rename(temp,path);temp=null;rows=next;return result;}finally{if(temp)await unlink(temp).catch(()=>{});busy=false;}}
 return {
  list(){return rows.map(d=>{const r=reports.get(d.id);return {id:d.id,revision:d.revision,mode:d.mode,settings:d.settings,reported:r?.settings??null,firmware:r?.firmware??null,lastSeen:r?.at??null,state:!r||now()-r.at>180000?'offline':r.error?'error':r.revision===d.revision&&r.mode===d.mode&&(d.mode==='local'||JSON.stringify(r.settings)===JSON.stringify(d.settings))?'applied':'pending',error:r?.error??''};});},
  async update(id,input){if(!input||Object.keys(input).some(k=>!['revision','mode','settings'].includes(k))||!['local','central'].includes(input.mode))throw Error('DISPLAY_REQUEST');const settings=validateDisplaySettings(input.settings);return transaction(next=>{const d=next.find(x=>x.id===id);if(!d)throw Error('DISPLAY_NOT_FOUND');if(input.revision!==d.revision)throw Error('DISPLAY_CONFLICT');if(d.revision>=0xffffffff)throw Error('DISPLAY_CONFLICT');d.revision++;d.mode=input.mode;d.settings=settings;return {revision:d.revision};});},
  async sync(input,key){
   if(!input||Object.keys(input).some(k=>!['id','firmware','revision','mode','settings','error','release'].includes(k))||!/^[a-f0-9]{32}$/.test(input.id)||typeof key!=='string'||!/^[a-f0-9]{64}$/.test(key)||!/^\d+\.\d+\.\d+$/.test(input.firmware)||!Number.isSafeInteger(input.revision)||input.revision<0||!['local','central'].includes(input.mode)||!['','STORAGE','MEMORY','INVALID'].includes(input.error)||typeof input.release!=='boolean')throw Error('DISPLAY_REQUEST');
   const settings=validateDisplaySettings(input.settings);let d=rows.find(x=>x.id===input.id);
   if(!d){await transaction(next=>{if(next.length>=32)throw Error('DISPLAY_FULL');next.push({id:input.id,keyHash:hash(key).toString('hex'),revision:0,mode:'local',settings});});d=rows.find(x=>x.id===input.id);}
   if(!timingSafeEqual(Buffer.from(d.keyHash,'hex'),hash(key)))throw Error('DISPLAY_AUTH');
   if(input.release&&d.mode!=='local'){await transaction(next=>{const row=next.find(x=>x.id===input.id);if(row.revision>=0xffffffff)throw Error('DISPLAY_CONFLICT');row.mode='local';row.settings=settings;row.revision++;});d=rows.find(x=>x.id===input.id);}
   reports.set(d.id,{at:now(),firmware:input.firmware,revision:input.revision,mode:input.mode,settings,error:input.error});
   return {schema:1,revision:d.revision,mode:d.mode,settings:d.settings};
  }
 };
}
