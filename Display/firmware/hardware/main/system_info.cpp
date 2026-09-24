#include "preferences.h"
#include "system_info.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "live.h"
#include <cstdio>
void readPanelSystemInfo(PanelSystemInfo& out){
    esp_chip_info_t chip={};esp_chip_info(&chip);
    uint32_t flash=0;esp_flash_get_size(nullptr,&flash);
    const unsigned long seconds=esp_timer_get_time()/1000000;
    const char* reset="Sonstiger Reset";
    switch(esp_reset_reason()){
        case ESP_RST_POWERON:reset="Einschalten";break;
        case ESP_RST_SW:reset="Software-Neustart";break;
        case ESP_RST_PANIC:reset="Programmfehler";break;
        case ESP_RST_TASK_WDT:case ESP_RST_INT_WDT:case ESP_RST_WDT:reset="Watchdog";break;
        case ESP_RST_BROWNOUT:reset="Unterspannung";break;
        case ESP_RST_DEEPSLEEP:reset="Deep-Sleep";break;
        default:break;
    }
    snprintf(out.hardware,sizeof(out.hardware),
        "Waveshare ESP32-S3-Touch-LCD-5B\nSKU 28151\n\n"
        "Firmware: 0.7.2\nESP-IDF: %s\nLVGL: 8.4.0\n\n"
        "ESP32-S3: Revision %u.%u · %u Kerne\nCPU-Konfiguration: %u MHz\n"
        "Flash erkannt: %u MiB\nPSRAM erkannt: %u MiB\n\n"
        "Display: 1024 × 600 · RGB565\nTouch: GT911 · kapazitiv\n"
        "Displaypuffer: 2 × 1200 KiB · PSRAM\nUSB: Serial/JTAG · 115200 Baud",
        esp_get_idf_version(),chip.revision/100,chip.revision%100,chip.cores,CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        (unsigned)(flash/1048576),(unsigned)(esp_psram_get_size()/1048576));
    preferences::Config panelConfig;preferences::get(panelConfig);
    live::Status state;live::status(state);
    wifi_ap_record_t ap={};char signal[32]="nicht verbunden";
    if(state.wifi&&esp_wifi_sta_get_ap_info(&ap)==ESP_OK)snprintf(signal,sizeof(signal),"%d dBm",ap.rssi);
    snprintf(out.runtime,sizeof(out.runtime),
        "Laufzeit: %lu Tage %02lu:%02lu:%02lu\nLetzter Start: %s\n\n"
        "WLAN: %s · Signal: %s\nIP: %s\nZeit: %s · HTTP: %d\n%s\n\n"
        "API: %s · PRTG: %s\nLetzte Abfrage: %u ms · Fehlerfolge: %u\nWiederholung: %s · WLAN-Grund: %d\n\n"
        "Speicher frei / grösster Block (KiB)\nIntern: %u / %u\nDMA intern: %u / %u\nPSRAM: %u / %u\n"
        "Intern/DMA überlappen; nicht addieren.\n\n"
        "Aggregator: %s\nAbfrage: 15 s · Demo-Wartezeit: 30 s\n"
        "Zugangsdaten: NVS, bleiben bei Updates\nUSB-Einrichtung optional: provision.py",
        seconds/86400,(seconds/3600)%24,(seconds/60)%60,seconds%60,reset,
        state.wifi?"verbunden":"getrennt",signal,state.ip,state.clock?"synchronisiert":"wartet",state.http,state.message,
        live::apiText(state.api),live::sourceText(state.source),state.roundtripMs,state.failedAttempts,state.retryPending?"vorgemerkt":"nein",state.disconnectReason,
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)/1024),
        (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)/1024),
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA|MALLOC_CAP_8BIT)/1024),
        (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA|MALLOC_CAP_8BIT)/1024),
        (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)/1024),
        (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT)/1024),*panelConfig.origin?panelConfig.origin:"nicht eingerichtet");
}
