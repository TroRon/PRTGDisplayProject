#include "preferences.h"
#include "ota_catalog.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include <atomic>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"
#include "mbedtls/base64.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
extern const char ota_public_key[] asm("_binary_ota_public_pem_start");
namespace ota {
static bool manifestUrl(const Config& c,char* url,size_t capacity){
 if(c.direct){snprintf(url,capacity,"%s",c.url);return true;}
 return preferences::endpoint("/api/v1/firmware/manifest.json",url,capacity);
}
static StaticSemaphore_t storage;
static SemaphoreHandle_t guard;
static std::atomic<bool> initialized{false};
static Config selected;
static Status state;
static unsigned command=0;
static uint32_t pendingAt=0;
static Offer releases[MaxReleases];
static char checkedManifest[384]={};
static Offer offer;
struct PsramAllocator {
 void* allocate(size_t n){return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
 void deallocate(void* p){heap_caps_free(p);}
 void* reallocate(void* p,size_t n){return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
};
static void lock(){xSemaphoreTake(guard,portMAX_DELAY);}
static void unlock(){xSemaphoreGive(guard);}
static void message(const char* s){lock();snprintf(state.message,sizeof(state.message),"%s",s);unlock();ESP_LOGI("EAGLENET_OTA","%s",s);}
void config(Config& c){if(!initialized){c=Config{};return;}lock();c=selected;unlock();}
void status(Status& s){if(!initialized){s=Status{};return;}lock();s=state;unlock();}
bool busy(){Status s;status(s);return s.busy;}
void begin(bool networkReady){
 guard=xSemaphoreCreateMutexStatic(&storage);
 nvs_handle_t n;
 if(nvs_open("eagle-ota",NVS_READONLY,&n)==ESP_OK){
  Config c;size_t size=sizeof(c);
  if(nvs_get_blob(n,"config",&c,&size)==ESP_OK&&size==sizeof(c)&&validConfig(c))selected=c;
  nvs_close(n);
 }
 esp_ota_img_states_t imageState;
 state.pending=esp_ota_get_state_partition(esp_ota_get_running_partition(),&imageState)==ESP_OK&&imageState==ESP_OTA_IMG_PENDING_VERIFY;
 pendingAt=millis();state.ready=networkReady;initialized=true;
 message(state.pending?"Neue Version prüfen und innert 120 s behalten bestätigen":networkReady?"Update-Kanal wählen und Update prüfen":"Netzwerkworker fehlt; OTA nicht verfügbar");
}
void tick(){
 if(!initialized)return;
 lock();const bool pending=state.pending;
 const unsigned elapsed=uint32_t(millis()-pendingAt)/1000;
 state.seconds=pending&&elapsed<120?120-elapsed:0;unlock();
 if(pending&&elapsed>=120){ESP_LOGW("EAGLENET_OTA","Confirmation timeout; reboot for rollback");esp_restart();}
}
bool check(const Config& c){
 if(!initialized||!validConfig(c))return false;
 lock();
 bool ok=state.ready&&!state.busy&&!state.pending;
 if(ok){selected=c;state.available=false;state.offered[0]=0;state.count=0;++state.revision;state.busy=true;state.progress=0;command=1;}
 unlock();return ok;
}
bool choose(unsigned index){
 if(!initialized)return false;
 lock();bool ok=!state.busy&&!state.pending&&index<state.count;
 if(ok){state.chosen=index;offer=releases[index];state.targetSequence=offer.sequence;state.targetSize=offer.size;
  snprintf(state.offered,sizeof(state.offered),"%s",offer.version);state.available=true;}
 unlock();return ok;
}
bool install(const Config& c){
 if(!initialized)return false;
 char manifest[384];if(!manifestUrl(c,manifest,sizeof(manifest)))return false;
 lock();
 bool ok=!strcmp(manifest,checkedManifest)&&state.ready&&!state.busy&&!state.pending&&state.available&&validConfig(c)&&c.direct==selected.direct&&(!c.direct||!strcmp(c.url,selected.url));
 if(ok){state.busy=true;state.available=false;state.progress=0;command=2;}
 unlock();return ok;
}
void confirm(){
 if(!initialized)return;
 lock();const bool ok=state.pending&&state.ready&&!state.busy;
 if(ok){command=3;state.busy=true;}
 unlock();
}
static esp_http_client_handle_t open(const char* url,const live::Config& credentials,bool authenticated,int* http=nullptr){
 esp_http_client_config_t c={};c.url=url;c.crt_bundle_attach=esp_crt_bundle_attach;
 c.timeout_ms=10000;c.disable_auto_redirect=true;c.buffer_size=2048;c.buffer_size_tx=1024;
 auto client=esp_http_client_init(&c);
 if(!client){message("OTA: HTTPS-Speicher nicht verfügbar");return nullptr;}
 esp_err_t err=ESP_OK;const char* stage="header";
 if(authenticated){char header[sizeof(credentials.token)+8];snprintf(header,sizeof(header),"Bearer %s",credentials.token);
  err=esp_http_client_set_header(client,"Authorization",header);memset(header,0,sizeof(header));}
 if(err==ESP_OK){stage="connect";err=esp_http_client_open(client,0);}
 int status=0;int64_t length=-1;
 if(err==ESP_OK){stage="response";length=esp_http_client_fetch_headers(client);status=esp_http_client_get_status_code(client);}
 if(http)*http=status;
 if(err!=ESP_OK||length<0||status!=200){
  if(err==ESP_OK&&length<0)err=ESP_FAIL;
  int tls=0,flags=0;esp_http_client_get_and_clear_last_tls_error(client,&tls,&flags);
  ESP_LOGW("EAGLENET_OTA","request stage=%s err=%s HTTP=%d tls=%d flags=%d",stage,esp_err_to_name(err),status,tls,flags);
  char detail[160];
  if(status)snprintf(detail,sizeof(detail),"OTA HTTP %d: %s",status,status==404?"Datei fehlt; Kanal/Manifest-URL prüfen":status==401||status==403?"Zugriff abgelehnt":status>=300&&status<400?"Direkte Datei-URL ohne Weiterleitung verwenden":"Serverantwort nicht erfolgreich");
  else if(tls||flags)snprintf(detail,sizeof(detail),"OTA TLS fehlgeschlagen: Zeit, Zertifikat und Internetzugang prüfen");
  else snprintf(detail,sizeof(detail),"OTA Verbindung fehlgeschlagen (%s): DNS, Internetzugang oder Timeout",esp_err_to_name(err));
  message(detail);esp_http_client_cleanup(client);return nullptr;
 }
 return client;
}
static bool signedOffer(const char* envelope,size_t size,Offer& result){
 BasicJsonDocument<PsramAllocator> wrapper(12288);
 if(deserializeJson(wrapper,envelope,size)||!wrapper["payload"].is<const char*>()||!wrapper["signature"].is<const char*>())return false;
 const char* payload=wrapper["payload"],*signature=wrapper["signature"];
 unsigned char bytes[2048],sig[256],hash[32];size_t count=0,sigsize=0;
 if(mbedtls_base64_decode(bytes,sizeof(bytes)-1,&count,(const unsigned char*)payload,strlen(payload))||
    mbedtls_base64_decode(sig,sizeof(sig),&sigsize,(const unsigned char*)signature,strlen(signature))||sigsize!=256||!count)return false;
 bytes[count]=0;
 mbedtls_pk_context key;mbedtls_pk_init(&key);
 int err=mbedtls_pk_parse_public_key(&key,(const unsigned char*)ota_public_key,strlen(ota_public_key)+1);
 if(!err)err=mbedtls_sha256(bytes,count,hash,0);
 if(!err)err=mbedtls_pk_verify(&key,MBEDTLS_MD_SHA256,hash,sizeof(hash),sig,sigsize);
 mbedtls_pk_free(&key);if(err)return false;
 BasicJsonDocument<PsramAllocator> doc(4096);
 if(deserializeJson(doc,bytes,count))return false;
 return offerMetadata(doc.as<JsonVariantConst>(),result);
}
static bool saveConfig(const Config& c){
 nvs_handle_t n;if(nvs_open("eagle-ota",NVS_READWRITE,&n)!=ESP_OK)return false;
 bool ok=nvs_set_blob(n,"config",&c,sizeof(c))==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);return ok;
}
bool configure(const Config& c){
 if(!initialized||!validConfig(c))return false;
 lock();
 bool ok=!state.busy&&!state.pending&&saveConfig(c);
 if(ok){selected=c;state.available=false;state.offered[0]=0;state.count=0;++state.revision;state.progress=0;
  snprintf(state.message,sizeof(state.message),"Update-Kanal gespeichert; Update prüfen bei Bedarf");}
 unlock();return ok;
}
static void fetchOffer(const Config& c,const live::Config& credentials){
 if(!saveConfig(c)){message("Update-Kanal konnte nicht gespeichert werden");return;}
 message("Verfügbare Versionen werden geprüft");
 char manifest[384],catalog[384];
 if(!manifestUrl(c,manifest,sizeof(manifest))||!catalogUrl(manifest,catalog,sizeof(catalog))){message("Kanal-Adresse prüfen; Aggregator unter Panel eintragen");return;}
 int http=0;bool legacy=false;
 auto client=open(catalog,credentials,!c.direct,&http);
 if(!client&&http==404){legacy=true;client=open(manifest,credentials,!c.direct);}
 if(!client)return;
 constexpr size_t Limit=16384;
 char* data=(char*)heap_caps_malloc(Limit+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 size_t used=0;const uint32_t start=millis();
 if(data)while(used<Limit&&uint32_t(millis()-start)<30000&&!esp_http_client_is_complete_data_received(client)){
  int n=esp_http_client_read(client,data+used,Limit-used);if(n<=0)break;used+=n;
 }
 bool complete=data&&esp_http_client_is_complete_data_received(client);
 esp_http_client_cleanup(client);
 Offer candidates[MaxReleases];unsigned count=0;bool ok=complete;
 if(ok&&legacy){Offer one;ok=signedOffer(data,used,one)&&addOffer(candidates,count,one);}
 else if(ok){
  BasicJsonDocument<PsramAllocator> doc(32768);
  ok=!deserializeJson(doc,data,used)&&(doc["schema"]|0)==1&&doc["releases"].is<JsonArray>()&&doc["releases"].size()>0&&doc["releases"].size()<=MaxReleases;
  if(ok)for(JsonVariant entry:doc["releases"].as<JsonArray>()){
   char envelope[4096];Offer one;
   size_t size=measureJson(entry);
   if(size>=sizeof(envelope)){ok=false;break;}
   serializeJson(entry,envelope,sizeof(envelope));
   if(!signedOffer(envelope,size,one)||!addOffer(candidates,count,one)){ok=false;break;}
  }
 }
 heap_caps_free(data);
 if(!ok||!count){message(complete?"Versionsliste abgelehnt: Signatur/Board/Version prüfen":"Versionsliste unvollständig oder zu gross; erneut prüfen");return;}
 lock();
 memcpy(releases,candidates,count*sizeof(Offer));snprintf(checkedManifest,sizeof(checkedManifest),"%s",manifest);
 state.count=count;state.chosen=0;++state.revision;offer=releases[0];
 for(unsigned i=0;i<count;++i)snprintf(state.versions[i],sizeof(state.versions[i]),"%s",releases[i].version);
 snprintf(state.offered,sizeof(state.offered),"%s",offer.version);state.available=true;state.targetSequence=offer.sequence;state.targetSize=offer.size;
 unlock();message(legacy?"Einzelangebot geladen; Server bietet noch keine Versionsliste":"Signierte Versionen geladen; Zielversion auswählen");
}
static void download(const Config& c,const live::Config& credentials){
 char url[512];
 char manifest[384];
 if(!manifestUrl(c,manifest,sizeof(manifest))||!binaryUrl(manifest,offer.hash,url,sizeof(url))){message("Ungültiger Downloadpfad");return;}
 auto partition=esp_ota_get_next_update_partition(nullptr);
 if(!partition||partition==esp_ota_get_running_partition()||offer.size>partition->size){message("OTA-Partition fehlt; USB-Migration erforderlich");return;}
 message("Firmware wird geladen; Monitoring pausiert vorübergehend");
 auto client=open(url,credentials,!c.direct);
 if(!client)return;
 auto* buffer=(unsigned char*)heap_caps_malloc(4096,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 esp_ota_handle_t handle=0;bool started=false,ended=false;
 mbedtls_sha256_context sha;mbedtls_sha256_init(&sha);
 bool ok=[&](){
  if(!buffer)return false;
  auto length=esp_http_client_get_content_length(client);
  if(length>0&&(unsigned)length!=offer.size)return false;
  if(esp_ota_begin(partition,OTA_WITH_SEQUENTIAL_WRITES,&handle)!=ESP_OK)return false;
  started=true;if(mbedtls_sha256_starts(&sha,0))return false;
  unsigned used=0;const uint32_t start=millis();
  while(used<offer.size){
   if(uint32_t(millis()-start)>180000)return false;
   int n=esp_http_client_read(client,(char*)buffer,offer.size-used<4096?offer.size-used:4096);
   if(n<=0||mbedtls_sha256_update(&sha,buffer,n)||esp_ota_write(handle,buffer,n)!=ESP_OK)return false;
   used+=n;lock();state.progress=(unsigned)((uint64_t)used*100/offer.size);unlock();vTaskDelay(1);
  }
  if(!esp_http_client_is_complete_data_received(client)){
   // Consume chunk terminator; reject any extra byte beyond the signed size.
   int n=esp_http_client_read(client,(char*)buffer,1);
   if(n!=0||!esp_http_client_is_complete_data_received(client))return false;
  }
  unsigned char hash[32];char digest[65];if(mbedtls_sha256_finish(&sha,hash))return false;
  for(unsigned i=0;i<32;++i)snprintf(digest+2*i,3,"%02x",hash[i]);
  if(strcmp(digest,offer.hash))return false;
  esp_err_t end=esp_ota_end(handle);ended=true;if(end!=ESP_OK)return false;
  esp_app_desc_t app;
  if(esp_ota_get_partition_description(partition,&app)!=ESP_OK||strcmp(app.project_name,"eaglenet_lcd5b")||strcmp(app.version,offer.version))return false;
  return esp_ota_set_boot_partition(partition)==ESP_OK;
 }();
 if(started&&!ended)esp_ota_abort(handle);
 mbedtls_sha256_free(&sha);heap_caps_free(buffer);esp_http_client_cleanup(client);
 if(!ok){message("Update fehlgeschlagen; bisherige Firmware bleibt aktiv");return;}
 message("Firmware geprüft. Neustart; danach am Display bestätigen");vTaskDelay(pdMS_TO_TICKS(1500));esp_restart();
}
void service(const live::Config& credentials){
 if(!initialized)return;
 lock();unsigned action=command;command=0;Config c=selected;unlock();
 if(!action)return;
 if(action==3){
  if(esp_ota_mark_app_valid_cancel_rollback()==ESP_OK){lock();state.pending=false;unlock();message("Firmware bestätigt und dauerhaft übernommen");}
  else message("Bestätigung fehlgeschlagen; Rückfall bleibt aktiv");
 }else{
  live::Status net;live::status(net);
  if(!net.wifi||!net.clock)message("Für OTA werden WLAN und synchronisierte Zeit benötigt");
  else if(action==1)fetchOffer(c,credentials);
  else download(c,credentials);
 }
 lock();state.busy=false;unlock();
}
}
