#include "remote_protocol.h"
#include "remote_config.h"
#include "preferences.h"
#include "ota.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <new>
namespace remoteconfig {
struct Saved {uint32_t version=1;char id[33]={},key[65]={};bool released=false;displayconfig::Command command;};
struct State {Saved saved;displayconfig::Settings report;displayconfig::Command pending;bool hasPending=false;char error[16]={};};
static State* state=nullptr;static StaticSemaphore_t mutexStorage;static SemaphoreHandle_t mutex=nullptr;
static uint32_t lastPoll=0;
struct Guard {Guard(){xSemaphoreTake(mutex,portMAX_DELAY);}~Guard(){xSemaphoreGive(mutex);}};
struct Allocator {void* allocate(size_t n){return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}void deallocate(void* p){heap_caps_free(p);}void* reallocate(void* p,size_t n){return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}};
static bool persist(const Saved& s){nvs_handle_t n;if(nvs_open("eagle-remote",NVS_READWRITE,&n)!=ESP_OK)return false;bool ok=nvs_set_blob(n,"config",&s,sizeof(s))==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);return ok;}
static bool matches(const char* origin){preferences::Config c;preferences::get(c);return !strcmp(c.origin,origin);}
static bool central(){return state->saved.command.central&&matches(state->saved.command.settings.view.origin);}
void begin(){
 if(state){return;}mutex=xSemaphoreCreateMutexStatic(&mutexStorage);void* p=heap_caps_malloc(sizeof(State),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!p)return;state=new(p) State;
 nvs_handle_t n;bool found=false;if(nvs_open("eagle-remote",NVS_READONLY,&n)==ESP_OK){size_t size=sizeof(Saved);found=nvs_get_blob(n,"config",&state->saved,&size)==ESP_OK&&size==sizeof(Saved);nvs_close(n);}
 auto& s=state->saved;
 if(found&&(s.version!=1||!memchr(s.id,0,sizeof(s.id))||!memchr(s.key,0,sizeof(s.key))||strlen(s.id)!=32||strlen(s.key)!=64||!displayconfig::validOptions(s.command.settings.view)||!memchr(s.command.settings.name,0,sizeof(s.command.settings.name)))){heap_caps_free(state);state=nullptr;return;}
 if(!found){s=Saved{};uint8_t random[48];esp_fill_random(random,sizeof(random));for(unsigned i=0;i<16;i++)snprintf(s.id+2*i,3,"%02x",random[i]);for(unsigned i=0;i<32;i++)snprintf(s.key+2*i,3,"%02x",random[i+16]);memset(random,0,sizeof(random));if(!persist(s)){heap_caps_free(state);state=nullptr;return;}}
 lastPoll=millis()-60000;
}
bool managed(){if(!state)return false;Guard g;return central();}
void effectiveName(char* name,unsigned cap){if(!state)return;Guard g;if(central())snprintf(name,cap,"%s",state->saved.command.settings.name);}
void report(const displayconfig::Options& options){if(!state)return;preferences::Config c;preferences::get(c);Guard g;state->report.view=options;if(strcmp(options.origin,c.origin)){memset(state->report.view.favorites,0,sizeof(state->report.view.favorites));state->report.view.favoriteHome=false;}snprintf(state->report.view.origin,sizeof(state->report.view.origin),"%s",c.origin);snprintf(state->report.name,sizeof(state->report.name),"%s",central()?state->saved.command.settings.name:c.name);}
bool restore(displayconfig::Command& c){if(!state)return false;Guard g;if(!central())return false;c=state->saved.command;return true;}
bool hasPending(){if(!state)return false;Guard g;return state->hasPending;}
bool pending(displayconfig::Command& c){if(!state)return false;Guard g;if(!state->hasPending||!matches(state->pending.settings.view.origin))return false;c=state->pending;return true;}
void failed(const char* code){if(!state)return;Guard g;snprintf(state->error,sizeof(state->error),"%s",code);}
bool accept(const displayconfig::Command& c){if(!state)return false;Guard g;if(!matches(c.settings.view.origin))return false;Saved next=state->saved;next.command=c;next.released=false;if(!persist(next)){snprintf(state->error,sizeof(state->error),"STORAGE");return false;}state->saved=next;state->hasPending=false;*state->error=0;return true;}
bool release(){if(!state)return false;Guard g;Saved next=state->saved;next.command.central=false;next.released=true;if(!persist(next))return false;state->saved=next;state->hasPending=false;return true;}
void service(const char* token,char* scratch,unsigned capacity){
 if(!state||!*token||capacity<8193||ota::busy()||uint32_t(millis()-lastPoll)<60000)return;
 lastPoll=millis();
 preferences::Config source;preferences::get(source);if(!preferences::valid(source))return;
 BasicJsonDocument<Allocator> doc(12288);char key[65];
 {Guard g;doc["id"]=state->saved.id;doc["firmware"]=ota::Version;doc["revision"]=matches(state->saved.command.settings.view.origin)?state->saved.command.revision:0;doc["mode"]=central()?"central":"local";doc["release"]=state->saved.released;doc["error"]=state->error;remoteprotocol::encode(doc.createNestedObject("settings"),state->report);memcpy(key,state->saved.key,sizeof(key));}
 if(doc.overflowed())return;
 size_t size=serializeJson(doc,scratch,8192);if(size>=8192)return;
 char endpoint[384];snprintf(endpoint,sizeof(endpoint),"%s/api/v1/display/sync",source.origin);esp_http_client_config_t config={};config.url=endpoint;config.crt_bundle_attach=esp_crt_bundle_attach;config.timeout_ms=3000;config.disable_auto_redirect=true;config.method=HTTP_METHOD_POST;config.buffer_size=1024;config.buffer_size_tx=1024;
 auto client=esp_http_client_init(&config);if(!client)return;char auth[280];snprintf(auth,sizeof(auth),"Bearer %s",token);esp_http_client_set_header(client,"Authorization",auth);esp_http_client_set_header(client,"X-Display-Key",key);esp_http_client_set_header(client,"Content-Type","application/json");memset(auth,0,sizeof(auth));memset(key,0,sizeof(key));
 bool ok=false;int code=0;const int64_t start=esp_timer_get_time();
 if(esp_http_client_open(client,size)==ESP_OK&&esp_http_client_write(client,scratch,size)==int(size)&&esp_http_client_fetch_headers(client)>=0){
  code=esp_http_client_get_status_code(client);if(code==200){size_t used=0;while(used<8192&&!esp_http_client_is_complete_data_received(client)&&esp_timer_get_time()-start<8000000){int n=esp_http_client_read(client,scratch+used,8192-used);if(n<=0)break;used+=n;}scratch[used]=0;
   if(esp_http_client_is_complete_data_received(client)&&!deserializeJson(doc,scratch,used,DeserializationOption::NestingLimit(6))&&doc["schema"]==1&&doc["revision"].is<uint32_t>()&&doc["mode"].is<const char*>()){
    // Allocate the candidate in PSRAM, not on the network task's small stack.
    void* mem=heap_caps_malloc(sizeof(displayconfig::Command),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);auto* command=mem?new(mem) displayconfig::Command:nullptr;
    if(command){const char* mode=doc["mode"];command->revision=doc["revision"];command->central=!strcmp(mode,"central");snprintf(command->settings.view.origin,sizeof(command->settings.view.origin),"%s",source.origin);
     ok=(!strcmp(mode,"local")||command->central)&&remoteprotocol::decode(doc["settings"],command->settings);
     if(ok){Guard g;if(!(state->saved.released&&!command->central)&&matches(source.origin)&&command->revision<state->saved.command.revision&&matches(state->saved.command.settings.view.origin)){snprintf(state->error,sizeof(state->error),"INVALID");ok=false;}else if(matches(source.origin)&&(!state->saved.released||!command->central)&&(*state->error||state->saved.released||command->revision!=state->saved.command.revision||command->central!=central()||!matches(state->saved.command.settings.view.origin))){state->pending=*command;state->hasPending=true;}}
     command->~Command();heap_caps_free(command);
    }
   }
  }
 }
 esp_http_client_close(client);esp_http_client_cleanup(client);memset(scratch,0,8193);ESP_LOGI("DISPLAY_CONFIG","sync HTTP=%d valid=%d",code,ok);
}
}


