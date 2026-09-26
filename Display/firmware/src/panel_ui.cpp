#include "panel_ui.h"
#include "board_port.h"
#include <lvgl.h>
#include <Arduino.h>
#include <string.h>
LV_FONT_DECLARE(panel_font_14);
LV_FONT_DECLARE(panel_font_20);
static panel::Snapshot shown;
static lv_obj_t *brand, *health, *connection, *footer, *tiles[6], *tileNames[6], *tileStates[6], *tileCounts[6];
static lv_obj_t *section, *pageTitle, *backButton, *detailList, *scrollHint, *demoButton, *shade;
static int selected=-1, width=320, height=240;
static bool wide=false, dimmed=false;
static bool hardwareIdleControl=false;
static const char* (*emptyEntityMessage)()=nullptr;
static bool (*entityFilter)(const panel::Entity&)=nullptr;
static void (*decorateEntity)(lv_obj_t*,const panel::Entity&)=nullptr;
static uint32_t activity=0, lastClock=0;
static void (*advance)()=nullptr;
static lv_color_t color(panel::Status s) {
  return lv_color_hex(s==panel::Status::Ok ? 0x20C878 : s==panel::Status::Warning ? 0xFFB020 : s==panel::Status::Critical ? 0xF04455 : 0x9AAFC3);
}
static void visible(lv_obj_t* object,bool show) {
  if(show)lv_obj_clear_flag(object,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(object,LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t* box(lv_obj_t* parent,int x,int y,int w,int h) {
  auto* object=lv_obj_create(parent); lv_obj_set_pos(object,x,y); lv_obj_set_size(object,w,h);
  lv_obj_set_style_pad_all(object,0,0); lv_obj_set_style_border_width(object,0,0);
  lv_obj_set_style_bg_opa(object,LV_OPA_TRANSP,0); lv_obj_clear_flag(object,LV_OBJ_FLAG_SCROLLABLE);
  return object;
}
static lv_obj_t* label(lv_obj_t* parent,const char* text,int x,int y,int w,const lv_font_t* font=&panel_font_14) {
  auto* object=lv_label_create(parent); lv_obj_set_pos(object,x,y);lv_obj_set_width(object,w);
  lv_obj_set_style_text_color(object,lv_color_hex(0xF1F5F9),0); lv_obj_set_style_text_font(object,font,0); lv_label_set_text(object,text); return object;
}
static void badge(lv_obj_t* object,panel::Status status,const char* text) {
  lv_label_set_text(object,text);lv_obj_set_style_text_color(object,color(status),0);
  lv_obj_set_style_bg_color(object,color(status),0);lv_obj_set_style_bg_opa(object,LV_OPA_10,0);
  lv_obj_set_style_radius(object,5,0);lv_obj_set_style_pad_ver(object,3,0);
  lv_obj_set_style_text_align(object,LV_TEXT_ALIGN_CENTER,0);
}
static void wake() {
  activity=millis();
  if(dimmed){dimmed=false;visible(shade,false);boardDim(false);}
}
static void readTouch(lv_indev_drv_t*,lv_indev_data_t* data) {
  static bool wakeOnly=false;int16_t x=0,y=0;bool pressed;
  boardTouch(x,y,pressed);
  if(!pressed)wakeOnly=false;
  if(pressed&&dimmed){wake();wakeOnly=true;}
  if(wakeOnly){data->state=LV_INDEV_STATE_RELEASED;return;}
  if(pressed)wake();
  data->point.x=x;data->point.y=y;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
}
static void updateConnection(uint32_t now) {
  const char* link=shown.demo ? (shown.apiAvailable?"Testdaten":"Demo-Fehler") : (shown.apiAvailable?"API OK":"API-Fehler");
  if(!shown.hasUpdate){lv_label_set_text(connection,shown.demo?"Testdaten werden geladen":"Warte auf erste API-Daten");return;}
  uint32_t age=shown.sourceAgeSeconds+uint32_t(now-shown.receivedAtMs)/1000;
  if(age<60)lv_label_set_text_fmt(connection,"%s · Daten vor %lu s",link,(unsigned long)age);
  else if(age<3600)lv_label_set_text_fmt(connection,"%s · Daten vor %lu min",link,(unsigned long)(age/60));
  else lv_label_set_text_fmt(connection,"%s · Daten vor %lu h",link,(unsigned long)(age/3600));
  lv_obj_set_style_text_color(connection,shown.apiAvailable?lv_color_hex(0xA7B9CA):color(panel::Status::Unknown),0);
}
static void updateScrollHint() {
  const char* text="";
  if(lv_obj_get_scroll_bottom(detailList)>2)text="Mehr unten · zum Scrollen wischen";
  else if(lv_obj_get_scroll_top(detailList)>2)text="Mehr oben · zum Scrollen wischen";
  if(strcmp(lv_label_get_text(scrollHint),text))lv_label_set_text(scrollHint,text);
}
static void makeCard(const char* name,panel::Status status,const char* body,bool statusVisible=true) {
  const int columns=wide?3:1;
  int cardWidth=(width-2*(wide?16:8)-(columns-1)*12)/columns;
  auto* card=lv_obj_create(detailList);lv_obj_set_width(card,cardWidth);lv_obj_set_height(card,LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(card,10,0);lv_obj_set_style_pad_row(card,10,0);
  lv_obj_set_style_bg_color(card,lv_color_hex(0x142334),0);lv_obj_set_style_border_width(card,1,0);
  lv_obj_set_style_border_color(card,statusVisible&&status!=panel::Status::Ok?color(status):lv_color_hex(0x2D4257),0);
  lv_obj_set_style_radius(card,8,0);lv_obj_set_flex_flow(card,LV_FLEX_FLOW_COLUMN);
  lv_obj_clear_flag(card,LV_OBJ_FLAG_SCROLLABLE);
  auto* row=box(card,0,0,cardWidth-22,LV_SIZE_CONTENT);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START);
  label(row,name,0,0,statusVisible?cardWidth-124:cardWidth-22,&panel_font_20);
  if(statusVisible){auto* state=label(row,"",0,0,94);badge(state,status,panel::statusText(status));}
  auto* values=label(card,body,0,0,cardWidth-22);lv_obj_set_style_text_line_space(values,4,0);
  lv_obj_set_style_text_color(values,lv_color_hex(0xBFCFDD),0);
}
// Hardware follows the configured order; the historical simulator keeps severity order.
static bool preserveEntityOrder=false;
static bool uniformSystemCardHeights=false;
static void renderCards(bool resetScroll) {
  if(!wide&&selected<0)return;
  int scroll=resetScroll?0:lv_obj_get_scroll_y(detailList);lv_obj_clean(detailList);
  if(selected==5) {
    if(!shown.apiAvailable)makeCard("Keine aktuellen Daten",panel::Status::Unknown,shown.message);
    else if(!*shown.alerts)makeCard("Keine Hinweise",panel::Status::Ok,"Alle eingerichteten Systeme sind OK.",false);
    else {
      const char* begin=shown.alerts;
      while(*begin) {
        const char* end=strstr(begin,"\n\n");size_t n=end?size_t(end-begin):strlen(begin);
        char block[1400];memcpy(block,begin,n);block[n]=0;
        char* body=strchr(block,'\n');if(body)*body++=0;
        char* stateText=strrchr(block,':');panel::Status status=panel::Status::Unknown;
        if(stateText){*stateText++=0;if(strstr(stateText,"Kritisch"))status=panel::Status::Critical;else if(strstr(stateText,"Warnung"))status=panel::Status::Warning;}
        makeCard(block,status,body?body:"");if(!end)break;begin=end+2;
      }
    }
  } else {
    unsigned count=0;
    for(unsigned rank=0;rank<(preserveEntityOrder?1u:4u);rank++)for(unsigned i=0;i<shown.entityCount;i++) {
      const auto& entity=shown.entities[i];
      if((!entityFilter||entityFilter(entity))&&(selected<0||entity.category==unsigned(selected))&&(preserveEntityOrder||panel::severityRank(entity.status)==rank)) {
        makeCard(entity.name,entity.status,entity.details);
        if(decorateEntity)decorateEntity(lv_obj_get_child(detailList,-1),entity);
        ++count;
      }
    }
    if(!count)makeCard(selected<0?"Warte auf Daten":panel::categoryName(selected),panel::Status::Unknown,
      selected>=0&&!shown.categories[selected].enabled?"Noch nicht eingerichtet":emptyEntityMessage&&emptyEntityMessage()?emptyEntityMessage():shown.message,false);
  }
  lv_obj_update_layout(detailList);
  // Measure natural content first, so no sensor text is clipped. Notes keep their own height.
  if(uniformSystemCardHeights&&selected!=5) {
    lv_coord_t tallest=0;
    for(uint32_t i=0;i<lv_obj_get_child_cnt(detailList);++i) {
      const auto h=lv_obj_get_height(lv_obj_get_child(detailList,i));
      if(h>tallest)tallest=h;
    }
    for(uint32_t i=0;i<lv_obj_get_child_cnt(detailList);++i)
      lv_obj_set_height(lv_obj_get_child(detailList,i),tallest);
    lv_obj_update_layout(detailList);
  }
  lv_obj_scroll_to_y(detailList,scroll,LV_ANIM_OFF);updateScrollHint();
}
static void navigate(int category) {
  wake();selected=category;
  for(int i=0;i<6;i++) {
    visible(tiles[i],wide||selected<0);
    lv_obj_set_style_border_width(tiles[i],selected==i?2:0,0);
    lv_obj_set_style_border_color(tiles[i],lv_color_hex(0x579DDE),0);
  }
  visible(section,wide||selected>=0);visible(detailList,wide||selected>=0);visible(scrollHint,wide||selected>=0);
  visible(footer,wide||selected<0);visible(demoButton,advance&&(wide||selected<0));visible(backButton,selected>=0);
  lv_label_set_text(pageTitle,selected<0?"Alle Systeme":selected==5?"Hinweise":panel::categoryName(selected));
  renderCards(true);
}
void uiBegin(void (*nextScenario)()) {
  advance=nextScenario;width=lv_disp_get_hor_res(nullptr);height=lv_disp_get_ver_res(nullptr);wide=width>=800;
  selected=-1;dimmed=false;shown=panel::Snapshot{};
  auto* screen=lv_scr_act();lv_obj_set_style_text_font(screen,&panel_font_14,0);
  lv_obj_set_style_bg_color(screen,lv_color_hex(0x0B1420),0);lv_obj_set_style_text_color(screen,lv_color_hex(0xF1F5F9),0);
  lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
  int margin=wide?16:8;
  brand=label(screen,"EAGLENET",margin,8,width-128,wide?&panel_font_20:&panel_font_14);
  health=label(screen,"",width-margin-100,6,100);badge(health,panel::Status::Unknown,"Unbekannt");
  connection=label(screen,"Warte auf Daten",margin,wide?38:30,width-2*margin);
  for(int i=0;i<6;i++) {
    int gap=wide?10:6;int columns=wide?6:3;int tileWidth=(width-2*margin-(columns-1)*gap)/columns;
    tiles[i]=lv_btn_create(screen);lv_obj_set_pos(tiles[i],margin+(i%columns)*(tileWidth+gap),(wide?74:54)+(i/columns)*64);
    lv_obj_set_size(tiles[i],tileWidth,wide?74:58);lv_obj_set_style_pad_all(tiles[i],0,0);
    lv_obj_set_style_bg_color(tiles[i],lv_color_hex(0x182A3C),0);lv_obj_set_style_radius(tiles[i],7,0);
    lv_obj_clear_flag(tiles[i],LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(tiles[i],[](lv_event_t* e){navigate((int)(intptr_t)lv_event_get_user_data(e));},LV_EVENT_CLICKED,(void*)(intptr_t)i);
    tileNames[i]=label(tiles[i],i==5?"Hinweise":panel::categoryName(i),2,wide?7:3,tileWidth-4);
    tileStates[i]=label(tiles[i],"",2,wide?28:20,tileWidth-4);
    tileCounts[i]=label(tiles[i],"",2,wide?51:38,tileWidth-4);
    for(auto* text:{tileNames[i],tileStates[i],tileCounts[i]})lv_obj_set_style_text_align(text,LV_TEXT_ALIGN_CENTER,0);
  }
  section=box(screen,margin,wide?162:52,width-2*margin,32);
  backButton=lv_btn_create(section);lv_obj_set_size(backButton,84,30);lv_obj_set_style_pad_all(backButton,0,0);
  auto* back=label(backButton,"Zurück",0,0,84);lv_obj_set_style_text_align(back,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(back);
  lv_obj_add_event_cb(backButton,[](lv_event_t*){navigate(-1);},LV_EVENT_CLICKED,nullptr);
  pageTitle=label(section,"",wide?102:96,4,width-2*margin-106,&panel_font_20);
  int listTop=wide?206:90;int listBottom=height-(wide?66:26);
  detailList=box(screen,margin,listTop,width-2*margin,listBottom-listTop);
  lv_obj_add_flag(detailList,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(detailList,LV_DIR_VER);
  lv_obj_set_scrollbar_mode(detailList,LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(detailList,wide?LV_FLEX_FLOW_ROW_WRAP:LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(detailList,12,0);lv_obj_set_style_pad_column(detailList,12,0);
  lv_obj_add_event_cb(detailList,[](lv_event_t*){updateScrollHint();},LV_EVENT_SCROLL,nullptr);
  scrollHint=label(screen,"",margin,listBottom+3,width-2*margin);lv_obj_set_style_text_color(scrollHint,lv_color_hex(0xA7B9CA),0);
  footer=label(screen,"",margin,wide?height-26:184,wide?width-270:width-2*margin);
  lv_label_set_long_mode(footer,LV_LABEL_LONG_DOT);lv_obj_set_height(footer,18);
  demoButton=lv_btn_create(screen);lv_obj_set_pos(demoButton,wide?width-248:margin,height-28);
  lv_obj_set_size(demoButton,wide?232:width-2*margin,24);lv_obj_set_style_pad_all(demoButton,0,0);
  auto* caption=label(demoButton,"DEMO: nächster Testfall",0,0,wide?232:width-2*margin);lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(caption);
  lv_obj_add_event_cb(demoButton,[](lv_event_t*){wake();if(advance)advance();},LV_EVENT_CLICKED,nullptr);
  shade=box(lv_layer_top(),0,0,width,height);lv_obj_set_style_bg_color(shade,lv_color_black(),0);lv_obj_set_style_bg_opa(shade,LV_OPA_70,0);
  lv_obj_clear_flag(shade,LV_OBJ_FLAG_CLICKABLE);visible(shade,false);
  static lv_indev_drv_t input;lv_indev_drv_init(&input);input.type=LV_INDEV_TYPE_POINTER;input.read_cb=readTouch;lv_indev_drv_register(&input);
  activity=millis();lastClock=activity;navigate(-1);
}
void uiUpdate(const panel::Snapshot& value) {
  bool changed=shown.entityCount!=value.entityCount||memcmp(shown.entities,value.entities,sizeof(shown.entities))||
    memcmp(shown.categories,value.categories,sizeof(shown.categories))||strcmp(shown.alerts,value.alerts)||
    shown.apiAvailable!=value.apiAvailable||strcmp(shown.message,value.message);
  shown=value;
  lv_label_set_text_fmt(brand,"EAGLENET · %s",shown.demo?"DEMO":"LIVE");badge(health,shown.overall,panel::statusText(shown.overall));
  updateConnection(millis());
  for(unsigned i=0;i<panel::CategoryCount;i++) {
    const auto& category=shown.categories[i];
    lv_label_set_text(tileStates[i],category.enabled?panel::statusText(category.status):"Offen");
    lv_obj_set_style_text_color(tileStates[i],color(category.status),0);
    if(category.enabled)lv_label_set_text_fmt(tileCounts[i],"%u/%u OK",category.good,category.total);else lv_label_set_text(tileCounts[i],"Nicht aktiv");
    lv_obj_set_style_bg_color(tiles[i],lv_color_hex(category.enabled&&category.status!=panel::Status::Ok?0x283044:0x182A3C),0);
  }
  lv_label_set_text(tileStates[5],shown.apiAvailable?(shown.alertCount?"Prüfen":"Keine"):"Prüfen");
  lv_obj_set_style_text_color(tileStates[5],color(shown.overall),0);
  if(shown.apiAvailable)lv_label_set_text_fmt(tileCounts[5],"%u aktiv",shown.alertCount);else lv_label_set_text(tileCounts[5],"API-Fehler");
  lv_label_set_text(footer,shown.message);if(changed)renderCards(false);boardStatus(shown.overall);
}
void uiLoop(uint32_t now) {
  if(uint32_t(now-lastClock)>=1000){lastClock=now;updateConnection(now);}
  if(!hardwareIdleControl&&!dimmed&&uint32_t(now-activity)>60000){dimmed=true;visible(shade,true);boardDim(true);}
  lv_timer_handler();
}
