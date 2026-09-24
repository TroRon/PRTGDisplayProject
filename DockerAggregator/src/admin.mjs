import {createHash, timingSafeEqual} from 'node:crypto';
import {readFile} from 'node:fs/promises';

const hash = value => createHash('sha256').update(value).digest();
const errors = new Set(['SAVE_BUSY','CONFIG_CONFLICT','CONFIG_REQUEST','CONFIG_ENTITIES','CONFIG_TIMING','CONFIG_STALE_WINDOW','CONFIG_PRTG_HTTPS_ORIGIN','CONFIG_UNKNOWN_FIELD','CONFIG_ENTITY','CONFIG_SENSOR_KEY','CONFIG_SENSOR_ID','CONFIG_METRICS','CONFIG_TOO_LARGE','PRTG_TOKEN_REQUIRED','PRTG_TOKEN_INVALID']);
export function createAdmin({origin, token, panelToken, store, runtime, firmware, log=()=>{}}) {
  const url=new URL(origin);
  if(url.origin!==origin || url.username || url.password || !(url.protocol==='https:' || (url.protocol==='http:' && ['127.0.0.1','localhost','[::1]'].includes(url.hostname))) || !token || token.length<32 || token===panelToken) throw Error('ADMIN_CONFIG');
  const expected=hash(`Bearer ${token}`), attempts=new Map();
  const headers={'Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer','Content-Security-Policy':"default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'"};
  const send=(res,code,value)=>{res.writeHead(code,{...headers,'Content-Type':'application/json; charset=utf-8'});res.end(JSON.stringify(value));};
  const assets={'/admin':'admin.html','/admin/':'admin.html','/admin.js':'admin.js','/admin.css':'admin.css'};
  async function body(req){
    if(req.headers['content-type']!=='application/json')throw Error('CONFIG_REQUEST');
    let size=0;const chunks=[];
    for await(const chunk of req){size+=chunk.length;if(size>131072)throw Error('CONFIG_TOO_LARGE');chunks.push(chunk);}
    try{return JSON.parse(Buffer.concat(chunks));}catch{throw Error('CONFIG_REQUEST');}
  }
  return async(req,res)=>{
    // Only the configured HTTPS origin, never untrusted X-Forwarded-* values.
    if(req.headers.host!==url.host || (req.headers.origin && req.headers.origin!==origin) || (!['GET','HEAD'].includes(req.method)&&req.headers.origin!==origin))return send(res,403,{error:'ORIGIN_DENIED'});
    if(req.method==='GET'&&assets[req.url]){
      const name=assets[req.url];res.writeHead(200,{...headers,'Content-Type':name.endsWith('.js')?'text/javascript; charset=utf-8':name.endsWith('.css')?'text/css; charset=utf-8':'text/html; charset=utf-8'});
      res.end(await readFile(new URL('./'+name,import.meta.url)));return;
    }
    const peer=req.socket.remoteAddress||'',now=Date.now();
    for(const [key,value] of attempts)if(value.until<=now)attempts.delete(key);
    if(attempts.get(peer)?.count>=10)return send(res,429,{error:'LOGIN_RATE_LIMIT'});
    if(!timingSafeEqual(expected,hash(req.headers.authorization||''))){
      if(attempts.size>=256&&!attempts.has(peer))return send(res,429,{error:'LOGIN_RATE_LIMIT'});
      const previous=attempts.get(peer);attempts.set(peer,{count:(previous?.count||0)+1,until:previous?.until||now+60000});return send(res,401,{error:'UNAUTHORIZED'});
    }
    attempts.delete(peer);
    if(req.url==='/api/admin/config'&&req.method==='GET')return send(res,200,store.view());
    if(req.url==='/api/admin/status'&&req.method==='GET'){
      let versions=[],firmwareError=false;
      try{if(firmware)versions=await firmware.list();}catch{firmwareError=true;}
      return send(res,200,{health:runtime.snapshot(),versions,firmwareEnabled:!!firmware,firmwareError});
    }
    if(req.url==='/api/admin/config'&&req.method==='PUT'){
      try{const result=await store.save(await body(req));log('INFO',`Admin: Konfiguration Revision ${result.revision} gespeichert und übernommen.`);return send(res,200,result);}
      catch(e){const code=errors.has(e.message)?e.message:'SAVE_FAILED';log('WARN','Admin: Konfiguration nicht übernommen.');return send(res,code==='CONFIG_CONFLICT'||code==='SAVE_BUSY'?409:400,{error:code});}
    }
    if(req.url==='/api/admin/prtg-test'&&req.method==='POST'){
      if(runtime.demo)return send(res,200,{message:'Demo-Modus: keine Verbindung zu PRTG aufgebaut.'});
      if(!runtime.current.hasPrtgToken)return send(res,400,{error:'PRTG_TOKEN_REQUIRED'});
      try{const rows=await runtime.current.client.table('sensors',{columns:'objid',count:'1'});return send(res,200,{message:rows.length?'PRTG erreichbar; API-Zugang und Sensor-Leserecht bestätigt.':'PRTG erreichbar; für diesen Benutzer sind keine Sensoren sichtbar.'});}
      catch{return send(res,502,{error:'PRTG_TEST_FAILED'});}
    }
    if(req.url==='/api/admin/firmware'&&req.method==='PUT'){
      if(!firmware)return send(res,503,{error:'FIRMWARE_DISABLED'});
      // Same explicit administrator credential, not the panel token; reuse signature/size/storage checks.
      req.url='/api/v1/firmware/upload';return firmware(req,res);
    }
    send(res,404,{error:'NOT_FOUND'});
  };
}
