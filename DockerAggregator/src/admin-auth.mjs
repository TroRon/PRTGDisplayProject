import {scrypt, randomBytes, createHash, timingSafeEqual} from 'node:crypto';
import {promisify} from 'node:util';
import {readFile, open, rename, unlink} from 'node:fs/promises';
import {join} from 'node:path';
const derive=promisify(scrypt), hash=s=>createHash('sha256').update(s).digest();
export async function createAdminAuth({directory, token, now=Date.now}) {
 const file=join(directory,'password.json'),sessions=new Map();let saved=null,busy=false;
 try{saved=JSON.parse(await readFile(file,'utf8'));if(saved.schema!==1||! /^[a-f0-9]{32}$/.test(saved.salt)||! /^[a-f0-9]{128}$/.test(saved.hash))throw Error('PASSWORD_STATE');}catch(e){if(e.code!=='ENOENT')throw e;}
 const configured=()=>!!saved;
 const tokenOK=value=>typeof value==='string'&&timingSafeEqual(hash(value),hash(token));
 async function passwordOK(value){return !!saved&&typeof value==='string'&&Buffer.byteLength(value)<=256&&timingSafeEqual(await derive(value,saved.salt,64),Buffer.from(saved.hash,'hex'));}
 function prune(){for(const [key,s] of sessions)if(s.idle<=now()||s.until<=now())sessions.delete(key);}
 function issue(){prune();if(sessions.size>=32)sessions.delete(sessions.keys().next().value);const value=randomBytes(32).toString('hex');sessions.set(hash(value).toString('hex'),{idle:now()+900000,until:now()+28800000});return value;}
 return {
  configured,
  async login(value){if(busy)throw Error('AUTH_BUSY');busy=true;try{if(!(saved?await passwordOK(value):tokenOK(value)))return null;return issue();}finally{busy=false;}},
  authorize(header){const value=header?.startsWith('Bearer ')?header.slice(7):'';if(tokenOK(value))return true;prune();const session=sessions.get(hash(value).toString('hex'));if(!session)return false;session.idle=now()+900000;return true;},
  logout(header){sessions.delete(hash(header?.slice(7)||'').toString('hex'));},
  async change({currentPassword,password}){
   if(busy)throw Error('AUTH_BUSY');busy=true;let temporary;
   try{
    if(saved&&!await passwordOK(currentPassword))throw Error('PASSWORD_CURRENT');
    if(typeof password!=='string'||[...password].length<5||Buffer.byteLength(password)>256||/[\x00-\x1f\x7f]/.test(password)||tokenOK(password))throw Error('PASSWORD_POLICY');
    const salt=randomBytes(16).toString('hex'),next={schema:1,salt,hash:(await derive(password,salt,64)).toString('hex')};
    temporary=join(directory,'.password-'+randomBytes(12).toString('hex')+'.tmp');const f=await open(temporary,'wx',0o660);
    try{await f.writeFile(JSON.stringify(next)+'\n');await f.sync();}finally{await f.close();}
    await rename(temporary,file);temporary=null;saved=next;sessions.clear();return issue();
   }finally{if(temporary)await unlink(temporary).catch(()=>{});busy=false;}
  }
 };
}
