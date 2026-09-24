#include "ota.h"
#include "esp_task_wdt.h"
#include "Arduino.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_lcd_panel_rgb.h"
#include "memory_diagnostics.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "waveshare_rgb_lcd_port.h"
#include "board_port.h"
#include "panel_ui.h"
#include "live.h"
#include "settings.h"
#include "demo_mode.h"
#include "link_policy.h"
#include <cstring>
#include <new>

static const char *TAG="EAGLENET";
static esp_lcd_touch_handle_t touch;
static panel::State state;
void provisioningBegin();

void boardStatus(panel::Status) {} // No external LED hardware in this release.
void boardDim(bool dimmed) {
    // Existing UI dims with its overlay. Do not switch the panel off on inactivity.
    ESP_LOGI(TAG,"UI dim=%d; backlight remains on",dimmed);
}
bool boardBegin(){return true;}
void boardTouch(int16_t &x,int16_t &y,bool &pressed) {
    pressed=false;
    if(!touch)return;
    if(esp_lcd_touch_read_data(touch)!=ESP_OK)return;
    esp_lcd_touch_point_data_t point={};uint8_t count=0;
    pressed=esp_lcd_touch_get_data(touch,&point,&count,1)==ESP_OK&&count>0;
    if(pressed){x=point.x;y=point.y;}
    // Do not log keyboard touch coordinates: they can reveal entered secrets.
}
extern "C" void app_main() {
    ESP_LOGI(TAG,"0.8.0 | Waveshare ESP32-S3-Touch-LCD-5B SKU 28151 | LIVE + LABELLED OFFLINE DEMO");
    ESP_LOGI(TAG,"IDF %s; reset=%d; RGB 1024x600; flash QIO; PSRAM OPI 80 MHz",esp_get_idf_version(),esp_reset_reason());
    uint32_t flashBytes=0;ESP_ERROR_CHECK(esp_flash_get_size(nullptr,&flashBytes));
    if(flashBytes!=16*1024*1024 || !esp_psram_is_initialized() || esp_psram_get_size()!=8*1024*1024){
        ESP_LOGE(TAG,"Unexpected memory: flash=%lu PSRAM=%u; display not started",(unsigned long)flashBytes,(unsigned)esp_psram_get_size());
        return;
    }
    ESP_LOGI(TAG,"Memory verified: 16 MB flash, 8 MB PSRAM; free PSRAM=%u",(unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    logMemory("before_rgb");
    live::prepare();
    // FULL reuses two PSRAM panel buffers. PARTIAL in adapter 0.5.2 allocates
    // a separate INTERNAL strip, regardless of profile.use_psram.
    const auto mode=ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_FULL;
    const auto rotation=ESP_LV_ADAPTER_ROTATE_0;
    esp_lcd_panel_handle_t panel=nullptr;
    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(mode,rotation,&panel,&touch));
    logMemory("after_rgb");
    void *fb0=nullptr,*fb1=nullptr;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel,2,&fb0,&fb1));
    ESP_LOGI(TAG,"RGB buffers=2 bytes_each=1228800 PSRAM=%d/%d",esp_ptr_external_ram(fb0),esp_ptr_external_ram(fb1));
    if(!fb0||!fb1||fb0==fb1||!esp_ptr_external_ram(fb0)||!esp_ptr_external_ram(fb1)){ESP_LOGE(TAG,"RGB buffer placement invalid");return;}
    esp_lv_adapter_config_t adapter=ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter.task_stack_size=12*1024;adapter.stack_in_psram=true;
    logMemory("before_lvgl_init");
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter));
    logMemory("after_lvgl_init");
    esp_lv_adapter_display_config_t display=ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(panel,nullptr,1024,600,rotation);
    display.tear_avoid_mode=mode;display.profile.buffer_height=600;
    display.profile.use_psram=true;
    logMemory("before_lvgl_register");
    auto* registered=esp_lv_adapter_register_display(&display);
    logMemory("after_lvgl_register");
    if(!registered){ESP_LOGE(TAG,"Display registration failed");return;}
    auto* draw=registered->driver->draw_buf;
    const bool reuse=draw&&draw->size==1024*600&&registered->driver->full_refresh&&
        ((draw->buf1==fb0&&draw->buf2==fb1)||(draw->buf1==fb1&&draw->buf2==fb0));
    ESP_LOGI(TAG,"LVGL mode=DOUBLE_FULL shared_panel_buffers=%d extra_draw_bytes=0",reuse);
    if(!reuse){ESP_LOGE(TAG,"Unexpected LVGL buffer fallback; stopping startup");return;}
    ESP_ERROR_CHECK(esp_lv_adapter_start());
    logMemory("after_lvgl_start");
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    uiBegin(nullptr);
    hardwareSettingsAttach();
    state.fail("Verbindung wird vorbereitet; Demo nach 30 Sekunden ohne Daten");uiUpdate(state.value);
    esp_lv_adapter_unlock();
    ESP_ERROR_CHECK(waveshare_rgb_lcd_backlight_on());
    logMemory("after_ui");
    const bool networkReady=live::begin();
    logMemory("after_network_begin");
    if(!networkReady){
        ESP_LOGE(TAG,"Network unavailable; UI stays active; live data unknown");
        ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));state.fail("Netzwerk nicht verfügbar - Live-Daten unbekannt");uiUpdate(state.value);esp_lv_adapter_unlock();
    }
    ota::begin(networkReady);
    ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));
    ota::Status bootStatus;ota::status(bootStatus);
    if(bootStatus.pending){ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));settingsOpen();settingsShowFirmware();esp_lv_adapter_unlock();}
    provisioningBegin();
    logMemory("after_usb");
    // Keep the overview visible for an unconfigured showroom device.
    // Settings remain reachable via the existing button, without credentials.
    auto* demoMemory=heap_caps_malloc(sizeof(panel::Snapshot),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    auto* demo=demoMemory?new(demoMemory) panel::Snapshot:nullptr;
    if(!demo)ESP_LOGE(TAG,"Demo allocation failed; live/unknown display remains available");
    presentation::Controller presentation;
    bool sourcesUnavailable=false,showingDemo=false;
    uint32_t lastDemo=0;
    presentation.update(state.value,false,millis());
    ESP_LOGI(TAG,"Display/UI active; network_ready=%d",networkReady);
    uint32_t lastLog=0,lastSettings=0;
    for(;;){
        esp_task_wdt_reset();ota::tick();
        ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
        bool changed=live::read(state.value,sourcesUnavailable);
        // Sample after reading: a freshly queued snapshot can be newer than a pre-read timestamp.
        const uint32_t now=millis();
        if(state.value.apiAvailable&&uint32_t(now-state.value.receivedAtMs)>panel::LinkTimeoutMs){link_policy::unavailable(state.value,"Keine aktuellen Aggregator-Daten");changed=true;}
        const bool nextDemo=demo&&presentation.update(state.value,sourcesUnavailable,now);
        const bool switched=nextDemo!=showingDemo;
        if(switched)ESP_LOGW(TAG,"Presentation=%s; live_api=%d sources_unavailable=%d",nextDemo?"DEMO_SYNTHETIC":"LIVE",state.value.apiAvailable,sourcesUnavailable);
        showingDemo=nextDemo;
        if(showingDemo){
            if(switched||uint32_t(now-lastDemo)>=5000){presentation::generate(*demo,now);uiUpdate(*demo);lastDemo=now;}
        }else if(changed||switched)uiUpdate(state.value);
        if(uint32_t(now-lastSettings)>1000){settingsTick();lastSettings=now;}
        uiLoop(now);
        esp_lv_adapter_unlock();
        if(uint32_t(now-lastLog)>30000){lastLog=now;logMemory("alive");}
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
