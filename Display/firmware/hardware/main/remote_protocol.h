#pragma once
#include "display_config.h"
#include <ArduinoJson.h>
#include <cstdio>
namespace remoteprotocol {
inline void encode(JsonObject o,const displayconfig::Settings& s){const auto& v=s.view;o["name"]=s.name;o["rotate"]=v.rotate;o["rotateSeconds"]=v.rotateSeconds;o["night"]=v.night;o["nightStart"]=v.nightStart;o["nightEnd"]=v.nightEnd;o["brightness"]=v.brightness;o["utcOffsetMinutes"]=v.utcOffsetMinutes;o["interrupt"]=v.interrupt;o["history"]=v.history;o["favoriteHome"]=v.favoriteHome;auto a=o.createNestedArray("favorites");for(const auto& id:v.favorites)if(*id)a.add(id);}
inline bool decode(JsonObject o,displayconfig::Settings& s){
 if(o.size()!=12||!o["name"].is<const char*>()||!strlen(o["name"].as<const char*>())||strlen(o["name"].as<const char*>())>48)return false;
 for(const char* k:{"rotate","night","interrupt","history","favoriteHome"})if(!o[k].is<bool>())return false;
 for(const char* k:{"rotateSeconds","nightStart","nightEnd","brightness","utcOffsetMinutes"})if(!o[k].is<int>())return false;
 auto& v=s.view;snprintf(s.name,sizeof(s.name),"%s",o["name"].as<const char*>());for(const unsigned char* p=(const unsigned char*)s.name;*p;p++)if(*p<32||*p==127)return false;
 v.rotate=o["rotate"];v.night=o["night"];v.interrupt=o["interrupt"];v.history=o["history"];v.favoriteHome=o["favoriteHome"];v.rotateSeconds=o["rotateSeconds"].as<int>();v.nightStart=o["nightStart"].as<int>();v.nightEnd=o["nightEnd"].as<int>();v.brightness=o["brightness"].as<int>();v.utcOffsetMinutes=o["utcOffsetMinutes"].as<int>();
 if(!o["favorites"].is<JsonArray>()||o["favorites"].size()>24)return false;
 memset(v.favorites,0,sizeof(v.favorites));unsigned count=0;for(JsonVariant id:o["favorites"].as<JsonArray>()){if(!id.is<const char*>()||!strlen(id.as<const char*>())||strlen(id.as<const char*>())>63)return false;for(unsigned i=0;i<count;i++)if(!strcmp(v.favorites[i],id.as<const char*>()))return false;snprintf(v.favorites[count++],64,"%s",id.as<const char*>());}return displayconfig::validOptions(v);
}
}

