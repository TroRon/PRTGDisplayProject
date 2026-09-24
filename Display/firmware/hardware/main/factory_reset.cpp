#include "factory_reset.h"
#include "reset_policy.h"
#include "ota.h"
#include "esp_flash.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_log.h"
#include <cstring>
namespace factoryreset {
static const unsigned char marker[16]={'P','R','T','G','R','E','S','E','T','1',0xa7,0x59,0xc3,0x2d,0x8e,0x61};
// Free sector after otadata: compatible with the existing OTA partition table.
// Reserved for reset intent only, never credentials. Reject overlapping layouts.
static const esp_partition_t* layout(){
 uint32_t bytes=0;if(esp_flash_get_size(nullptr,&bytes)!=ESP_OK||bytes!=16*1024*1024)return nullptr;
 auto* nvs=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_NVS,"nvs");
 auto* ota=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_OTA,"otadata");
 if(!nvs||nvs->address!=0x9000||nvs->size!=0x6000||nvs->encrypted||!ota||ota->address!=0xc10000||ota->size!=0x2000)return nullptr;
 auto it=esp_partition_find(ESP_PARTITION_TYPE_ANY,ESP_PARTITION_SUBTYPE_ANY,nullptr);
 while(it){const auto* p=esp_partition_get(it);if(resetpolicy::overlaps(p->address,p->size)){esp_partition_iterator_release(it);return nullptr;}it=esp_partition_next(it);}
 return nvs;
}
static bool erased(uint32_t address,uint32_t size){
 unsigned char buffer[256];
 for(uint32_t offset=0;offset<size;offset+=sizeof(buffer)){
  if(esp_flash_read(nullptr,buffer,address+offset,sizeof(buffer))!=ESP_OK)return false;
  for(auto byte:buffer)if(byte!=0xff){memset(buffer,0,sizeof(buffer));return false;}
 }memset(buffer,0,sizeof(buffer));return true;
}
struct Storage {
 const esp_partition_t* nvs;
 bool eraseData(){return esp_partition_erase_range(nvs,0,nvs->size)==ESP_OK;}
 bool verifyData(){return erased(nvs->address,nvs->size);}
 bool eraseMarker(){return esp_flash_erase_region(nullptr,resetpolicy::MarkerAddress,resetpolicy::SectorSize)==ESP_OK;}
 bool verifyMarker(){return erased(resetpolicy::MarkerAddress,resetpolicy::SectorSize);}
};
// Called before NVS initialization and before any worker/UI/USB/network task.
bool resume(){
 auto* nvs=layout();if(!nvs){ESP_LOGE("EAGLENET_RESET","Unsupported partition layout; startup stopped");return false;}
 unsigned char saved[sizeof(marker)];
 if(esp_flash_read(nullptr,saved,resetpolicy::MarkerAddress,sizeof(saved))!=ESP_OK)return false;
 if(memcmp(saved,marker,sizeof(marker)))return true;
 ESP_LOGW("EAGLENET_RESET","Erasing all NVS sectors before services start");
 Storage storage{nvs};bool ok=resetpolicy::finish(storage);
 ESP_LOGI("EAGLENET_RESET","Factory reset verified=%d; no customer values logged",ok);return ok;
}
bool request(){
 if(!layout()||!ota::reserveReset())return false;
 bool written=esp_flash_erase_region(nullptr,resetpolicy::MarkerAddress,resetpolicy::SectorSize)==ESP_OK&&esp_flash_write(nullptr,marker,resetpolicy::MarkerAddress,sizeof(marker))==ESP_OK;
 unsigned char saved[sizeof(marker)]={};
 bool confirmed=esp_flash_read(nullptr,saved,resetpolicy::MarkerAddress,sizeof(saved))==ESP_OK&&!memcmp(saved,marker,sizeof(marker));
 if(!confirmed){ESP_LOGE("EAGLENET_RESET","Reset intent verification failed; write=%d",written);ota::cancelReset();return false;}
 ESP_LOGW("EAGLENET_RESET","Factory reset confirmed locally; restarting for verified erase");esp_restart();return true;
}
}
