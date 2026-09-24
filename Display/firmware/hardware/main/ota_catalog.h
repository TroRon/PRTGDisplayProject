#pragma once
#include "ota_policy.h"
#include <ArduinoJson.h>
namespace ota {
struct Offer {unsigned sequence=0,size=0;char hash[65]={};char version[32]={};};
inline bool offerMetadata(JsonVariantConst doc,Offer& result){
 if(!doc.is<JsonObjectConst>()||(doc["schema"]|0)!=1||strcmp(doc["board"]|"",Board)||strcmp(doc["layout"]|"",Layout)||
    !doc["sequence"].is<unsigned>()||!doc["size"].is<unsigned>())return false;
 const char* version=doc["version"]|"",*hash=doc["sha256"]|"";
 if(!hashText(hash)||strlen(version)>=sizeof(result.version)||!versionSequence(version))return false;
 if(doc["sequence"].as<unsigned>()!=versionSequence(version)||doc["size"].as<unsigned>()<1024||doc["size"].as<unsigned>()>0x400000)return false;
 result.sequence=doc["sequence"];result.size=doc["size"];
 snprintf(result.version,sizeof(result.version),"%s",version);snprintf(result.hash,sizeof(result.hash),"%s",hash);return true;
}
inline bool addOffer(Offer* list,unsigned& count,const Offer& value){
 for(unsigned i=0;i<count;++i)if(list[i].sequence==value.sequence)return !strcmp(list[i].hash,value.hash)&&list[i].size==value.size;
 if(count>=MaxReleases)return false;
 unsigned at=count++;while(at&&list[at-1].sequence<value.sequence){list[at]=list[at-1];--at;}list[at]=value;return true;
}
inline bool catalogUrl(const char* manifest,char* out,size_t size){
 if(!httpsUrl(manifest))return false;
 auto last=strrchr(manifest,'/');int n=snprintf(out,size,"%.*s/catalog.json",int(last-manifest),manifest);
 return n>0&&size_t(n)<size;
}
inline const char* actionText(unsigned sequence){return sequence<Sequence?"Downgrade":sequence==Sequence?"Neuinstallation":"Update";}
}
