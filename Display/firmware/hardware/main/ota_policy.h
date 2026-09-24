#pragma once
#include <cstring>
#include <cstdio>
#include "ota.h"
namespace ota {
inline unsigned versionSequence(const char* version){
 unsigned major=0,minor=0,patch=0;char tail=0,canonical[32];
 if(sscanf(version,"%u.%u.%u%c",&major,&minor,&patch,&tail)!=3||major>429495||minor>99||patch>99)return 0;
 snprintf(canonical,sizeof(canonical),"%u.%u.%u",major,minor,patch);
 return strcmp(version,canonical)?0:major*10000+minor*100+patch;
}
inline bool httpsUrl(const char* url){
 if(!url||strncmp(url,"https://",8))return false;
 const char* path=strchr(url+8,'/');
 if(!path||path==url+8||!path[1])return false;
 for(const unsigned char* p=(const unsigned char*)url;*p;++p)
  if(*p<33||*p>126||*p=='@'||*p=='#'||*p=='?'||*p=='\\')return false;
 return strlen(url)<384;
}
inline bool validConfig(const Config& c){return c.version==1&&memchr(c.url,0,sizeof(c.url))&&(!c.direct||httpsUrl(c.url));}
inline bool hashText(const char* s){
 if(!s||strlen(s)!=64)return false;
 for(unsigned i=0;i<64;++i)if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;
 return true;
}
inline bool binaryUrl(const char* manifest,const char* hash,char* dest,size_t cap){
 if(!httpsUrl(manifest)||!hashText(hash))return false;
 const char* last=strrchr(manifest,'/');
 const int n=snprintf(dest,cap,"%.*s/%s.bin",(int)(last-manifest),manifest,hash);
 return n>0&&(size_t)n<cap;
}
}
