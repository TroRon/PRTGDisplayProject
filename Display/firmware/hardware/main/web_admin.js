'use strict';
const $=id=>document.getElementById(id);
let csrf='',state=null,loaded=false,scanRevision=-1,catalogRevision=-1,pendingInstall=null,polling=false;
function notice(message,error=false){$('notice').hidden=false;$('notice').textContent=message;$('notice').classList.toggle('error',error);}
function signedOut(){csrf='';loaded=false;state=null;pendingInstall=null;$('app').hidden=true;$('logout').hidden=true;$('login').hidden=false;$('confirmation').close();for(const id of ['loginPassword','wifiPassword','panelToken','currentPassword','newPassword','repeatPassword'])$(id).value='';}
async function request(path,value){
 const options={cache:'no-store',credentials:'same-origin',signal:AbortSignal.timeout(10000)};
 if(value!==undefined){options.method='POST';options.headers={'Content-Type':'application/json','X-CSRF-Token':csrf};options.body=JSON.stringify(value);}
 let response;try{response=await fetch(path,options);}catch{throw Error('Display nicht erreichbar. Bei WLAN-Wechsel oder Neustart die IP-Adresse am Display prüfen.');}
 const result=await response.json();if(!response.ok){if(response.status===401||response.status===403)signedOut();throw Error(result.message||'Anfrage fehlgeschlagen.');}return result;
}
async function action(value){return request('/api/action',value);}
function channel(){return {direct:$('channel').value==='direct',url:$('manifest').value.trim()};}
function paint(s){
 state=s;csrf=s.csrf;$('app').hidden=false;$('login').hidden=true;$('logout').hidden=false;
 $('heading').textContent=s.name;$('subtitle').textContent=`Firmware ${s.version} · ${s.ip}`;
 $('liveStatus').textContent=`WLAN: ${s.connected?'verbunden':'getrennt'} · ${s.ssid}\nAPI: ${s.api} · PRTG: ${s.source}\n${s.message}`;
 if(!loaded){for(const [id,value] of Object.entries({ssid:s.ssid,ntp:s.ntp,panelName:s.name,origin:s.origin,manifest:s.ota.url}))$(id).value=value;$('channel').value=s.ota.direct?'direct':'aggregator';loaded=true;catalogRevision=-1;scanRevision=-1;}
 $('scanStatus').textContent=s.scan.message;$('scan').disabled=s.scan.busy||s.ota.busy||s.ota.pending;
 if(scanRevision!==s.scan.revision){scanRevision=s.scan.revision;$('networks').replaceChildren();for(const n of s.scan.networks){const b=document.createElement('button');b.type='button';b.textContent=`${n.ssid} · ${n.signal} dBm${n.supported?'':' · nicht unterstützt'}`;b.disabled=!n.supported;b.addEventListener('click',()=>{if($('ssid').value!==n.ssid)$('wifiPassword').value='';$('ssid').value=n.ssid;$('wifiPassword').focus();});$('networks').append(b);}}
 $('otaStatus').textContent=`${s.ota.message}\nFortschritt: ${s.ota.progress} %${s.ota.pending?` · Bestätigung erforderlich: ${s.ota.seconds} s`:''}`;
 if(catalogRevision!==s.ota.revision){catalogRevision=s.ota.revision;pendingInstall=null;$('confirmation').close();$('versions').replaceChildren();for(const v of s.ota.versions){const o=document.createElement('option');o.value=v.sequence;o.textContent=`${v.version} · ${v.action}`;$('versions').append(o);}if(!s.ota.versions.length){const o=document.createElement('option');o.value='';o.textContent='Zuerst Versionen prüfen';$('versions').append(o);}}
 const blocked=s.ota.busy||s.ota.pending;$('install').disabled=blocked||!s.ota.ready||!s.ota.versions.length;$('check').disabled=blocked||!s.ota.ready;$('versions').disabled=blocked;$('keep').hidden=!s.ota.pending;$('keep').disabled=s.ota.busy;
 for(const form of ['wifiForm','panelForm','channelForm','passwordForm'])for(const control of $(form).elements)control.disabled=blocked;
 $('manifest').disabled=blocked||$('channel').value!=='direct';
}
async function refresh(){if(polling)return;polling=true;try{paint(await request('/api/status'));}finally{polling=false;}}
function bind(id,event,handler){$(id).addEventListener(event,async e=>{e.preventDefault();try{await handler(e);}catch(error){notice(error.message,true);}});}
bind('loginForm','submit',async()=>{const password=$('loginPassword').value;$('loginPassword').value='';await request('/api/login',{user:'admin',password});loaded=false;await refresh();notice('Angemeldet.');});
bind('logout','click',async()=>{await action({action:'logout'});signedOut();notice('Abgemeldet.');});
bind('scan','click',async()=>{await action({action:'scan'});await refresh();});
bind('wifiForm','submit',async()=>{const value={action:'wifi',ssid:$('ssid').value,password:$('wifiPassword').value,token:$('panelToken').value,ntp:$('ntp').value.trim()};const r=await action(value);$('wifiPassword').value='';$('panelToken').value='';notice(r.message);});
bind('panelForm','submit',async()=>{notice((await action({action:'panel',name:$('panelName').value,origin:$('origin').value.trim().replace(/\/$/,'')})).message);await refresh();});
bind('channel','change',()=>{$('manifest').disabled=$('channel').value!=='direct';pendingInstall=null;$('confirmation').close();});
bind('channelForm','submit',async()=>{notice((await action({action:'channel',...channel()})).message);await refresh();});
bind('check','click',async()=>{notice((await action({action:'check',...channel()})).message);await refresh();});
bind('install','click',()=>{if(!state)return;const v=state.ota.versions.find(v=>v.sequence===Number($('versions').value));if(!v)return;pendingInstall={revision:state.ota.revision,sequence:v.sequence};$('installSummary').textContent=`${v.action}: ${state.version} → ${v.version}. ${v.action==='Downgrade'?'Ältere Versionen können weniger Funktionen bieten.':''}`;$('confirmation').showModal();});
bind('cancelInstall','click',()=>{pendingInstall=null;$('confirmation').close();});
bind('confirmInstall','click',async()=>{const target=pendingInstall;if(!target)return;pendingInstall=null;$('confirmation').close();notice((await action({action:'install',...target,confirm:true})).message);await refresh();});
bind('keep','click',async()=>{notice((await action({action:'keep'})).message);await refresh();});
bind('passwordForm','submit',async()=>{const next=$('newPassword').value;if(next!==$('repeatPassword').value)throw Error('Die neuen Passwörter stimmen nicht überein.');const r=await action({action:'password',current:$('currentPassword').value,password:next});signedOut();notice(r.message);});
setInterval(()=>{if(csrf)refresh().catch(e=>notice(e.message,true));},3000);
refresh().catch(()=>signedOut());
