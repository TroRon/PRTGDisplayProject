#include "preferences.h"
#include "ota.h"
#include "settings.h"
#include "live.h"
#include "system_info.h"
#include <lvgl.h>
LV_FONT_DECLARE(panel_font_14);
LV_FONT_DECLARE(panel_font_20);
namespace settings_impl {
static lv_obj_t *parent,*pages[5],*tabs[5],*hardwareInfo,*runtimeInfo;
static unsigned activeTab=0;
static lv_obj_t *otaSource,*otaUrl,*otaStatus,*otaCheck,*otaInstall,*otaKeep,*otaKeyboard;
static bool installArmed=false;
static lv_obj_t *panelOrigin,*panelName,*panelNotice,*panelKeyboard;
static lv_obj_t *screen=nullptr,*keyboard,*ssid,*password,*token,*ntp,*statusLabel,*notice,*saveButton,*title;
static lv_obj_t* text(const char* caption,int x,int y,int width) {
    auto* obj=lv_label_create(parent);lv_label_set_text(obj,caption);lv_obj_set_pos(obj,x,y);lv_obj_set_width(obj,width);return obj;
}
static void close(lv_event_t*) {
    // Remove editable secret copies when leaving this screen.
    lv_textarea_set_text(password,"");lv_textarea_set_text(token,"");
    lv_obj_del(screen);screen=nullptr;
}
static void focus(lv_event_t* e) {
    auto* target=lv_event_get_target(e);auto* kb=(target==panelOrigin||target==panelName)?panelKeyboard:target==otaUrl?otaKeyboard:keyboard;
    lv_keyboard_set_textarea(kb,target);
    lv_obj_clear_flag(kb,LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t* field(const char* caption,const char* value,int x,int y,int width,unsigned limit,bool secret=false) {
    text(caption,x,y,width);
    auto* obj=lv_textarea_create(parent);lv_obj_set_pos(obj,x,y+23);lv_obj_set_size(obj,width,43);
    lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_textarea_set_one_line(obj,true);lv_textarea_set_max_length(obj,limit);
    lv_textarea_set_password_mode(obj,secret);lv_textarea_set_password_show_time(obj,0);
    lv_textarea_set_text(obj,value);lv_obj_add_event_cb(obj,focus,LV_EVENT_FOCUSED,nullptr);
    lv_obj_add_event_cb(obj,focus,LV_EVENT_CLICKED,nullptr);return obj;
}
static lv_obj_t* button(const char* caption,int x,int y,int width,lv_event_cb_t cb) {
    auto* obj=lv_btn_create(parent);lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,width,43);
    auto* label=lv_label_create(obj);lv_label_set_text(label,caption);lv_obj_center(label);
    if(cb)lv_obj_add_event_cb(obj,cb,LV_EVENT_CLICKED,nullptr);
    return obj;
}
static ota::Config updateConfig(){
    ota::Config c;c.direct=lv_dropdown_get_selected(otaSource)==1;
    snprintf(c.url,sizeof(c.url),"%s",lv_textarea_get_text(otaUrl));return c;
}
static void updateCheck(lv_event_t*){
    installArmed=false;lv_label_set_text(lv_obj_get_child(otaInstall,0),"Installieren");
    if(!ota::check(updateConfig()))lv_label_set_text(otaStatus,"Update-Prüfung nicht gestartet: HTTPS-Adresse/Status prüfen.");
}
static void updateInstall(lv_event_t*){
    if(!installArmed){installArmed=true;lv_label_set_text(lv_obj_get_child(otaInstall,0),"Neustart bestätigen");return;}
    installArmed=false;lv_label_set_text(lv_obj_get_child(otaInstall,0),"Installieren");
    if(!ota::install(updateConfig()))lv_label_set_text(otaStatus,"Kanal geändert oder Update nicht bereit. Erneut prüfen.");
}
static void selectTab(unsigned index) {
    activeTab=index;
    for(unsigned i=0;i<5;++i){
        if(i==index)lv_obj_clear_flag(pages[i],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(pages[i],LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(tabs[i],lv_color_hex(i==index?0x218CE0:0x1B3248),0);
    }
    lv_obj_scroll_to_y(screen,0,LV_ANIM_OFF);
    settingsTick();
}
static lv_obj_t* page() {
    auto* obj=lv_obj_create(screen);lv_obj_set_pos(obj,0,110);lv_obj_set_size(obj,1024,490);
    lv_obj_set_style_pad_all(obj,0,0);lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_bg_opa(obj,LV_OPA_TRANSP,0);lv_obj_set_scroll_dir(obj,LV_DIR_VER);
    lv_obj_set_style_text_color(obj,lv_color_hex(0xE4EDF5),0);
    return obj;
}
static bool copy(char* dest,size_t length,lv_obj_t* field) {
    const char* value=lv_textarea_get_text(field);if(strlen(value)>=length)return false;
    memcpy(dest,value,strlen(value)+1);return true;
}
static void save(lv_event_t*) {
    live::Status status;live::status(status);
    if(status.startupFailed){lv_label_set_text(notice,"Netzwerk nicht verfügbar. Heap-Diagnose im USB-Log prüfen; Konfiguration bleibt erhalten.");return;}
    live::Config value;
    bool valid=copy(value.ssid,sizeof(value.ssid),ssid)&&copy(value.password,sizeof(value.password),password)&&
        copy(value.token,sizeof(value.token),token)&&copy(value.ntp,sizeof(value.ntp),ntp)&&live::validate(value);
    if(!valid)lv_label_set_text(notice,"Bitte prüfen: SSID bis 32 Byte, Passwort 8-63 Byte, Panel-Token 16-256 Zeichen, NTP-Hostname/IP.");
    else lv_label_set_text(notice,live::save(value)?"Übernahme angefordert. Der Verbindungsstatus zeigt das Ergebnis.":"Ein Speichervorgang läuft noch. Bitte kurz warten.");
    memset(&value,0,sizeof(value));
}
}
void settingsOpen() {
    using namespace settings_impl;
    if(screen)return;
    live::Config value;live::config(value);
    screen=lv_obj_create(lv_layer_top());lv_obj_set_pos(screen,0,0);lv_obj_set_size(screen,1024,600);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(screen,0,0);lv_obj_set_style_border_width(screen,0,0);
    lv_obj_set_style_radius(screen,0,0);lv_obj_set_style_bg_color(screen,lv_color_hex(0x0B1825),0);
    lv_obj_set_style_text_color(screen,lv_color_hex(0xE4EDF5),0);lv_obj_set_style_text_font(screen,&panel_font_14,0);
    parent=screen;
    title=text("Einstellungen · WLAN und Live-Daten",24,18,780);lv_obj_set_style_text_font(title,&panel_font_20,0);
    button("Schliessen",852,12,148,close);
    const char* captions[]={"Verbindung","System-Informationen","Copyright und Idee","Firmware","Panel"};
    const unsigned positions[]={0,1,4,2,3}; // Copyright stays at the far right.
    for(unsigned i=0;i<5;++i){
        tabs[i]=button(captions[i],24+positions[i]*196,60,190,nullptr);
        lv_obj_add_event_cb(tabs[i],[](lv_event_t* e){selectTab((unsigned)(uintptr_t)lv_event_get_user_data(e));},LV_EVENT_CLICKED,(void*)(uintptr_t)i);
    }
    for(unsigned i=0;i<5;++i)pages[i]=page();
    parent=pages[0];lv_obj_clear_flag(parent,LV_OBJ_FLAG_SCROLLABLE);
    ssid=field("WLAN-Name (2.4 GHz)",value.ssid,24,0,468,32);
    password=field("WLAN-Passwort",value.password,516,0,484,63,true);
    token=field("Panel-Token (kein PRTG-Key)",value.token,24,83,468,256,true);
    ntp=field("NTP-Zeitserver (Hostname oder IPv4)",value.ntp,516,83,484,127);
    statusLabel=text("",24,162,976);lv_obj_set_height(statusLabel,60);lv_label_set_long_mode(statusLabel,LV_LABEL_LONG_WRAP);
    notice=text("Speichern verbindet neu. Schliessen verwirft ungespeicherte Eingaben; Registerwechsel erhält sie.",24,224,976);
    lv_obj_set_height(notice,20);lv_label_set_long_mode(notice,LV_LABEL_LONG_WRAP);
    saveButton=button("Speichern & verbinden",24,246,300,save);
    text("0.7.2 · WPA2/WPA3 · HTTPS · USB-Einrichtung optional",350,259,650);
    keyboard=lv_keyboard_create(parent);lv_obj_set_align(keyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(keyboard,0,290);lv_obj_set_size(keyboard,1024,200);
    lv_obj_set_style_text_font(keyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);
    lv_keyboard_set_textarea(keyboard,ssid);
    parent=pages[1];
    hardwareInfo=text("",24,8,468);runtimeInfo=text("",516,8,484);
    lv_obj_set_style_text_line_space(hardwareInfo,3,0);lv_obj_set_style_text_line_space(runtimeInfo,3,0);
    parent=pages[2];
    auto* heading=text("PRTG Display · Infrastruktur auf einen Blick",24,12,976);
    lv_obj_set_style_text_font(heading,&panel_font_20,0);
    text("Copyright (c) 2026 Ronny Troxler / EagleNET\nIdee und Projektleitung: Ronny Troxler\nEntwicklung mit Unterstützung von OpenAI Codex",24,58,976);
    text("Die Idee\nEin eigenständiges Touch-Panel macht bestehende Monitoring-Daten im Alltag sichtbar.\nPRTG erfasst die Infrastruktur, der Aggregator bündelt sie, das Panel zeigt den Zustand.\nKlare Systemkarten, echte Umlaute und Details per Touch statt einer Blackbox.",24,136,976);
    text("Unser Anspruch\nEchte Fehler und fehlende Messwerte bleiben erkennbar. Für Vorführungen gibt es einen\ndeutlich markierten Demo-Modus mit fiktiven Daten. DEMO bedeutet keine Live-Überwachung.\nNetzwerk- und Dienste-Daten sind in der Demo Beispiele; die Live-Anbindung ist noch offen.",24,244,976);
    text("Technische Basis und Drittlizenzen\nWaveshare · Espressif ESP-IDF · LVGL · ArduinoJson · Montserrat\nDie jeweiligen Copyright- und Lizenzhinweise bleiben gültig: siehe LICENSES im Release.\nProjekt: github.com/TroRon/PRTGDisplayProject",24,352,976);
    parent=pages[3];lv_obj_clear_flag(parent,LV_OBJ_FLAG_SCROLLABLE);
    text("Signierte Firmware · Installation startet das Display neu · WLAN und Token bleiben erhalten",24,8,976);
    ota::Config update;ota::config(update);
    otaSource=lv_dropdown_create(parent);lv_obj_set_pos(otaSource,24,38);lv_obj_set_size(otaSource,400,36);
    lv_dropdown_set_symbol(otaSource,"v");lv_dropdown_set_options(otaSource,"Aggregator (intern)\nDirekter HTTPS-Download");lv_dropdown_set_selected(otaSource,update.direct?1:0);
    otaUrl=field("Direktkanal: HTTPS-Adresse der manifest.json (beim Aggregator nicht benötigt)",update.url,24,84,976,383);
    otaStatus=text("",24,158,976);lv_obj_set_height(otaStatus,62);
    otaCheck=button("Update prüfen",24,234,220,updateCheck);
    otaInstall=button("Installieren",258,234,250,updateInstall);
    otaKeep=button("Diese Version behalten",522,234,290,[](lv_event_t*){ota::confirm();});
    otaKeyboard=lv_keyboard_create(parent);lv_obj_set_align(otaKeyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(otaKeyboard,0,290);lv_obj_set_size(otaKeyboard,1024,200);
    lv_obj_set_style_text_font(otaKeyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);lv_keyboard_set_textarea(otaKeyboard,otaUrl);
    parent=pages[4];lv_obj_clear_flag(parent,LV_OBJ_FLAG_SCROLLABLE);
    preferences::Config identity;preferences::get(identity);
    panelOrigin=field("Aggregator-Adresse: https://hostname oder https://hostname:port (ohne Pfad)",identity.origin,24,0,976,255);
    panelName=field("Anzeigename dieses Panels (maximal 48 UTF-8-Bytes)",identity.name,24,83,976,48);
    panelNotice=text("Adresse und Name gelten auch nach Neustart. Systemnamen der Messwerte kommen vom Aggregator.",24,170,976);
    button("Panel speichern",24,246,300,[](lv_event_t*){
        preferences::Config c;snprintf(c.origin,sizeof(c.origin),"%s",lv_textarea_get_text(panelOrigin));
        size_t n=strlen(c.origin);if(n&&c.origin[n-1]=='/')c.origin[n-1]=0;
        const char* name=lv_textarea_get_text(panelName);
        if(strlen(name)>=sizeof(c.name)){lv_label_set_text(panelNotice,"Name ist zu lang (maximal 48 UTF-8-Bytes).");return;}
        snprintf(c.name,sizeof(c.name),"%s",name);
        lv_label_set_text(panelNotice,preferences::save(c)?"Panel-Einstellungen gespeichert. Neue Adresse gilt ab der nächsten Abfrage.":"Nicht gespeichert: HTTPS-Adresse/Name prüfen; während OTA kurz warten.");
    });
    panelKeyboard=lv_keyboard_create(parent);lv_obj_set_align(panelKeyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(panelKeyboard,0,290);lv_obj_set_size(panelKeyboard,1024,200);
    lv_obj_set_style_text_font(panelKeyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);lv_keyboard_set_textarea(panelKeyboard,panelOrigin);
    installArmed=false;
    selectTab(0);
    memset(&value,0,sizeof(value));settingsTick();
}
void settingsTick() {
    using namespace settings_impl;
    if(!screen)return;
    const char* headings[]={"Einstellungen · WLAN und Live-Daten","Einstellungen · System-Informationen","Einstellungen · Copyright und Idee","Einstellungen · Firmware-Update","Einstellungen · Panel"};
    lv_label_set_text(title,hardwareDemoActive()?"Einstellungen · DEMO aktiv · Fiktive Daten":headings[activeTab]);
    if(activeTab==1){
        PanelSystemInfo info;readPanelSystemInfo(info);
        lv_label_set_text(hardwareInfo,info.hardware);lv_label_set_text(runtimeInfo,info.runtime);
    }
    if(activeTab==3){
        ota::Status update;ota::status(update);
        char confirmation[128];
        if(update.pending)snprintf(confirmation,sizeof(confirmation),"Bestätigung erforderlich: %u s bis Rückfall",update.seconds);
        else snprintf(confirmation,sizeof(confirmation),"Nach dem Update: neue Version innert 120 s bestätigen");
        lv_label_set_text_fmt(otaStatus,"Installiert: %s · Angebot: %s · Fortschritt: %u %%\n%s\n%s",ota::Version,*update.offered?update.offered:"noch nicht geprüft",update.progress,update.message,confirmation);
        auto enabled=[](lv_obj_t* obj,bool yes){if(yes)lv_obj_clear_state(obj,LV_STATE_DISABLED);else lv_obj_add_state(obj,LV_STATE_DISABLED);};
        enabled(otaCheck,update.ready&&!update.busy&&!update.pending);
        enabled(otaInstall,update.ready&&!update.busy&&!update.pending&&update.available);
        enabled(otaSource,!update.busy&&!update.pending);enabled(otaUrl,!update.busy&&!update.pending&&lv_dropdown_get_selected(otaSource)==1);
        if(update.pending)lv_obj_clear_flag(otaKeep,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(otaKeep,LV_OBJ_FLAG_HIDDEN);
        enabled(otaKeep,update.ready&&!update.busy);
    }
    live::Status value;live::status(value);
    lv_obj_set_style_text_color(statusLabel,lv_color_hex(value.startupFailed?0xFF8080:0xE4EDF5),0);
    lv_label_set_text_fmt(statusLabel,"WLAN: %s   IP: %s   Zeit: %s   HTTP: %d\nAPI: %s · PRTG: %s%s\n%s",
        value.wifi?"verbunden":"getrennt",value.ip,value.clock?"synchronisiert":"wartet",value.http,live::apiText(value.api),live::sourceText(value.source),value.retryPending?" · Wiederholung folgt":"",value.message);
}



void settingsShowFirmware(){if(settings_impl::screen)settings_impl::selectTab(3);}
