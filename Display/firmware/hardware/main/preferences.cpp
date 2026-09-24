#include "preferences.h"
#include "ota.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
namespace preferences {
static Config current;
static StaticSemaphore_t storage;
static SemaphoreHandle_t guard;
void begin(){
 guard=xSemaphoreCreateMutexStatic(&storage);
 nvs_handle_t n;if(nvs_open("eagle-panel",NVS_READONLY,&n)!=ESP_OK)return;
 Config c;size_t size=sizeof(c);
 if(nvs_get_blob(n,"config",&c,&size)==ESP_OK&&size==sizeof(c)&&valid(c)){xSemaphoreTake(guard,portMAX_DELAY);current=c;xSemaphoreGive(guard);}
 nvs_close(n);
}
void get(Config& c){if(guard)xSemaphoreTake(guard,portMAX_DELAY);c=current;if(guard)xSemaphoreGive(guard);}
bool save(const Config& c){
 if(!guard||!valid(c)||ota::busy())return false;
 nvs_handle_t n;if(nvs_open("eagle-panel",NVS_READWRITE,&n)!=ESP_OK)return false;
 bool ok=nvs_set_blob(n,"config",&c,sizeof(c))==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);
 if(ok){xSemaphoreTake(guard,portMAX_DELAY);current=c;xSemaphoreGive(guard);}return ok;
}
bool endpoint(const char* path,char* output,size_t capacity){
 Config c;get(c);if(!valid(c))return false;
 int n=snprintf(output,capacity,"%s%s",c.origin,path);return n>0&&(size_t)n<capacity;
}
}
