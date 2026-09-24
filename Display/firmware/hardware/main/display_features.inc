// Hardware-only essentials. Deferred experiments are not compiled or allocated.
#include <ctime>
#ifdef DISPLAY_FEATURES_NATIVE_TEST
#include "../test/display_features_stubs.h"
#else
#include "nvs.h"
#endif
#include "ota.h"
namespace displayfeatures {
struct Options {
 uint32_t version=1,rotateSeconds=20,nightStart=22,nightEnd=7,brightness=25;
 int32_t utcOffsetMinutes=60;
 bool rotate=false,night=false;
};
static Options options,draft;
static lv_obj_t *dialog=nullptr,*optionNotice=nullptr;
static lv_obj_t *rotateCheck,*nightCheck,*interval,*startHour,*endHour,*level,*zone;
static uint32_t lastRotation=0,lastTick=0,lastAge=0;
static bool started=false;
static bool valid(const Options& c){
 return c.version==1&&c.rotateSeconds>=10&&c.rotateSeconds<=60&&c.nightStart<=23&&c.nightEnd<=23&&c.brightness>=10&&c.brightness<=100&&c.utcOffsetMinutes>=-720&&c.utcOffsetMinutes<=840;
}
static void resetDimming(){dimmed=false;visible(shade,false);}
static lv_obj_t* caption(lv_obj_t* parent,const char* text){auto* l=lv_label_create(parent);lv_label_set_text(l,text);lv_obj_set_width(l,LV_PCT(100));return l;}
static lv_obj_t* action(lv_obj_t* parent,const char* text,lv_event_cb_t cb){auto* b=lv_btn_create(parent);lv_obj_set_size(b,280,44);auto* l=lv_label_create(b);lv_label_set_text(l,text);lv_obj_center(l);if(cb)lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);return b;}
static void close(lv_event_t*){if(dialog){lv_obj_del(dialog);dialog=nullptr;}lastRotation=millis();lv_disp_trig_activity(nullptr);}
static void window(const char* title){
 dialog=lv_obj_create(lv_layer_top());lv_obj_set_size(dialog,1024,600);lv_obj_set_pos(dialog,0,0);
 lv_obj_set_style_bg_color(dialog,lv_color_hex(0x102333),0);lv_obj_set_style_text_color(dialog,lv_color_hex(0xF1F5F9),0);lv_obj_set_style_text_font(dialog,&panel_font_14,0);
 lv_obj_set_flex_flow(dialog,LV_FLEX_FLOW_COLUMN);lv_obj_set_style_pad_all(dialog,20,0);lv_obj_set_style_pad_row(dialog,14,0);
 caption(dialog,title);action(dialog,"Schliessen / verwerfen",close);resetDimming();
}
static void decorate(lv_obj_t* card,const panel::Entity& e){
 char age[100];const int64_t now=time(nullptr);
 if(shown.demo)snprintf(age,sizeof(age),"DEMO · Datenalter nicht anwendbar");
 else if(e.observedAt<=0||now<1700000000||e.observedAt>now+60)snprintf(age,sizeof(age),"Messzeit unbekannt");
 else {const auto seconds=now>e.observedAt?now-e.observedAt:0;snprintf(age,sizeof(age),"Messung vor %lld %s%s",(long long)(seconds<60?seconds:seconds<3600?seconds/60:seconds/3600),seconds<60?"s":seconds<3600?"min":"h",shown.apiAvailable?"":" · Verbindung fehlt");}
 auto* l=caption(card,age);lv_obj_set_style_text_color(l,lv_color_hex(0x9EB7C9),0);
}
static lv_obj_t* check(lv_obj_t* p,const char* name,bool on){auto* c=lv_checkbox_create(p);lv_checkbox_set_text(c,name);if(on)lv_obj_add_state(c,LV_STATE_CHECKED);return c;}
static lv_obj_t* number(lv_obj_t* p,const char* name,int value,int lo,int hi){
 caption(p,name);auto* row=lv_obj_create(p);lv_obj_set_size(row,940,64);lv_obj_set_style_pad_all(row,8,0);lv_obj_clear_flag(row,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(row,lv_color_hex(0x1B3248),0);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);
 auto* n=lv_spinbox_create(row);lv_obj_set_size(n,160,44);lv_spinbox_set_range(n,lo,hi);lv_spinbox_set_digit_format(n,hi<=23?2:hi<=100?3:4,0);lv_spinbox_set_value(n,value);lv_spinbox_set_step(n,1);
 auto* minus=action(row,"-",nullptr);lv_obj_set_width(minus,90);lv_obj_add_event_cb(minus,[](lv_event_t* e){auto* n=(lv_obj_t*)lv_event_get_user_data(e);lv_spinbox_set_step(n,n==zone?15:1);lv_spinbox_decrement(n);},LV_EVENT_CLICKED,n);
 auto* plus=action(row,"+",nullptr);lv_obj_set_width(plus,90);lv_obj_add_event_cb(plus,[](lv_event_t* e){auto* n=(lv_obj_t*)lv_event_get_user_data(e);lv_spinbox_set_step(n,n==zone?15:1);lv_spinbox_increment(n);},LV_EVENT_CLICKED,n);return n;
}
static void save(lv_event_t*){
 draft.rotate=lv_obj_has_state(rotateCheck,LV_STATE_CHECKED);draft.night=lv_obj_has_state(nightCheck,LV_STATE_CHECKED);
 draft.rotateSeconds=lv_spinbox_get_value(interval);draft.nightStart=lv_spinbox_get_value(startHour);draft.nightEnd=lv_spinbox_get_value(endHour);draft.brightness=lv_spinbox_get_value(level);draft.utcOffsetMinutes=lv_spinbox_get_value(zone);
 if(!valid(draft)||ota::busy()){lv_label_set_text(optionNotice,"Nicht gespeichert: Werte prüfen / OTA abwarten.");return;}
 nvs_handle_t n;if(nvs_open("eagle-view",NVS_READWRITE,&n)!=ESP_OK){lv_label_set_text(optionNotice,"Speicher nicht verfügbar. Bisherige Einstellungen bleiben erhalten.");return;}
 bool ok=nvs_set_blob(n,"basic",&draft,sizeof(draft))==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);
 if(ok){options=draft;resetDimming();lv_label_set_text(optionNotice,"Gespeichert. Nachtzeiten verwenden den UTC-Versatz (Sommerzeit manuell einstellen).");}else lv_label_set_text(optionNotice,"Speichern fehlgeschlagen. Bisherige Einstellungen bleiben erhalten.");
}
static void open(){
 if(dialog)return;
 draft=options;window("Anzeige");
 rotateCheck=check(dialog,"Automatischer Seitenwechsel (Berührung pausiert 60 Sekunden)",draft.rotate);interval=number(dialog,"Seitenwechsel in Sekunden",draft.rotateSeconds,10,60);
 nightCheck=check(dialog,"Zeitgesteuerter Nachtmodus",draft.night);startHour=number(dialog,"Nacht ab Stunde",draft.nightStart,0,23);endHour=number(dialog,"Nacht bis Stunde (gleiche Stunden = ganztägig)",draft.nightEnd,0,23);level=number(dialog,"Sichtbare Helligkeit in Prozent (softwareseitige Abdunklung)",draft.brightness,10,100);zone=number(dialog,"UTC-Versatz in Minuten (15er-Schritte), z. B. 60 Winter / 120 Sommer",draft.utcOffsetMinutes,-720,840);
 caption(dialog,"Berühren hellt 60 Sekunden auf. Ohne synchronisierte Uhr bleibt die Anzeige hell. Keine automatische Sommerzeitumstellung.");
 optionNotice=caption(dialog,"");action(dialog,"Anzeige speichern",save);
}
static void begin(){
 hardwareIdleControl=true;decorateEntity=decorate;
 nvs_handle_t n;if(nvs_open("eagle-view",NVS_READONLY,&n)==ESP_OK){Options c;size_t size=sizeof(c);if(nvs_get_blob(n,"basic",&c,&size)==ESP_OK&&size==sizeof(c)&&valid(c))options=c;nvs_close(n);}
}
static void tick(uint32_t now){
 if(uint32_t(now-lastTick)<1000)return;
 lastTick=now;
 ota::Status otaState;ota::status(otaState);const bool busy=settingsIsOpen()||dialog||otaState.busy||otaState.pending;
 const uint32_t idle=lv_disp_get_inactive_time(nullptr);
 if(!started){lastRotation=now;started=true;}
 if(busy||idle<60000)lastRotation=now;
 if(options.rotate&&!busy&&idle>=60000&&uint32_t(now-lastRotation)>=options.rotateSeconds*1000){
  int next=selected;for(unsigned i=0;i<panel::CategoryCount+1;i++){next=(next+2)%(panel::CategoryCount+1)-1;if(next<0||shown.categories[next].enabled)break;}navigate(next);hardwareCategories();lastRotation=now;
 }
 const int64_t utc=time(nullptr);bool night=false;
 if(options.night&&utc>=1700000000){const int hour=((utc/60+options.utcOffsetMinutes)%1440+1440)%1440/60;night=options.nightStart==options.nightEnd||(options.nightStart<options.nightEnd?(hour>=int(options.nightStart)&&hour<int(options.nightEnd)):(hour>=int(options.nightStart)||hour<int(options.nightEnd)));}
 const bool dark=night&&!busy&&idle>=60000;
 if(dark!=dimmed){dimmed=dark;lv_obj_set_style_bg_opa(shade,(lv_opa_t)(255*(100-options.brightness)/100),0);visible(shade,dark);}
 if(!busy&&idle>=2000&&uint32_t(now-lastAge)>=30000){lastAge=now;renderCards(false);}
}
}
void displayOptionsOpen(){displayfeatures::open();}
