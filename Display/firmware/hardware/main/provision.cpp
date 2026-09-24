#include "live.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "provision_policy.h"
#include "web_admin.h"
static const char* Tag="EAGLENET_USB";
static void provisionTask(void*) {
    char line[2048];size_t used=0;bool overflow=false;
    for(;;) {
        char c;
        if(usb_serial_jtag_read_bytes(&c,1,pdMS_TO_TICKS(100))!=1)continue;
        if(c=='\r')continue;
        if(c!='\n'){if(used<sizeof(line)-1)line[used++]=c;else overflow=true;continue;}
        line[used]=0;bool accepted=false;
        if(!overflow&&strncmp(line,"WEB_CONFIG ",11)==0){
            char password[64]={};accepted=decodeWebConfig(line+11,password,sizeof(password))&&webadmin::password(password);
            memset(password,0,sizeof(password));memset(line,0,sizeof(line));used=0;overflow=false;
            ESP_LOGI(Tag,"%s",accepted?"WEB_CONFIG_SAVED":"WEB_CONFIG_REJECTED");continue;
        }
        if(!overflow&&strncmp(line,"PANEL_CONFIG ",13)==0){
            preferences::Config value;
            accepted=decodePanelConfig(line+13,value)&&preferences::save(value);
            memset(&value,0,sizeof(value));memset(line,0,sizeof(line));used=0;overflow=false;
            ESP_LOGI(Tag,"%s",accepted?"PANEL_CONFIG_SAVED":"PANEL_CONFIG_REJECTED");
            continue;
        }
        if(!overflow&&strncmp(line,"OTA_CONFIG ",11)==0){
            ota::Config value;
            accepted=decodeOtaConfig(line+11,value)&&ota::configure(value);
            memset(&value,0,sizeof(value));memset(line,0,sizeof(line));used=0;overflow=false;
            ESP_LOGI(Tag,"%s",accepted?"OTA_CONFIG_SAVED":"OTA_CONFIG_REJECTED");
            continue;
        }
        if(!overflow&&strncmp(line,"CONFIG ",7)==0){
            live::Config value;
            accepted=decodeConfig(line+7,value)&&live::save(value);
            memset(&value,0,sizeof(value));
        }
        memset(line,0,sizeof(line));used=0;overflow=false;
        ESP_LOGI(Tag,"%s",accepted?"CONFIG_PENDING":"CONFIG_REJECTED");
    }
}
void provisioningBegin() {
    usb_serial_jtag_driver_config_t cfg=USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    cfg.rx_buffer_size=2048;cfg.tx_buffer_size=2048;
    if(usb_serial_jtag_driver_install(&cfg)!=ESP_OK){ESP_LOGW(Tag,"USB provisioning unavailable");return;}
    usb_serial_jtag_vfs_use_driver();
    if(xTaskCreate(provisionTask,"usb-config",6144,nullptr,1,nullptr)!=pdPASS)ESP_LOGW(Tag,"USB provisioning task unavailable");
}
