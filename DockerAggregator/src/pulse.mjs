import {createHash} from 'node:crypto';
import {overallStatus} from './health.mjs';
export const pulseDefaults=()=>({enabled:false,url:'',label:'Pulse',allowHttp:false});
export function validatePulse(p){
 if(!p||Object.keys(p).some(k=>!['enabled','url','label','allowHttp'].includes(k))||typeof p.enabled!=='boolean'||typeof p.allowHttp!=='boolean'||typeof p.url!=='string'||typeof p.label!=='string'||!p.label.trim()||Buffer.byteLength(p.label)>48||/[\x00-\x1f\x7f]/.test(p.label))throw Error('CONFIG_PULSE');
 if(!p.url&&!p.enabled)return p;
 let u;try{u=new URL(p.url);}catch{throw Error('CONFIG_PULSE');}
 if(!['https:','http:'].includes(u.protocol)||u.username||u.password||u.search||u.hash||u.pathname!=='/'||u.protocol==='http:'&&!p.allowHttp)throw Error('CONFIG_PULSE');
 return p;
}
const stamp=v=>typeof v==='number'&&Number.isFinite(v)&&v>1e12?v:typeof v==='string'?Date.parse(v):NaN;
const text=(v,max)=>Array.from(v.replace(/[\x00-\x1f\x7f]/g,' ')).slice(0,max).join('');
export class PulseClient {
 constructor(config,token,{fetchImpl=fetch,clock=Date.now}={}){this.config=config;this.token=token;this.fetch=fetchImpl;this.clock=clock;}
 async get(path){
  const response=await this.fetch(new URL(path,this.config.pulse.url),{headers:{'X-API-Token':this.token,Accept:'application/json'},redirect:'error',signal:AbortSignal.timeout(Math.min(this.config.request_timeout_seconds,30)*1000)});
  if(!response.ok)throw Error(response.status===401||response.status===403?'PULSE_AUTH':'PULSE_HTTP');
  if(!response.headers.get('content-type')?.includes('application/json'))throw Error('PULSE_SHAPE');
  const chunks=[];let size=0;
  for await(const chunk of response.body){size+=chunk.length;if(size>524288)throw Error('PULSE_SIZE');chunks.push(Buffer.from(chunk));}
  try{return JSON.parse(Buffer.concat(chunks).toString('utf8'));}catch{throw Error('PULSE_JSON');}
 }
 async collect(){
  if(!this.token)return {error:'PULSE_NOT_CONFIGURED'};
  try{
   const [summary,alerts]=await Promise.all([this.get('/api/state/summary'),this.get('/api/alerts/active')]);
   const observed=stamp(summary.lastUpdate),now=this.clock();
   if(!Number.isFinite(observed)||observed>now+60000||now-observed>this.config.stale_seconds*1000)throw Error('PULSE_STALE');
   if(!Array.isArray(alerts)||alerts.length>512||!Number.isInteger(summary.activeAlerts)||summary.activeAlerts!==alerts.length)throw Error('PULSE_SHAPE');
   const ids=new Set();
   const clean=alerts.map(a=>{if(!a||typeof a.id!=='string'||!a.id||ids.has(a.id)||typeof a.level!=='string'||typeof a.resourceName!=='string'||typeof a.message!=='string'||typeof a.acknowledged!=='boolean'||!Number.isFinite(stamp(a.startTime)))throw Error('PULSE_SHAPE');ids.add(a.id);return {id:'pulse-'+createHash('sha256').update(a.id).digest('hex').slice(0,24),status:['critical','warning'].includes(a.level)?a.level:'unknown',label:text(a.resourceName,48),message:text(a.message,220),since:new Date(stamp(a.startTime)).toISOString(),acknowledged:a.acknowledged};});
   clean.sort((a,b)=>['critical','unknown','warning'].indexOf(a.status)-['critical','unknown','warning'].indexOf(b.status)||a.since.localeCompare(b.since)||a.id.localeCompare(b.id));
   return {observed,received:now,alerts:clean};
  }catch(e){return {error:['PULSE_AUTH','PULSE_HTTP','PULSE_SHAPE','PULSE_SIZE','PULSE_JSON','PULSE_STALE'].includes(e.message)?e.message:'PULSE_UNAVAILABLE'};}
 }
}
export function mergePulse(health,config,record,now=Date.now()){
 if(!config.pulse?.enabled)return health;
 const error=record?.error||(!record?'PULSE_WAITING':now-record.observed>config.stale_seconds*1000||now-record.received>config.stale_seconds*1000?'PULSE_STALE':null);
 const alerts=error?[]:record.alerts,counts={critical:0,warning:0,unknown:0};for(const a of alerts)counts[a.status]++;
 const status=error?'unknown':alerts.length?overallStatus(alerts.map(a=>a.status)):'ok';
 const observed=error?null:new Date(record.observed).toISOString();
 const add=(id,label,state,message,metrics={},reason=null)=>({id,label,status:state,sensors:[{key:'pulse',sensor_id:null,status:state,reason,observed_at:observed,metrics,message}]});
 const existing=Object.values(health.categories).reduce((n,c)=>n+c.entities.length,0);
 const slots=Math.min(6,Math.max(0,24-existing-1));
 const shown=alerts.slice(0,slots);
 const summary=add('pulse-overview',config.pulse.label,status,error?'Pulse-Daten nicht verfügbar oder veraltet.':alerts.length?`${alerts.length} aktive Alarme; ${shown.length} Detailkarten. Quittiert bedeutet weiterhin aktiv.`:'Keine aktiven Pulse-Alarme.',error?{}:{critical:counts.critical,warning:counts.warning,unknown:counts.unknown,active:alerts.length,more:alerts.length-shown.length},error);
 const entities=[summary,...shown.map(a=>add(a.id,'Pulse: '+a.label,a.status,`${a.message}\nSeit ${a.since}${a.acknowledged?' · Quittiert':''}`))];
 const group=health.categories.services;group.entities.push(...entities);group.enabled=true;group.reason=null;group.entities_total=group.entities.length;group.entities_ok=group.entities.filter(e=>e.status==='ok').length;group.status=overallStatus(group.entities.map(e=>e.status));
 // One overview alarm avoids duplicating all incidents in the limited panel hint list.
 if(status!=='ok')health.alerts.push({id:'pulse-overview',category:'services',entity:'pulse-overview',label:config.pulse.label,status,reason:error||'PULSE_ALERTS',message:summary.sensors[0].message,sensor:'pulse'});
 health.overall=overallStatus(Object.values(health.categories).filter(c=>c.enabled).map(c=>c.status));health.data_complete=health.data_complete&&!error&&!counts.unknown;
 // Pulse-only setups are valid as well.
 if(existing===0)health.data_complete=!error&&!counts.unknown;
 health.alerts.sort((a,b)=>['critical','unknown','warning'].indexOf(a.status)-['critical','unknown','warning'].indexOf(b.status));
 return health;
}
