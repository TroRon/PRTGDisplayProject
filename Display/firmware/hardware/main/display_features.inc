// Hardware-only optional presentation features; bounded RAM history, no periodic flash writes. Accessed exclusively under the LVGL lock.
#include <ctime>
#include <cmath>
#include <new>
#ifdef DISPLAY_FEATURES_NATIVE_TEST
#include "../test/display_features_stubs.h"
#else
#include "nvs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#endif
#include "ota.h"
namespace displayfeatures {
constexpr unsigned Samples=16;
struct Options {
 uint32_t version=1,rotateSeconds=20,nightStart=22,nightEnd=7,brightness=25;
 int32_t utcOffsetMinutes=60;
 bool rotate=false,night=false,interrupt=false,history=false,favoriteHome=false;
 char favorites[panel::MaxEntities][64]={};
 char origin[256]={};
};
struct Point { uint32_t at=0;float value=0; };
struct Track { char id[64]={},key[128]={};uint32_t count=0,gaps=0;bool pendingGap=false;Point points[Samples]; };
struct History { uint32_t version=1;char origin[256]={};Track tracks[panel::MaxEntities]; };
static Options options,draft;
static History* history=nullptr;
static bool favoritesOnly=false;
static lv_obj_t *dialog=nullptr,*optionNotice=nullptr,*favoriteButton=nullptr;
static lv_obj_t *rotateCheck,*nightCheck,*alertCheck,*historyCheck,*homeCheck,*interval,*startHour,*endHour,*level,*zone;
static lv_obj_t* favoriteChecks[panel::MaxEntities]={};
static char favoriteIds[panel::MaxEntities][64]={};
static unsigned favoriteCount=0;
static uint32_t lastRotation=0,lastTick=0,lastInterrupt=0,lastAge=0;
static char criticalIds[panel::MaxEntities][64]={};static unsigned criticalCount=0;
static bool criticalSeeded=false,started=false;
static char pendingCritical[64]={};
static bool valid(const Options& c){
 if(!memchr(c.origin,0,sizeof(c.origin)))return false;
 if(c.version!=1||c.rotateSeconds<10||c.rotateSeconds>60||c.nightStart>23||c.nightEnd>23||c.brightness<10||c.brightness>100||c.utcOffsetMinutes< -720||c.utcOffsetMinutes>840)return false;
 for(const auto& id:c.favorites)if(!memchr(id,0,sizeof(id)))return false;
 return true;
}
static bool favorite(const char* id){if(!*id)return false;for(const auto& f:options.favorites)if(!strcmp(id,f))return true;return false;}
static bool filter(const panel::Entity& e){preferences::Config source;preferences::get(source);return shown.demo||strcmp(source.origin,options.origin)||selected>=0||!favoritesOnly||favorite(e.id);}
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
static Track* track(const panel::Entity& e,bool create){
 if(!history||!*e.id)return nullptr;
 for(auto& t:history->tracks)if(!strcmp(t.id,e.id))return &t;
 if(create)for(auto& t:history->tracks)if(!*t.id){snprintf(t.id,sizeof(t.id),"%s",e.id);return &t;}
 return nullptr;
}
static void graph(lv_event_t* event){
 if(dialog)return;
 const unsigned i=(uintptr_t)lv_event_get_user_data(event);if(i>=shown.entityCount)return;
 const auto& e=shown.entities[i];window(e.name);
 caption(dialog,"Verlauf echter Messwerte · bis zu 16 Beobachtungen · Rohwerte wie vom Aggregator geliefert");
 if(shown.demo){caption(dialog,"DEMO: Kein echter Verlauf. Demo-Werte werden nicht gespeichert.");return;}
 auto* t=track(e,false);
 if(!options.history||!t||t->count<2){caption(dialog,"Noch kein Verlauf. Unter Anzeige & Favoriten aktivieren; mindestens zwei neue Messungen abwarten.");return;}
 caption(dialog,t->key);
 auto* chart=lv_chart_create(dialog);lv_obj_set_size(chart,940,240);lv_chart_set_type(chart,LV_CHART_TYPE_LINE);
 lv_chart_set_point_count(chart,t->count*2-1);auto* series=lv_chart_add_series(chart,lv_color_hex(0x57C9A4),LV_CHART_AXIS_PRIMARY_Y);
 float low=t->points[0].value,high=low;
 for(unsigned n=0;n<t->count;n++){if(t->points[n].value<low)low=t->points[n].value;if(t->points[n].value>high)high=t->points[n].value;}
 // Normalize only the drawing coordinates. Actual min/max remain explicitly labelled.
 const double span=high>low?double(high)-low:1;
 caption(dialog,"Momentaufnahme beim Öffnen · aktuelle Zustände stehen auf der Systemkarte");
 lv_chart_set_range(chart,LV_CHART_AXIS_PRIMARY_Y,0,1000);
 for(unsigned n=0;n<t->count;n++){
  lv_chart_set_value_by_id(chart,series,n*2,(lv_coord_t)((double(t->points[n].value)-low)*1000/span));
  if(n+1<t->count)lv_chart_set_value_by_id(chart,series,n*2+1,(t->gaps&(1u<<(n+1)))||t->points[n+1].at-t->points[n].at>180?LV_CHART_POINT_NONE:(lv_coord_t)(((double(t->points[n].value)+t->points[n+1].value)/2-low)*1000/span));
 }
 char line[240];snprintf(line,sizeof(line),"Minimum %.2f · Maximum %.2f · Letzter Wert %.2f\nX: Beobachtungen in Reihenfolge, kein gleichmässiges Zeitraster. Lücken über 3 Minuten werden unterbrochen.",low,high,t->points[t->count-1].value);caption(dialog,line);
 time_t first=t->points[0].at,last=t->points[t->count-1].at;struct tm a,b;gmtime_r(&first,&a);gmtime_r(&last,&b);char from[32],to[32];strftime(from,sizeof(from),"%d.%m. %H:%M UTC",&a);strftime(to,sizeof(to),"%d.%m. %H:%M UTC",&b);snprintf(line,sizeof(line),"%s bis %s · %s",from,to,"Nur im RAM · Neustart löscht den Verlauf");caption(dialog,line);
}
static void decorate(lv_obj_t* card,const panel::Entity& e){
 char age[100];const int64_t now=time(nullptr);
 if(shown.demo)snprintf(age,sizeof(age),"DEMO · Datenalter nicht anwendbar");
 else if(e.observedAt<=0||now<1700000000||e.observedAt>now+60)snprintf(age,sizeof(age),"Messzeit unbekannt");
 else {const auto seconds=now>e.observedAt?now-e.observedAt:0;snprintf(age,sizeof(age),"Messung vor %lld %s%s",(long long)(seconds<60?seconds:seconds<3600?seconds/60:seconds/3600),seconds<60?"s":seconds<3600?"min":"h",shown.apiAvailable?"":" · Verbindung fehlt");}
 auto* l=caption(card,age);lv_obj_set_style_text_color(l,lv_color_hex(0x9EB7C9),0);
 if(options.history){auto* button=action(card,"Verlauf",graph);lv_obj_set_width(button,LV_PCT(100));lv_obj_remove_event_cb(button,graph);lv_obj_add_event_cb(button,graph,LV_EVENT_CLICKED,(void*)(uintptr_t)(&e-shown.entities));}
}
static void historyOrigin(){
 if(!history)return;
 preferences::Config source;preferences::get(source);
 if(strcmp(history->origin,source.origin)){memset(history,0,sizeof(*history));history->version=1;snprintf(history->origin,sizeof(history->origin),"%s",source.origin);}
}
static void observe(const panel::Snapshot& value){
 static char lastOrigin[256]={};preferences::Config source;preferences::get(source);
 if(strcmp(lastOrigin,source.origin)){snprintf(lastOrigin,sizeof(lastOrigin),"%s",source.origin);criticalSeeded=false;criticalCount=0;*pendingCritical=0;favoritesOnly=options.favoriteHome&&!strcmp(options.origin,source.origin);}
 historyOrigin();
 if(value.demo||!value.apiAvailable){if(history)for(auto& t:history->tracks)t.pendingGap=true;*pendingCritical=0;criticalSeeded=false;criticalCount=0;return;}
 if(history&&options.history){
  // Removed/disabled systems must not occupy the finite history slots forever.
  for(auto& t:history->tracks)if(*t.id){bool found=false;for(unsigned i=0;i<value.entityCount;i++)if(!strcmp(t.id,value.entities[i].id))found=true;if(!found){t=Track{};}}
  const int64_t now=time(nullptr);
  for(unsigned i=0;i<value.entityCount;i++){
   const auto& e=value.entities[i];if(!e.trendValid||!std::isfinite(e.trendValue)||e.trendObservedAt<=0||e.trendObservedAt>now||now-e.trendObservedAt>180||e.status==panel::Status::Unknown){auto* missing=track(e,false);if(missing)missing->pendingGap=true;continue;}
   auto* t=track(e,true);if(!t)continue;
   if(strcmp(t->key,e.trendKey)){*t=Track{};snprintf(t->id,sizeof(t->id),"%s",e.id);snprintf(t->key,sizeof(t->key),"%s",e.trendKey);}
   if(t->count&&e.trendObservedAt<=t->points[t->count-1].at)continue;
   if(t->count==Samples){memmove(t->points,t->points+1,(Samples-1)*sizeof(Point));t->gaps>>=1;--t->count;}
   if(t->pendingGap)t->gaps|=1u<<t->count;
   t->pendingGap=false;
   t->points[t->count].at=e.trendObservedAt;t->points[t->count].value=e.trendValue;++t->count;
  }
 }
 char newCritical[64]={};
 for(unsigned i=0;i<value.entityCount;i++)if(value.entities[i].status==panel::Status::Critical&&*value.entities[i].id){bool known=false;for(unsigned j=0;j<criticalCount;j++)if(!strcmp(criticalIds[j],value.entities[i].id))known=true;if(!known&&!*newCritical)snprintf(newCritical,sizeof(newCritical),"%s",value.entities[i].id);}
 criticalCount=0;for(unsigned i=0;i<value.entityCount;i++)if(value.entities[i].status==panel::Status::Critical&&*value.entities[i].id)snprintf(criticalIds[criticalCount++],64,"%s",value.entities[i].id);
 if(criticalSeeded&&*newCritical&&options.interrupt)snprintf(pendingCritical,sizeof(pendingCritical),"%s",newCritical);
 bool stillCritical=false;for(unsigned i=0;i<value.entityCount;i++)if(!strcmp(pendingCritical,value.entities[i].id)&&value.entities[i].status==panel::Status::Critical)stillCritical=true;
 if(!stillCritical)*pendingCritical=0;
 criticalSeeded=true;
}
static lv_obj_t* check(lv_obj_t* p,const char* name,bool on){auto* c=lv_checkbox_create(p);lv_checkbox_set_text(c,name);if(on)lv_obj_add_state(c,LV_STATE_CHECKED);return c;}
static lv_obj_t* number(lv_obj_t* p,const char* name,int value,int lo,int hi){
 caption(p,name);auto* row=lv_obj_create(p);lv_obj_set_size(row,940,64);lv_obj_set_style_pad_all(row,8,0);lv_obj_clear_flag(row,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(row,lv_color_hex(0x1B3248),0);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);
 auto* n=lv_spinbox_create(row);lv_obj_set_size(n,160,44);lv_spinbox_set_range(n,lo,hi);lv_spinbox_set_digit_format(n,hi<=23?2:hi<=100?3:4,0);lv_spinbox_set_value(n,value);lv_spinbox_set_step(n,1);
 auto* minus=action(row,"-",nullptr);lv_obj_set_width(minus,90);lv_obj_add_event_cb(minus,[](lv_event_t* e){auto* n=(lv_obj_t*)lv_event_get_user_data(e);lv_spinbox_set_step(n,n==zone?15:1);lv_spinbox_decrement(n);},LV_EVENT_CLICKED,n);
 auto* plus=action(row,"+",nullptr);lv_obj_set_width(plus,90);lv_obj_add_event_cb(plus,[](lv_event_t* e){auto* n=(lv_obj_t*)lv_event_get_user_data(e);lv_spinbox_set_step(n,n==zone?15:1);lv_spinbox_increment(n);},LV_EVENT_CLICKED,n);return n;
}
static bool ensureHistory(){
 if(history)return true;
 void* memory=heap_caps_malloc(sizeof(History),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
 if(!memory){ESP_LOGW("DISPLAY","History allocation failed; live monitoring remains active");return false;}
 history=new(memory) History;return true;
}
static void save(lv_event_t*){
 draft.rotate=lv_obj_has_state(rotateCheck,LV_STATE_CHECKED);draft.night=lv_obj_has_state(nightCheck,LV_STATE_CHECKED);draft.interrupt=lv_obj_has_state(alertCheck,LV_STATE_CHECKED);draft.history=lv_obj_has_state(historyCheck,LV_STATE_CHECKED);draft.favoriteHome=lv_obj_has_state(homeCheck,LV_STATE_CHECKED);
 draft.rotateSeconds=lv_spinbox_get_value(interval);draft.nightStart=lv_spinbox_get_value(startHour);draft.nightEnd=lv_spinbox_get_value(endHour);draft.brightness=lv_spinbox_get_value(level);draft.utcOffsetMinutes=lv_spinbox_get_value(zone);
 for(unsigned i=0;i<favoriteCount;i++){bool on=lv_obj_has_state(favoriteChecks[i],LV_STATE_CHECKED);for(auto& id:draft.favorites)if(!strcmp(id,favoriteIds[i]))*id=0;if(on)for(auto& id:draft.favorites)if(!*id){snprintf(id,sizeof(id),"%s",favoriteIds[i]);break;}}
 if(!valid(draft)||ota::busy()){lv_label_set_text(optionNotice,"Nicht gespeichert: Werte prüfen / OTA abwarten.");return;}
 if(draft.history&&!ensureHistory()){lv_label_set_text(optionNotice,"Verlauf nicht verfügbar: PSRAM fehlt. Live-Anzeige bleibt aktiv.");return;}
 nvs_handle_t n;if(nvs_open("eagle-view",NVS_READWRITE,&n)!=ESP_OK){if(!options.history&&history){heap_caps_free(history);history=nullptr;}lv_label_set_text(optionNotice,"Speicher nicht verfügbar. Bisherige Einstellungen bleiben erhalten.");return;}
 bool ok=nvs_set_blob(n,"options",&draft,sizeof(draft))==ESP_OK&&nvs_commit(n)==ESP_OK;nvs_close(n);
 if(ok){options=draft;if(!options.history&&history){history->~History();heap_caps_free(history);history=nullptr;}if(!options.interrupt)*pendingCritical=0;favoritesOnly=options.favoriteHome;renderCards(false);resetDimming();lv_label_set_text(optionNotice,"Gespeichert. Nachtzeiten verwenden den eingestellten UTC-Versatz (keine automatische Sommerzeit).");}else {if(!options.history&&history){heap_caps_free(history);history=nullptr;}lv_label_set_text(optionNotice,"Speichern fehlgeschlagen. Bisherige Einstellungen bleiben erhalten.");}
}
static void open(){
 if(dialog)return;
 draft=options;preferences::Config source;preferences::get(source);if(strcmp(draft.origin,source.origin)){memset(draft.favorites,0,sizeof(draft.favorites));draft.favoriteHome=false;}snprintf(draft.origin,sizeof(draft.origin),"%s",source.origin);window("Anzeige & Favoriten");
 rotateCheck=check(dialog,"Automatischer Seitenwechsel (Berührung pausiert 60 Sekunden)",draft.rotate);interval=number(dialog,"Seitenwechsel in Sekunden",draft.rotateSeconds,10,60);
 alertCheck=check(dialog,"Bei neuer kritischer Störung zur betroffenen Rubrik wechseln",draft.interrupt);
 nightCheck=check(dialog,"Zeitgesteuerter Nachtmodus",draft.night);startHour=number(dialog,"Nacht ab Stunde",draft.nightStart,0,23);endHour=number(dialog,"Nacht bis Stunde (gleiche Stunden = ganztägig)",draft.nightEnd,0,23);level=number(dialog,"Sichtbare Helligkeit in Prozent (softwareseitige Abdunklung)",draft.brightness,10,100);zone=number(dialog,"UTC-Versatz in Minuten (15er-Schritte), z. B. 60 Winter / 120 Sommer",draft.utcOffsetMinutes,-720,840);
 caption(dialog,"Berühren hellt 60 Sekunden auf. Ohne synchronisierte Uhr bleibt die Anzeige hell. Keine automatische Sommerzeitumstellung.");
 historyCheck=check(dialog,"Echte Messwertverläufe sammeln (erster verfügbarer Messwert pro System)",draft.history);
 caption(dialog,"Bis 16 echte Messzeitpunkte je System, nur im RAM. Neustart oder Deaktivieren löscht den Verlauf. Langzeitdaten bleiben in PRTG.");

 homeCheck=check(dialog,"Favoriten als Startübersicht",draft.favoriteHome);caption(dialog,"Favoriten: stabile System-ID; Namen können im Aggregator geändert werden.");favoriteCount=0;
 for(unsigned i=0;i<shown.entityCount;i++)if(*shown.entities[i].id&&!shown.demo){snprintf(favoriteIds[favoriteCount],64,"%s",shown.entities[i].id);favoriteChecks[favoriteCount]=check(dialog,shown.entities[i].name,!strcmp(options.origin,source.origin)&&favorite(shown.entities[i].id));++favoriteCount;}
 if(!favoriteCount)caption(dialog,"Für die Favoritenauswahl zuerst echte Live-Daten empfangen.");
 optionNotice=caption(dialog,"");action(dialog,"Anzeige speichern",save);
}
static void begin(){
 hardwareIdleControl=true;entityFilter=filter;decorateEntity=decorate;
 emptyEntityMessage=[]()->const char*{preferences::Config source;preferences::get(source);return selected<0&&favoritesOnly&&!shown.demo&&!strcmp(options.origin,source.origin)?"Keine aktiven Favoriten. Unter Einstellungen / Panel / Anzeige auswählen oder Favoriten / Alle antippen.":nullptr;};
 nvs_handle_t n;if(nvs_open("eagle-view",NVS_READONLY,&n)==ESP_OK){
 Options c;size_t size=sizeof(c);
 if(nvs_get_blob(n,"options",&c,&size)==ESP_OK&&size==sizeof(c)&&valid(c))options=c;
 else {
  struct Basic {uint32_t version,rotateSeconds,nightStart,nightEnd,brightness;int32_t utcOffsetMinutes;bool rotate,night;} old{};
  size=sizeof(old);if(nvs_get_blob(n,"basic",&old,&size)==ESP_OK&&size==sizeof(old)&&old.version==1){
   c=Options{};c.rotateSeconds=old.rotateSeconds;c.nightStart=old.nightStart;c.nightEnd=old.nightEnd;c.brightness=old.brightness;c.utcOffsetMinutes=old.utcOffsetMinutes;c.rotate=old.rotate;c.night=old.night;if(valid(c))options=c;
  }
 }
 nvs_close(n);}
 if(options.history&&!ensureHistory())options.history=false;
 historyOrigin();favoritesOnly=options.favoriteHome;
 if(favoriteButton)return;
 favoriteButton=lv_btn_create(lv_scr_act());lv_obj_set_pos(favoriteButton,16,height-44);lv_obj_set_size(favoriteButton,200,40);auto* l=lv_label_create(favoriteButton);lv_label_set_text(l,"Favoriten / Alle");lv_obj_center(l);
 lv_obj_add_event_cb(favoriteButton,[](lv_event_t*){favoritesOnly=!favoritesOnly;navigate(-1);hardwareCategories();},LV_EVENT_CLICKED,nullptr);
 lv_obj_set_pos(footer,230,height-30);lv_obj_set_width(footer,520);
}
static void tick(uint32_t now){
 if(uint32_t(now-lastTick)<1000)return;
 lastTick=now;
 ota::Status otaState;ota::status(otaState);const bool busy=settingsIsOpen()||dialog||otaState.busy||otaState.pending;
 const uint32_t idle=lv_disp_get_inactive_time(nullptr);
 if(!started){lastRotation=now;started=true;}
 if(busy||idle<60000)lastRotation=now;
 if(options.interrupt&&*pendingCritical&&!busy&&!shown.demo&&shown.apiAvailable&&idle>=60000&&uint32_t(now-lastInterrupt)>=60000){
  for(unsigned i=0;i<shown.entityCount;i++)if(!strcmp(pendingCritical,shown.entities[i].id)&&shown.entities[i].status==panel::Status::Critical){favoritesOnly=false;navigate(shown.entities[i].category);hardwareCategories();lastInterrupt=lastRotation=now;break;}
  *pendingCritical=0;
 }
 if(options.rotate&&!busy&&idle>=60000&&uint32_t(now-lastRotation)>=options.rotateSeconds*1000){
  favoritesOnly=false;int next=selected;for(unsigned i=0;i<panel::CategoryCount+1;i++){next=(next+2)%(panel::CategoryCount+1)-1;if(next<0||shown.categories[next].enabled)break;}favoritesOnly=next<0&&options.favoriteHome;navigate(next);hardwareCategories();lastRotation=now;
 }
 const int64_t utc=time(nullptr);bool night=false;
 if(options.night&&utc>=1700000000){const int hour=((utc/60+options.utcOffsetMinutes)%1440+1440)%1440/60;night=options.nightStart==options.nightEnd||(options.nightStart<options.nightEnd?(hour>=int(options.nightStart)&&hour<int(options.nightEnd)):(hour>=int(options.nightStart)||hour<int(options.nightEnd)));}
 const bool dark=night&&!busy&&idle>=60000;
 if(dark!=dimmed){dimmed=dark;lv_obj_set_style_bg_opa(shade,(lv_opa_t)(255*(100-options.brightness)/100),0);visible(shade,dark);}
 if(!busy&&idle>=2000&&uint32_t(now-lastAge)>=30000){lastAge=now;renderCards(false);}
 preferences::Config source;preferences::get(source);if(selected<0)lv_label_set_text(pageTitle,favoritesOnly&&!shown.demo&&!strcmp(options.origin,source.origin)?"Favoriten":"Übersicht");
}
}
void displayOptionsOpen(){displayfeatures::open();}
