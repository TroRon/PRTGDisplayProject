#pragma once
#include "live.h"
#include "ota_policy.h"
#include "preferences.h"
#include <ArduinoJson.h>
// Mutable buffer keeps decoded secrets in the caller's buffer, which is cleared after use.
inline bool decodeConfig(char* json,live::Config& value) {
    DynamicJsonDocument doc(2048);
    if(deserializeJson(doc,json)||!doc.is<JsonObject>()||doc.size()!=4)return false;
    const char* names[]={"ssid","password","token","ntp"};
    char* targets[]={value.ssid,value.password,value.token,value.ntp};
    size_t sizes[]={sizeof(value.ssid),sizeof(value.password),sizeof(value.token),sizeof(value.ntp)};
    for(unsigned i=0;i<4;i++){
        if(!doc[names[i]].is<const char*>())return false;
        JsonString text=doc[names[i]].as<JsonString>();
        if(text.size()>=sizes[i]||text.size()!=strlen(text.c_str()))return false;
        memcpy(targets[i],text.c_str(),text.size()+1);
    }
    return live::validate(value);
}
inline bool decodeOtaConfig(char* json,ota::Config& value){
    DynamicJsonDocument doc(1024);
    if(deserializeJson(doc,json)||!doc.is<JsonObject>()||doc.size()!=2||
       !doc["direct"].is<bool>()||!doc["url"].is<const char*>())return false;
    JsonString url=doc["url"].as<JsonString>();
    if(url.size()>=sizeof(value.url)||url.size()!=strlen(url.c_str()))return false;
    value.direct=doc["direct"].as<bool>();
    memcpy(value.url,url.c_str(),url.size()+1);
    return ota::validConfig(value);
}
inline bool decodePanelConfig(char* json,preferences::Config& value){
    DynamicJsonDocument doc(1024);
    if(deserializeJson(doc,json)||!doc.is<JsonObject>()||doc.size()!=2)return false;
    const char* names[]={"origin","name"};
    char* targets[]={value.origin,value.name};
    size_t sizes[]={sizeof(value.origin),sizeof(value.name)};
    for(unsigned i=0;i<2;++i){
        if(!doc[names[i]].is<const char*>())return false;
        JsonString text=doc[names[i]].as<JsonString>();
        if(text.size()>=sizes[i]||text.size()!=strlen(text.c_str()))return false;
        memcpy(targets[i],text.c_str(),text.size()+1);
    }
    return preferences::valid(value);
}
