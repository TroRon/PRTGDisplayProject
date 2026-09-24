#include "preferences.h"
#include "ota.h"
#include "ota_catalog.h"
#include "settings.h"
#include "setup_portal.h"
#include "live.h"
#include "system_info.h"
#include "web_admin.h"
#include "factory_reset.h"
#include "reset_policy.h"
#include <lvgl.h>
LV_FONT_DECLARE(panel_font_14);
LV_FONT_DECLARE(panel_font_20);
namespace settings_impl {
static lv_obj_t *parent,*pages[6],*tabs[6],*hardwareInfo,*runtimeInfo;
static lv_obj_t *scanButton,*scanPanel,*scanList,*scanNotice,*webPassword,*webRepeat,*webNotice,*webKeyboard,*hotspotInfo,*hotspotButton;
static lv_obj_t *setupDialog=nullptr,*setupQr=nullptr;
static bool setupQrSeen=false;
static unsigned scanRevision=~0u;
static live::Scan displayedScan;
static unsigned activeTab=0;
static lv_obj_t *otaSource,*otaUrl,*otaStatus,*otaCheck,*otaInstall,*otaKeep,*otaKeyboard;
static bool installArmed=false;
static lv_obj_t *resetButton,*resetDialog=nullptr,*resetConfirm,*resetNotice;
static lv_obj_t *otaVersions,*otaCancel;
static unsigned catalogRevision=~0u;
static void disarm(lv_event_t*){installArmed=false;lv_label_set_text(lv_obj_get_child(otaInstall,0),"Installieren");}
static lv_obj_t *panelOrigin,*panelName,*panelNotice,*panelKeyboard;
static lv_obj_t *screen=nullptr,*keyboard,*ssid,*password,*token,*ntp,*statusLabel,*notice,*saveButton,*title;
static lv_obj_t* text(const char* caption,int x,int y,int width) {
    auto* obj=lv_label_create(parent);lv_label_set_text(obj,caption);lv_obj_set_pos(obj,x,y);lv_obj_set_width(obj,width);return obj;
}
static void setupClose(lv_event_t*) {
    if(setupDialog){
        if(setupQr){auto* img=lv_canvas_get_img(setupQr);if(img&&img->data)memset((void*)img->data,0,img->data_size);}
        lv_obj_del(setupDialog);setupDialog=nullptr;setupQr=nullptr;
    }
}
static void close(lv_event_t*) {
    setupClose(nullptr);
    // Remove editable secret copies when leaving this screen.
    lv_textarea_set_text(password,"");lv_textarea_set_text(token,"");lv_textarea_set_text(webPassword,"");lv_textarea_set_text(webRepeat,"");
    lv_obj_del(screen);screen=nullptr;resetDialog=nullptr;
}
static void focus(lv_event_t* e) {
    auto* target=lv_event_get_target(e);auto* kb=(target==webPassword||target==webRepeat)?webKeyboard:(target==panelOrigin||target==panelName)?panelKeyboard:target==otaUrl?otaKeyboard:keyboard;
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
static void setupShow(lv_event_t*) {
    live::Hotspot setup;live::hotspotStatus(setup);
    if(!setup.active||setupDialog){memset(setup.password,0,sizeof(setup.password));return;}
    char payload[96]={};bool valid=setupportal::wifiPayload(setup.password,payload,sizeof(payload));
    setupDialog=lv_obj_create(screen);lv_obj_set_pos(setupDialog,0,110);lv_obj_set_size(setupDialog,1024,490);
    lv_obj_clear_flag(setupDialog,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(setupDialog,0,0);
    lv_obj_set_style_bg_color(setupDialog,lv_color_hex(0x132538),0);lv_obj_set_style_text_color(setupDialog,lv_color_hex(0xE4EDF5),0);
    auto* previous=parent;parent=setupDialog;
    auto* heading=text("Mit dem Smartphone einrichten",28,18,968);lv_obj_set_style_text_font(heading,&panel_font_20,0);
    // 256 pixel QR, surrounded by a 32 pixel white quiet zone (at least four modules).
    auto* quiet=lv_obj_create(parent);lv_obj_set_pos(quiet,28,68);lv_obj_set_size(quiet,320,320);
    lv_obj_clear_flag(quiet,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(quiet,0,0);lv_obj_set_style_border_width(quiet,0,0);
    lv_obj_set_style_radius(quiet,0,0);lv_obj_set_style_bg_color(quiet,lv_color_white(),0);lv_obj_set_style_bg_opa(quiet,LV_OPA_COVER,0);
    setupQr=lv_qrcode_create(quiet,256,lv_color_black(),lv_color_white());lv_obj_center(setupQr);
    if(!valid||lv_qrcode_update(setupQr,payload,strlen(payload))!=LV_RES_OK){lv_obj_add_flag(quiet,LV_OBJ_FLAG_HIDDEN);text("QR-Code nicht verfügbar.\nWLAN manuell verbinden.",28,130,300);}
    text("1. QR-Code mit der Handy-Kamera scannen.\n    Mit SetupPRTGDisplay verbinden.\n\n2. Anmeldung im WLAN öffnen.\n    Falls keine Seite erscheint:\n    http://192.168.4.1 im Browser öffnen.\n\n3. Als admin anmelden, WLAN suchen und\n    anschliessend Panel konfigurieren.",382,68,608);
    char initial[13]={};webadmin::initialPassword(initial);
    auto* credentials=text("",382,258,608);
    lv_label_set_text_fmt(credentials,"WLAN: SetupPRTGDisplay\nWLAN-Passwort: %s\n\n%s%s",setup.password,*initial?"WebAdmin-Passwort: ":setup.initialAdmin?"WebAdmin-Passwort: ":"Dein gespeichertes WebAdmin-Passwort verwenden.",*initial?initial:setup.initialAdmin?setup.password:"");
    memset(initial,0,sizeof(initial));
    text("Ohne Internet verbunden bleiben. Hotspot: maximal 10 Minuten; endet bei WLAN-Verbindung.",28,397,968);
    button("Zurück zu Webzugang",28,430,440,setupClose);
    button("Hotspot stoppen",490,430,506,[](lv_event_t*){live::hotspotStop();setupClose(nullptr);});
    parent=previous;memset(payload,0,sizeof(payload));memset(setup.password,0,sizeof(setup.password));
}
static void resetCancel(lv_event_t*){if(resetDialog){lv_obj_del(resetDialog);resetDialog=nullptr;}}
static void resetAsk(lv_event_t*){
 ota::Status state;ota::status(state);if(!resetpolicy::allowed(state.busy,state.pending)||resetDialog)return;
 resetDialog=lv_obj_create(screen);lv_obj_set_pos(resetDialog,0,110);lv_obj_set_size(resetDialog,1024,490);
 lv_obj_set_style_bg_color(resetDialog,lv_color_hex(0x132538),0);lv_obj_set_style_pad_all(resetDialog,0,0);lv_obj_clear_flag(resetDialog,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_style_text_color(resetDialog,lv_color_hex(0xE4EDF5),0);lv_obj_t* previous=parent;parent=resetDialog;
 auto* heading=text("Werksreset: alle Kundendaten löschen?",28,24,960);lv_obj_set_style_text_font(heading,&panel_font_20,0);
 text("Gelöscht werden WLAN und Passwort, Panel-Token, Aggregator-Adresse, Displayname,\nNTP-/OTA-Einstellungen und WebAdmin-Passwort. Alle lokalen NVS-Daten werden gelöscht.\n\nDie installierte Firmware bleibt erhalten. Danach ist eine neue Einrichtung nötig.\nDas Display erhält ein neues initiales WebAdmin-Passwort.\n\nPRTG, Aggregator und externe Backups werden nicht verändert.\nWährend des Resets die Stromversorgung nicht trennen.",28,82,960);
 resetNotice=text("Dieser Vorgang kann nicht rückgängig gemacht werden.",28,290,960);
 button("Abbrechen",28,360,340,resetCancel);
 resetConfirm=button("Alle Daten löschen & neu starten",394,360,594,[](lv_event_t*){
  ota::Status status;ota::status(status);if(!resetpolicy::allowed(status.busy,status.pending)){lv_label_set_text(resetNotice,"Reset gesperrt: Update abschliessen und neue Version zuerst bestätigen.");return;}
  if(!factoryreset::request())lv_label_set_text(resetNotice,"Reset nicht abgeschlossen. Speicher-/OTA-Status prüfen; Gerät neu starten und Status kontrollieren.");
 });
 lv_obj_set_style_bg_color(resetConfirm,lv_color_hex(0xB53540),0);parent=previous;
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
    if(!installArmed){ota::Status s;ota::status(s);installArmed=true;lv_label_set_text(lv_obj_get_child(otaInstall,0),s.targetSequence<ota::Sequence?"Downgrade bestätigen":"Neustart bestätigen");return;}
    installArmed=false;lv_label_set_text(lv_obj_get_child(otaInstall,0),"Installieren");
    if(!ota::install(updateConfig()))lv_label_set_text(otaStatus,"Kanal geändert oder Update nicht bereit. Erneut prüfen.");
}
static void selectTab(unsigned index) {
    resetCancel(nullptr);setupClose(nullptr);
    activeTab=index;
    for(unsigned i=0;i<6;++i){
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
    if(!valid)lv_label_set_text(notice,"SSID bis 32 Byte, Passwort 8-63 Byte, Token leer oder 16-256 Zeichen, NTP-Hostname/IP prüfen.");
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
    const char* captions[]={"Verbindung","System-Info","Info","Firmware","Panel","Webzugang"};
    const unsigned positions[]={0,1,5,2,3,4}; // Info stays at the far right.
    for(unsigned i=0;i<6;++i){
        tabs[i]=button(captions[i],24+positions[i]*164,60,156,nullptr);
        lv_obj_add_event_cb(tabs[i],[](lv_event_t* e){selectTab((unsigned)(uintptr_t)lv_event_get_user_data(e));},LV_EVENT_CLICKED,(void*)(uintptr_t)i);
    }
    for(unsigned i=0;i<6;++i)pages[i]=page();
    parent=pages[0];lv_obj_clear_flag(parent,LV_OBJ_FLAG_SCROLLABLE);
    ssid=field("WLAN-Name (2.4 GHz)",value.ssid,24,0,310,32);
    scanButton=button("WLAN suchen",344,23,148,[](lv_event_t*){lv_obj_clear_flag(scanPanel,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(scanPanel);if(!live::scanStart()){live::Scan s;live::scanStatus(s);scanRevision=s.revision;lv_label_set_text(scanNotice,"Suche momentan nicht möglich. Netzwerk-/OTA-Status prüfen.");}else scanRevision=~0u;});
    password=field("WLAN-Passwort",value.password,516,0,484,63,true);
    token=field("Panel-Token (später ergänzbar, kein PRTG-Key)",value.token,24,83,468,256,true);
    ntp=field("NTP-Zeitserver (Hostname oder IPv4)",value.ntp,516,83,484,127);
    statusLabel=text("",24,162,976);lv_obj_set_height(statusLabel,60);lv_label_set_long_mode(statusLabel,LV_LABEL_LONG_WRAP);
    notice=text("Speichern verbindet neu. Schliessen verwirft ungespeicherte Eingaben; Registerwechsel erhält sie.",24,224,976);
    lv_obj_set_height(notice,20);lv_label_set_long_mode(notice,LV_LABEL_LONG_WRAP);
    saveButton=button("Speichern & verbinden",24,246,300,save);
    text("1.0.1 · WPA2/WPA3 · HTTPS · USB-Einrichtung optional",350,259,650);
    keyboard=lv_keyboard_create(parent);lv_obj_set_align(keyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(keyboard,0,290);lv_obj_set_size(keyboard,1024,200);
    lv_obj_set_style_text_font(keyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);
    lv_keyboard_set_textarea(keyboard,ssid);
    scanPanel=lv_obj_create(parent);lv_obj_set_pos(scanPanel,12,0);lv_obj_set_size(scanPanel,1000,480);lv_obj_set_style_pad_all(scanPanel,0,0);lv_obj_set_style_bg_color(scanPanel,lv_color_hex(0x132538),0);lv_obj_clear_flag(scanPanel,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_text_color(scanPanel,lv_color_hex(0xE4EDF5),0);
    parent=scanPanel;scanNotice=text("WLAN suchen …",16,16,750);
    button("Zurück",814,6,160,[](lv_event_t*){lv_obj_add_flag(scanPanel,LV_OBJ_FLAG_HIDDEN);});
    scanList=lv_list_create(scanPanel);lv_obj_set_pos(scanList,16,64);lv_obj_set_size(scanList,960,398);lv_obj_add_flag(scanPanel,LV_OBJ_FLAG_HIDDEN);scanRevision=~0u;
    lv_obj_set_style_bg_color(scanList,lv_color_hex(0x132538),0);lv_obj_set_style_border_color(scanList,lv_color_hex(0x355269),0);
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
    otaVersions=lv_dropdown_create(parent);lv_obj_set_pos(otaVersions,440,38);lv_obj_set_size(otaVersions,560,36);
    lv_dropdown_set_symbol(otaVersions,"v");lv_dropdown_set_options(otaVersions,"Zuerst Versionen prüfen");
    catalogRevision=~0u;
    lv_obj_add_event_cb(otaVersions,[](lv_event_t*){disarm(nullptr);ota::choose(lv_dropdown_get_selected(otaVersions));},LV_EVENT_VALUE_CHANGED,nullptr);
    lv_obj_add_event_cb(otaSource,disarm,LV_EVENT_VALUE_CHANGED,nullptr);
    otaUrl=field("Direktkanal: HTTPS-Adresse der manifest.json (beim Aggregator nicht benötigt)",update.url,24,84,976,383);
    lv_obj_add_event_cb(otaUrl,disarm,LV_EVENT_VALUE_CHANGED,nullptr);
    otaStatus=text("",24,158,976);lv_obj_set_height(otaStatus,62);
    otaCheck=button("Versionen prüfen",24,234,220,updateCheck);
    otaInstall=button("Installieren",258,234,250,updateInstall);
    otaKeep=button("Diese Version behalten",522,234,290,[](lv_event_t*){ota::confirm();});
    otaCancel=button("Abbrechen",826,234,174,disarm);
    resetButton=button("Werksreset",24,300,300,resetAsk);
    text("Löscht alle lokalen Kundendaten nach Rückfrage",344,312,640);
    otaKeyboard=lv_keyboard_create(parent);lv_obj_set_align(otaKeyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(otaKeyboard,0,290);lv_obj_set_size(otaKeyboard,1024,200);
    lv_obj_set_style_text_font(otaKeyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);lv_keyboard_set_textarea(otaKeyboard,otaUrl);
    lv_obj_add_flag(otaKeyboard,LV_OBJ_FLAG_HIDDEN);
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
    parent=pages[5];lv_obj_clear_flag(parent,LV_OBJ_FLAG_SCROLLABLE);
    webPassword=field("Neues WebAdmin-Passwort (5-63 UTF-8-Bytes)","",24,0,468,63,true);
    webRepeat=field("Passwort wiederholen", "",516,0,484,63,true);
    webNotice=text("",24,84,976);lv_obj_set_height(webNotice,50);
    hotspotInfo=text("",24,140,976);lv_obj_set_height(hotspotInfo,68);
    text("HTTP ist unverschlüsselt: nur im vertrauenswürdigen lokalen Netz verwenden. Passwort bleibt gespeichert.",24,212,976);
    button("Passwort speichern",24,234,230,[](lv_event_t*){
        const char* first=lv_textarea_get_text(webPassword);const char* second=lv_textarea_get_text(webRepeat);
        bool ok=!strcmp(first,second)&&webadmin::password(first);
        lv_label_set_text(webNotice,ok?"Gespeichert. Browser neu anmelden.":"Nicht gespeichert. Gleiche Passwörter mit 5-63 Byte verwenden; während OTA warten.");
        if(ok){lv_textarea_set_text(webPassword,"");lv_textarea_set_text(webRepeat,"");}
    });
    hotspotButton=button("Hotspot starten",264,234,210,[](lv_event_t*){if(!live::hotspotStart())lv_label_set_text(hotspotInfo,"Hotspot nicht gestartet: nur ohne WLAN-Verbindung und ohne laufendes OTA möglich.");});
    button("Hotspot stoppen",484,234,210,[](lv_event_t*){live::hotspotStop();});
    button("Web deaktivieren",704,234,296,[](lv_event_t*){if(webadmin::disable()){live::hotspotStop();lv_label_set_text(webNotice,"Webzugang deaktiviert. Passwort neu setzen zum Aktivieren.");}});
    button("WLAN-QR-Code anzeigen",24,292,450,setupShow);
      webKeyboard=lv_keyboard_create(parent);lv_obj_set_align(webKeyboard,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(webKeyboard,0,290);lv_obj_set_size(webKeyboard,1024,200);
    lv_obj_set_style_text_font(webKeyboard,LV_FONT_DEFAULT,LV_PART_ITEMS);lv_keyboard_set_textarea(webKeyboard,webPassword);lv_obj_add_flag(webKeyboard,LV_OBJ_FLAG_HIDDEN);
    installArmed=false;
    selectTab(0);
    memset(&value,0,sizeof(value));settingsTick();
}
void settingsTick() {
    using namespace settings_impl;
    if(!screen)return;
    live::Hotspot qrState;live::hotspotStatus(qrState);bool qrActive=qrState.active;memset(qrState.password,0,sizeof(qrState.password));
    if(!qrActive){setupClose(nullptr);setupQrSeen=false;}
    if(qrActive&&activeTab==5&&!setupQrSeen){setupQrSeen=true;setupShow(nullptr);}
    const char* headings[]={"Einstellungen · WLAN und Live-Daten","Einstellungen · System-Informationen","Einstellungen · Info","Einstellungen · Firmware-Update","Einstellungen · Panel","Einstellungen · Webzugang"};
    lv_label_set_text(title,hardwareDemoActive()?"Einstellungen · DEMO aktiv · Fiktive Daten":headings[activeTab]);
    if(activeTab==1){
        PanelSystemInfo info;readPanelSystemInfo(info);
        lv_label_set_text(hardwareInfo,info.hardware);lv_label_set_text(runtimeInfo,info.runtime);
    }
    if(activeTab==3){
        ota::Status update;ota::status(update);
        if(catalogRevision!=update.revision){
            catalogRevision=update.revision;char options[768]={};
            for(unsigned i=0;i<update.count;++i){size_t used=strlen(options);snprintf(options+used,sizeof(options)-used,"%s%s · %s",i?"\n":"",update.versions[i],ota::actionText(ota::versionSequence(update.versions[i])));}
            lv_dropdown_set_options(otaVersions,*options?options:"Zuerst Versionen prüfen");lv_dropdown_set_selected(otaVersions,update.chosen);
        }
        char confirmation[128];
        if(update.pending)snprintf(confirmation,sizeof(confirmation),"Bestätigung erforderlich: %u s bis Rückfall",update.seconds);
        else if(update.available&&update.targetSequence<ota::Sequence)snprintf(confirmation,sizeof(confirmation),"Downgrade: ältere Funktionen! Nach Neustart innert 120 s bestätigen.");
        else snprintf(confirmation,sizeof(confirmation),"Nach Installation: Anzeige prüfen und innert 120 s bestätigen");
        lv_label_set_text_fmt(otaStatus,"Installiert: %s · Ziel: %s · %u KiB · Fortschritt: %u %%\n%s\n%s",ota::Version,*update.offered?update.offered:"noch nicht geprüft",update.targetSize/1024,update.progress,update.message,confirmation);
        auto enabled=[](lv_obj_t* obj,bool yes){if(yes)lv_obj_clear_state(obj,LV_STATE_DISABLED);else lv_obj_add_state(obj,LV_STATE_DISABLED);};
        enabled(otaCheck,update.ready&&!update.busy&&!update.pending);
        enabled(otaInstall,update.ready&&!update.busy&&!update.pending&&update.available);
        enabled(otaVersions,!update.busy&&!update.pending&&update.count>0);
        if(installArmed&&!update.busy&&!update.pending)lv_obj_clear_flag(otaCancel,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(otaCancel,LV_OBJ_FLAG_HIDDEN);
        enabled(otaSource,!update.busy&&!update.pending);enabled(otaUrl,!update.busy&&!update.pending&&lv_dropdown_get_selected(otaSource)==1);
        if(update.pending)lv_obj_clear_flag(otaKeep,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(otaKeep,LV_OBJ_FLAG_HIDDEN);
        enabled(otaKeep,update.ready&&!update.busy);
        enabled(resetButton,resetpolicy::allowed(update.busy,update.pending));
        if(resetDialog)enabled(resetConfirm,resetpolicy::allowed(update.busy,update.pending));
    }
    live::Status value;live::status(value);
    if(activeTab==5&&!*lv_textarea_get_text(webPassword)){
        webadmin::Status web;webadmin::status(web);
        char initial[13]={};webadmin::initialPassword(initial);char info[128];
        if(*initial)snprintf(info,sizeof(info),"Initiales Web-Passwort: %s · bitte ändern",initial);else snprintf(info,sizeof(info),"%s",web.message);
        memset(initial,0,sizeof(initial));
        lv_label_set_text_fmt(webNotice,"%s\n%s%s%s",info,web.running&&value.wifi?"Browser: http://":"",web.running&&value.wifi?value.ip:"",web.running&&value.wifi?" · Benutzer admin":"");
        memset(info,0,sizeof(info));
    }
    if(activeTab==5){
        live::Hotspot setup;live::hotspotStatus(setup);
        if(setup.active)lv_label_set_text_fmt(hotspotInfo,"SSID: SetupPRTGDisplay · Noch %u s\nWLAN-Passwort: %s\nhttp://192.168.4.1 · Benutzer admin · %s",setup.seconds,setup.password,setup.initialAdmin?"Web-Passwort = WLAN-Passwort (bitte notieren)":"Dein bestehendes WebAdmin-Passwort verwenden");
        else lv_label_set_text_fmt(hotspotInfo,"%s\nOhne WLAN: Hotspot starten, Smartphone verbinden, dann http://192.168.4.1 öffnen.",setup.message);
        if(value.wifi||setup.pending||setup.active)lv_obj_add_state(hotspotButton,LV_STATE_DISABLED);else lv_obj_clear_state(hotspotButton,LV_STATE_DISABLED);
        memset(setup.password,0,sizeof(setup.password));
    }
    if(activeTab==0&&!lv_obj_has_flag(scanPanel,LV_OBJ_FLAG_HIDDEN)){
        live::Scan result;live::scanStatus(result);
        if(scanRevision!=result.revision){
            scanRevision=result.revision;displayedScan=result;lv_label_set_text(scanNotice,result.message);lv_obj_clean(scanList);
            for(unsigned i=0;i<result.count;++i){
                char caption[112];snprintf(caption,sizeof(caption),"%s · %d dBm%s",result.networks[i].ssid,result.networks[i].signal,result.networks[i].supported?"":" · nicht unterstützt");
                auto* choice=lv_list_add_btn(scanList,nullptr,caption);
                lv_obj_set_style_bg_color(choice,lv_color_hex(0x1B3248),0);lv_obj_set_style_text_color(choice,lv_color_hex(0xE4EDF5),0);
                lv_obj_set_style_text_color(choice,lv_color_hex(0x8395A5),LV_STATE_DISABLED);
                if(!result.networks[i].supported)lv_obj_add_state(choice,LV_STATE_DISABLED);
                lv_obj_add_event_cb(choice,[](lv_event_t* e){
                    const auto& scan=displayedScan;unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(e);if(i>=scan.count||scan.busy)return;
                    if(strcmp(lv_textarea_get_text(ssid),scan.networks[i].ssid))lv_textarea_set_text(password,"");
                    lv_textarea_set_text(ssid,scan.networks[i].ssid);lv_obj_add_flag(scanPanel,LV_OBJ_FLAG_HIDDEN);
                    lv_keyboard_set_textarea(keyboard,password);lv_obj_clear_flag(keyboard,LV_OBJ_FLAG_HIDDEN);lv_obj_add_state(password,LV_STATE_FOCUSED);
                },LV_EVENT_CLICKED,(void*)(uintptr_t)i);
            }
        }
    }
    lv_obj_set_style_text_color(statusLabel,lv_color_hex(value.startupFailed?0xFF8080:0xE4EDF5),0);
    lv_label_set_text_fmt(statusLabel,"WLAN: %s   IP: %s   Zeit: %s   HTTP: %d\nAPI: %s · PRTG: %s%s\n%s",
        value.wifi?"verbunden":"getrennt",value.ip,value.clock?"synchronisiert":"wartet",value.http,live::apiText(value.api),live::sourceText(value.source),value.retryPending?" · Wiederholung folgt":"",value.message);
}



void settingsShowFirmware(){if(settings_impl::screen)settings_impl::selectTab(3);}

void settingsShowWebAccess(){if(settings_impl::screen)settings_impl::selectTab(5);}
