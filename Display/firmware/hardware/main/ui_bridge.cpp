#include "preferences.h"
// Keep simulator source unchanged. Adapter owns LVGL timer processing.
#include <lvgl.h>
static uint32_t hardware_timer_owned_by_adapter(){return 0;}
#define lv_timer_handler hardware_timer_owned_by_adapter
#define uiUpdate sharedUiUpdate
#define uiLoop sharedUiLoop
#include "../../src/panel_ui.cpp"
#undef uiUpdate
#undef uiLoop
#include "settings.h"
void hardwareSettingsAttach() {
    lv_label_set_long_mode(brand,LV_LABEL_LONG_DOT);
    lv_obj_set_height(brand,26);
    advance=settingsOpen;
    lv_label_set_text(lv_obj_get_child(demoButton,0),"Einstellungen");
    lv_obj_set_pos(demoButton,width-248,height-44);lv_obj_set_size(demoButton,232,40);
    visible(demoButton,true);
}

static void hardwareDemoBanner(){
    preferences::Config identity;preferences::get(identity);
    lv_label_set_text_fmt(brand,"%s · LIVE",identity.name);
    if(!shown.demo){
        if(shown.hasUpdate&&!shown.apiAvailable){
            lv_label_set_text_fmt(brand,"%s · LIVE · VERALTETE WERTE",identity.name);
            badge(health,panel::Status::Unknown,"Unbekannt");
            const auto age=shown.sourceAgeSeconds+uint32_t(millis()-shown.receivedAtMs)/1000;
            lv_label_set_text_fmt(connection,"VERALTET · Letzte Daten vor %lu s · Aktueller Zustand unbekannt · Verbindung prüfen",(unsigned long)age);
            lv_obj_set_style_text_color(connection,lv_color_hex(0xFFD166),0);
            lv_obj_set_style_bg_color(connection,lv_color_hex(0x493500),0);lv_obj_set_style_bg_opa(connection,LV_OPA_COVER,0);
        }
        return;
    }
    lv_label_set_text_fmt(brand,"%s · DEMO · FIKTIVE DATEN",identity.name);
    badge(health,panel::Status::Warning,"DEMO");
    lv_label_set_text(connection,"DEMO: Keine Live-Überwachung · Fiktive Daten · Verbindungsstatus in Einstellungen");
    lv_obj_set_style_text_color(connection,lv_color_hex(0xFFD166),0);
    lv_obj_set_style_bg_color(connection,lv_color_hex(0x493500),0);
    lv_obj_set_style_bg_opa(connection,LV_OPA_COVER,0);
}
void uiUpdate(const panel::Snapshot& value){
    sharedUiUpdate(value);
    lv_obj_set_style_bg_opa(connection,LV_OPA_TRANSP,0);
    hardwareDemoBanner();
}
void uiLoop(uint32_t now){sharedUiLoop(now);hardwareDemoBanner();}
bool hardwareDemoActive(){return shown.demo;}
