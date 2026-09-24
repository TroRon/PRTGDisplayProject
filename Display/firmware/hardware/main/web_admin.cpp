#include "web_admin.h"
#include "web_policy.h"
#include "memory_diagnostics.h"
#include "live.h"
#include "preferences.h"
#include "ota_catalog.h"
#include "provision_policy.h"
#include "Arduino.h"
#include "esp_http_server.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "esp_log.h"
#include "nvs.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/platform_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
extern const char page[] asm("_binary_web_admin_html_start");
extern const char script[] asm("_binary_web_admin_js_start");
namespace webadmin {
static StaticSemaphore_t mutexStorage;
static SemaphoreHandle_t guard;
static Status state;
static httpd_handle_t server=nullptr;
static bool networkReady=false;
struct Credential {unsigned version=1;unsigned char salt[16]={},digest[32]={};};
static Credential credential;
struct Initial { Credential owner; char digits[13]={}; };
static char initialDigits[13]={};
static webpolicy::Session session;
static webpolicy::LoginLimit attempts;
static void lock(){xSemaphoreTake(guard,portMAX_DELAY);}
static void unlock(){xSemaphoreGive(guard);}
struct Buffer {
 char* data;size_t size;
 explicit Buffer(size_t n):data((char*)heap_caps_calloc(1,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)),size(n){}
 ~Buffer(){if(data){mbedtls_platform_zeroize(data,size);heap_caps_free(data);}}
};
struct Allocator {
 void* allocate(size_t n){return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
 void deallocate(void* p){heap_caps_free(p);}
 void* reallocate(void* p,size_t n){return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
};
using Document=BasicJsonDocument<Allocator>;
static bool hash(const char* value,const Credential& c,unsigned char* out){
 return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,(const unsigned char*)value,strlen(value),c.salt,sizeof(c.salt),20000,32,out)==0;
}
static bool verify(const char* value){ // guard held
 if(!state.configured||!webpolicy::validPassword(value))return false;
 unsigned char digest[32];bool ok=hash(value,credential,digest)&&webpolicy::equal((char*)digest,(char*)credential.digest,32);
 mbedtls_platform_zeroize(digest,sizeof(digest));return ok;
}
void status(Status& s){if(!guard){s=Status{};return;}lock();s=state;unlock();}
void initialPassword(char (&out)[13]){out[0]=0;if(!guard)return;lock();memcpy(out,initialDigits,sizeof(initialDigits));unlock();}
static bool eraseOptional(nvs_handle_t n,const char* key){auto e=nvs_erase_key(n,key);return e==ESP_OK||e==ESP_ERR_NVS_NOT_FOUND;}
bool password(const char* value){
 if(!guard||ota::busy()||!webpolicy::validPassword(value))return false;
 Credential next;esp_fill_random(next.salt,sizeof(next.salt));
 if(!hash(value,next,next.digest))return false;
 lock();nvs_handle_t n;bool ok=false;
 if(nvs_open("eagle-web",NVS_READWRITE,&n)==ESP_OK){ok=nvs_set_blob(n,"credential",&next,sizeof(next))==ESP_OK&&nvs_set_u8(n,"autodone",1)==ESP_OK&&eraseOptional(n,"initial")&&nvs_commit(n)==ESP_OK;nvs_close(n);}
 if(ok){mbedtls_platform_zeroize(initialDigits,sizeof(initialDigits));credential=next;state.configured=true;session.clear();attempts.success();snprintf(state.message,sizeof(state.message),"Passwort gespeichert. Web-Anmeldung als admin möglich.");}
 unlock();mbedtls_platform_zeroize(&next,sizeof(next));if(ok)live::adminChanged();return ok;
}
bool disable(){
 if(!guard||ota::busy())return false;
 lock();nvs_handle_t n;bool ok=false;
 if(nvs_open("eagle-web",NVS_READWRITE,&n)==ESP_OK){ok=nvs_set_u8(n,"autodone",1)==ESP_OK&&eraseOptional(n,"initial")&&eraseOptional(n,"credential")&&nvs_commit(n)==ESP_OK;nvs_close(n);}
 if(ok){mbedtls_platform_zeroize(initialDigits,sizeof(initialDigits));state.configured=false;session.clear();mbedtls_platform_zeroize(&credential,sizeof(credential));snprintf(state.message,sizeof(state.message),"Webzugang deaktiviert. Neues Passwort aktiviert ihn wieder.");}
 unlock();return ok;
}
static void randomHex(char* out){unsigned char bytes[32];esp_fill_random(bytes,sizeof(bytes));for(unsigned i=0;i<32;++i)snprintf(out+2*i,3,"%02x",bytes[i]);mbedtls_platform_zeroize(bytes,sizeof(bytes));}
static void headers(httpd_req_t* r){
 httpd_resp_set_hdr(r,"Cache-Control","no-store");httpd_resp_set_hdr(r,"X-Content-Type-Options","nosniff");
 httpd_resp_set_hdr(r,"Content-Security-Policy","default-src 'none'; script-src 'self'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
 httpd_resp_set_hdr(r,"Referrer-Policy","no-referrer");
}
static esp_err_t reply(httpd_req_t* r,const char* code,const char* message){
 headers(r);httpd_resp_set_status(r,code);httpd_resp_set_type(r,"application/json");
 Document doc(512);doc["message"]=message;char out[512];serializeJson(doc,out,sizeof(out));return httpd_resp_send(r,out,HTTPD_RESP_USE_STRLEN);
}
static bool header(httpd_req_t* r,const char* name,char* out,size_t n){out[0]=0;return httpd_req_get_hdr_value_len(r,name)<n&&httpd_req_get_hdr_value_str(r,name,out,n)==ESP_OK;}
static bool allowedHost(httpd_req_t* r,bool write){
    live::Status net;live::status(net);char host[64],origin[80];
 live::Hotspot setup;live::hotspotStatus(setup);const char* ip=nullptr;
 if(!header(r,"Host",host,sizeof(host)))return false;
 if(net.wifi&&webpolicy::host(host,net.ip))ip=net.ip;
 else if(setup.active&&webpolicy::host(host,"192.168.4.1"))ip="192.168.4.1";
 memset(setup.password,0,sizeof(setup.password));
 return ip&&(!write||(header(r,"Origin",origin,sizeof(origin))&&webpolicy::origin(origin,ip)));
}
static bool authorized(httpd_req_t* r,bool write){
 char cookie[160],csrf[80];header(r,"Cookie",cookie,sizeof(cookie));header(r,"X-CSRF-Token",csrf,sizeof(csrf));
 lock();bool ok=state.configured&&(write?session.write(cookie,csrf,millis()):session.valid(cookie,millis()));unlock();return ok;
}
static bool readBody(httpd_req_t* r,Buffer& body,Document& doc){
 char type[64];if(!body.data||!header(r,"Content-Type",type,sizeof(type))||strcmp(type,"application/json")||r->content_len<2||r->content_len>=body.size)return false;
 size_t used=0;while(used<r->content_len){int count=httpd_req_recv(r,body.data+used,r->content_len-used);if(count<=0)return false;used+=count;}
 return !deserializeJson(doc,body.data,used,DeserializationOption::NestingLimit(4))&&doc.is<JsonObject>()&&!doc.overflowed();
}
static bool string(JsonVariantConst v,char* out,size_t capacity){
 if(!v.is<const char*>())return false;
 auto s=v.as<JsonString>();if(s.size()>=capacity||s.size()!=strlen(s.c_str()))return false;
 memcpy(out,s.c_str(),s.size()+1);return true;
}
static esp_err_t root(httpd_req_t* r){
 if(!allowedHost(r,false))return reply(r,"403 Forbidden","Bitte die IP-Adresse des Displays verwenden.");
 headers(r);httpd_resp_set_type(r,!strcmp(r->uri,"/app.js")?"text/javascript; charset=utf-8":"text/html; charset=utf-8");
 return httpd_resp_send(r,!strcmp(r->uri,"/app.js")?script:page,HTTPD_RESP_USE_STRLEN);
}
static esp_err_t login(httpd_req_t* r){
 if(!allowedHost(r,true))return reply(r,"403 Forbidden","Anfrage abgelehnt.");
 lock();bool allowed=attempts.allowed(millis());unlock();if(!allowed)return reply(r,"429 Too Many Requests","Bitte eine Minute warten und erneut anmelden.");
 Buffer body(2048);Document doc(1024);if(!readBody(r,body,doc))return reply(r,"400 Bad Request","Ungültige Anmeldung.");
 char value[64]={},user[16]={};bool decoded=doc.size()==2&&string(doc["password"],value,sizeof(value))&&string(doc["user"],user,sizeof(user));
 char cookie[160]={};lock();bool ok=decoded&&!strcmp(user,"admin")&&verify(value);
 if(ok){attempts.success();session.clear();randomHex(session.id);randomHex(session.csrf);session.issued=millis();snprintf(cookie,sizeof(cookie),"sid=%s; Path=/; HttpOnly; SameSite=Strict; Max-Age=900",session.id);}
 else attempts.failed(millis());
 unlock();mbedtls_platform_zeroize(value,sizeof(value));
 if(!ok)return reply(r,"401 Unauthorized","Anmeldung fehlgeschlagen.");
 httpd_resp_set_hdr(r,"Set-Cookie",cookie);return reply(r,"200 OK","Angemeldet.");
}
static esp_err_t snapshot(httpd_req_t* r){
 if(!allowedHost(r,false)||!authorized(r,false))return reply(r,"401 Unauthorized","Bitte anmelden.");
 Document doc(8192);Buffer out(8192);if(!out.data)return reply(r,"503 Service Unavailable","Speicher momentan belegt.");
 live::Status net;live::status(net);live::Config wifi;live::config(wifi);preferences::Config panel;preferences::get(panel);ota::Config channel;ota::config(channel);ota::Status update;ota::status(update);live::Scan scan;live::scanStatus(scan);
 lock();doc["csrf"]=session.csrf;unlock();doc["version"]=ota::Version;doc["name"]=panel.name;doc["origin"]=panel.origin;
 doc["ssid"]=wifi.ssid;doc["ntp"]=wifi.ntp;doc["hasPassword"]=bool(*wifi.password);doc["hasToken"]=bool(*wifi.token);
 doc["ip"]=net.ip;doc["connected"]=net.wifi;doc["message"]=net.message;doc["api"]=live::apiText(net.api);doc["source"]=live::sourceText(net.source);
 auto sc=doc.createNestedObject("scan");sc["busy"]=scan.busy;sc["message"]=scan.message;sc["revision"]=scan.revision;auto networks=sc.createNestedArray("networks");
 for(unsigned i=0;i<scan.count;++i){auto row=networks.createNestedObject();row["ssid"]=scan.networks[i].ssid;row["signal"]=scan.networks[i].signal;row["supported"]=scan.networks[i].supported;}
 auto ot=doc.createNestedObject("ota");ot["direct"]=channel.direct;ot["url"]=channel.url;ot["busy"]=update.busy;ot["ready"]=update.ready;ot["pending"]=update.pending;ot["seconds"]=update.seconds;ot["revision"]=update.revision;ot["progress"]=update.progress;ot["message"]=update.message;
 auto versions=ot.createNestedArray("versions");for(unsigned i=0;i<update.count;++i){auto v=versions.createNestedObject();v["version"]=update.versions[i];v["sequence"]=ota::versionSequence(update.versions[i]);v["action"]=ota::actionText(ota::versionSequence(update.versions[i]));}
 mbedtls_platform_zeroize(&wifi,sizeof(wifi));
 if(doc.overflowed()||measureJson(doc)>=out.size)return reply(r,"503 Service Unavailable","Status momentan nicht verfügbar.");
 size_t size=serializeJson(doc,out.data,out.size);headers(r);httpd_resp_set_type(r,"application/json");return httpd_resp_send(r,out.data,size);
}
static esp_err_t action(httpd_req_t* r){
 if(!allowedHost(r,true)||!authorized(r,true))return reply(r,"403 Forbidden","Sitzung abgelaufen oder Anfrage abgelehnt. Erneut anmelden.");
 Buffer body(2048);Document doc(2048);if(!readBody(r,body,doc))return reply(r,"400 Bad Request","Ungültige Eingabe.");
 const char* act=doc["action"]|"";bool ok=false;
 if(!strcmp(act,"logout")){lock();session.clear();unlock();httpd_resp_set_hdr(r,"Set-Cookie","sid=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");return reply(r,"200 OK","Abgemeldet.");}
 if(!strcmp(act,"scan"))ok=live::scanStart();
 else if(!strcmp(act,"wifi")){
  live::Config c;live::config(c);char ssid[33]={},secret[257]={};
  bool valid=doc.size()==5&&string(doc["ssid"],ssid,sizeof(ssid))&&string(doc["password"],secret,64)&&string(doc["ntp"],c.ntp,sizeof(c.ntp));
  if(valid){if(*secret)memcpy(c.password,secret,strlen(secret)+1);else if(strcmp(ssid,c.ssid))valid=false;snprintf(c.ssid,sizeof(c.ssid),"%s",ssid);}
  valid=valid&&string(doc["token"],secret,sizeof(secret));if(valid&&*secret)snprintf(c.token,sizeof(c.token),"%s",secret);
  ok=valid&&live::save(c);mbedtls_platform_zeroize(secret,sizeof(secret));mbedtls_platform_zeroize(&c,sizeof(c));
  return reply(r,ok?"202 Accepted":"400 Bad Request",ok?"Speichern angefordert. Bei WLAN-Wechsel kann die Verbindung abbrechen; neue IP am Display prüfen.":"Nicht übernommen. WLAN-Passwort bei neuem WLAN erforderlich; Eingaben und Netzwerkstatus prüfen.");
 }else if(!strcmp(act,"panel")){
  preferences::Config c;ok=doc.size()==3&&string(doc["origin"],c.origin,sizeof(c.origin))&&string(doc["name"],c.name,sizeof(c.name))&&preferences::save(c);
 }else if(!strcmp(act,"channel")||!strcmp(act,"check")){
  ota::Config c;ok=doc.size()==3&&doc["direct"].is<bool>()&&string(doc["url"],c.url,sizeof(c.url));c.direct=doc["direct"]|false;
  ok=ok&&(!strcmp(act,"check")?ota::check(c):ota::configure(c));
 }else if(!strcmp(act,"install")){
  ota::Config c;ota::config(c);ok=doc.size()==4&&doc["confirm"].is<bool>()&&doc["confirm"].as<bool>()&&doc["revision"].is<unsigned>()&&doc["sequence"].is<unsigned>()&&ota::installVersion(c,doc["revision"],doc["sequence"]);
 }else if(!strcmp(act,"keep")){
  ota::Status s;ota::status(s);ok=s.pending&&s.ready&&!s.busy;if(ok)ota::confirm();
 }else if(!strcmp(act,"password")){
  char old[64]={},next[64]={};bool valid=doc.size()==3&&string(doc["current"],old,sizeof(old))&&string(doc["password"],next,sizeof(next));
  lock();bool allowed=attempts.allowed(millis());valid=allowed&&valid&&verify(old);if(allowed&&!valid)attempts.failed(millis());unlock();
  ok=valid&&password(next);mbedtls_platform_zeroize(old,sizeof(old));mbedtls_platform_zeroize(next,sizeof(next));
  return reply(r,ok?"200 OK":"400 Bad Request",ok?"Passwort gespeichert. Bitte erneut anmelden.":"Nicht geändert. Aktuelles Passwort und mindestens 5 Byte prüfen; nach Fehlversuchen warten.");
 }
 return reply(r,ok?"200 OK":"409 Conflict",ok?"Übernommen; Status beachten.":"Nicht übernommen. Eingaben/OTA-Status prüfen und gegebenenfalls erneut Versionen laden.");
}
// The temporary initial password is stored only until the user changes it.
// Its owner digest prevents stale display after older firmware changes credentials.
static bool createInitial(){
 Initial next;
 for(unsigned i=0;i<12;){unsigned char b;esp_fill_random(&b,1);if(b<250)next.digits[i++]=char('0'+b%10);}
 esp_fill_random(next.owner.salt,sizeof(next.owner.salt));
 bool ok=hash(next.digits,next.owner,next.owner.digest);nvs_handle_t n;
 if(ok&&nvs_open("eagle-web",NVS_READWRITE,&n)==ESP_OK){
  // Save recoverable digits first; credential presence is the activation marker.
  ok=nvs_set_blob(n,"initial",&next,sizeof(next))==ESP_OK&&nvs_set_blob(n,"credential",&next.owner,sizeof(next.owner))==ESP_OK&&nvs_set_u8(n,"autodone",1)==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);
 }else ok=false;
 if(ok){credential=next.owner;memcpy(initialDigits,next.digits,sizeof(initialDigits));state.configured=true;}
 mbedtls_platform_zeroize(&next,sizeof(next));return ok;
}
void begin(bool ready){
 guard=xSemaphoreCreateMutexStatic(&mutexStorage);networkReady=ready;lock();
 nvs_handle_t n;uint8_t done=0;bool missing=false;bool storageOk=false;
 auto opened=nvs_open("eagle-web",NVS_READONLY,&n);
 if(opened==ESP_OK){
  Credential c;size_t size=sizeof(c);auto result=nvs_get_blob(n,"credential",&c,&size);
  missing=result==ESP_ERR_NVS_NOT_FOUND;
  if(result==ESP_OK&&size==sizeof(c)&&c.version==1){credential=c;state.configured=true;}
  auto marker=nvs_get_u8(n,"autodone",&done);storageOk=marker==ESP_OK||marker==ESP_ERR_NVS_NOT_FOUND;
  Initial initial;size=sizeof(initial);
  if(state.configured&&nvs_get_blob(n,"initial",&initial,&size)==ESP_OK&&size==sizeof(initial)&&
     webpolicy::validInitial(initial.digits)&&!memcmp(&initial.owner,&credential,sizeof(credential))&&verify(initial.digits))memcpy(initialDigits,initial.digits,sizeof(initialDigits));
  mbedtls_platform_zeroize(&initial,sizeof(initial));mbedtls_platform_zeroize(&c,sizeof(c));nvs_close(n);
 }else if(opened==ESP_ERR_NVS_NOT_FOUND){missing=true;storageOk=true;}
 if(webpolicy::needsInitial(state.configured,missing,storageOk,done!=0)){
  if(!createInitial())snprintf(state.message,sizeof(state.message),"Initiales Web-Passwort nicht gespeichert; bitte lokal neu setzen.");
 }else if(!state.configured)snprintf(state.message,sizeof(state.message),"Webzugang deaktiviert oder Speicherfehler; Passwort lokal setzen.");
 if(state.configured)snprintf(state.message,sizeof(state.message),"WebAdmin eingerichtet. WLAN oder Einrichtungshotspot verbinden.");
 unlock();
 // Never log credentials or return the initial password through HTTP.
}
void tick(){
 if(!guard)return;
 Status s;status(s);
 if(!s.configured){if(server){httpd_stop(server);server=nullptr;lock();state.running=false;unlock();}return;}
 if(!networkReady||server)return;
 live::Status net;live::status(net);live::Hotspot setup;live::hotspotStatus(setup);memset(setup.password,0,sizeof(setup.password));if(!net.wifi&&!setup.active)return;
 static uint32_t last=0;if(last&&uint32_t(millis()-last)<30000)return;last=millis();
 httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.stack_size=8192;config.max_open_sockets=3;config.max_uri_handlers=5;config.lru_purge_enable=true;config.recv_wait_timeout=3;config.send_wait_timeout=3;
 logMemory("before_web_start");auto result=httpd_start(&server,&config);logMemory("after_web_start");
 if(result==ESP_OK){
  const httpd_uri_t routes[]={
   {.uri="/",.method=HTTP_GET,.handler=root,.user_ctx=nullptr},
   {.uri="/app.js",.method=HTTP_GET,.handler=root,.user_ctx=nullptr},
   {.uri="/api/login",.method=HTTP_POST,.handler=login,.user_ctx=nullptr},
   {.uri="/api/status",.method=HTTP_GET,.handler=snapshot,.user_ctx=nullptr},
   {.uri="/api/action",.method=HTTP_POST,.handler=action,.user_ctx=nullptr}};
  for(const auto& route:routes)if((result=httpd_register_uri_handler(server,&route))!=ESP_OK)break;
  if(result!=ESP_OK){httpd_stop(server);server=nullptr;}
 }
 lock();state.running=result==ESP_OK;snprintf(state.message,sizeof(state.message),state.running?"Webzugang bereit · Benutzer admin":"Webstart fehlgeschlagen; UI bleibt aktiv, neuer Versuch folgt.");unlock();
 ESP_LOGI("EAGLENET_WEB","HTTP start result=%s",esp_err_to_name(result));
}
}
