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
static void hardwareCategories(){
    // Before the first valid snapshot the configuration is still unknown.
    auto active=[](unsigned i){return i==panel::CategoryCount||!shown.hasUpdate||shown.categories[i].enabled;};
    if(selected>=0&&selected<int(panel::CategoryCount)&&!active(selected))navigate(-1);
    unsigned count=0;for(unsigned i=0;i<=panel::CategoryCount;++i)if(active(i))++count;
    const int margin=wide?16:8,gap=wide?10:6,columns=wide?int(count):3;
    const int tileWidth=(width-2*margin-(columns-1)*gap)/columns;
    unsigned slot=0;
    for(unsigned i=0;i<=panel::CategoryCount;++i){
        visible(tiles[i],active(i)&&(wide||selected<0));
        if(!active(i))continue;
        lv_obj_set_pos(tiles[i],margin+(slot%columns)*(tileWidth+gap),(wide?74:54)+(slot/columns)*64);
        lv_obj_set_width(tiles[i],tileWidth);
        for(auto* text:{tileNames[i],tileStates[i],tileCounts[i]})lv_obj_set_width(text,tileWidth-4);
        ++slot;
    }
}
#include "display_features.inc"
void hardwareSettingsAttach() {
    displayfeatures::begin();
    lv_label_set_long_mode(brand,LV_LABEL_LONG_DOT);
    lv_obj_set_height(brand,26);
    advance=settingsOpen;
    lv_label_set_text(lv_obj_get_child(demoButton,0),"Einstellungen");
    lv_obj_set_pos(demoButton,width-248,height-44);lv_obj_set_size(demoButton,232,40);
    visible(demoButton,true);
    // Shared navigation restores all tiles; apply hardware filtering in the same event.
    for(auto* tile:tiles)lv_obj_add_event_cb(tile,[](lv_event_t*){hardwareCategories();},LV_EVENT_CLICKED,nullptr);
    lv_obj_add_event_cb(backButton,[](lv_event_t*){hardwareCategories();},LV_EVENT_CLICKED,nullptr);
    hardwareCategories();
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
    preserveEntityOrder=true;
    uniformSystemCardHeights=true;
    sharedUiUpdate(value);
    hardwareCategories();
    lv_obj_set_style_bg_opa(connection,LV_OPA_TRANSP,0);
    hardwareDemoBanner();
}
void uiLoop(uint32_t now){sharedUiLoop(now);hardwareDemoBanner();displayfeatures::tick(now);}
bool hardwareDemoActive(){return shown.demo;}
