#pragma once
#include <cstring>
#include <cstdio>
namespace preferences {
struct Config { unsigned version=1;char origin[256]={};char name[49]="PRTG Display"; };
inline bool valid(const Config& c){
 if(c.version!=1||!memchr(c.origin,0,sizeof(c.origin))||!memchr(c.name,0,sizeof(c.name))||!*c.name)return false;
 for(const unsigned char* p=(const unsigned char*)c.name;*p;++p)if(*p<32||*p==127)return false;
 if(strncmp(c.origin,"https://",8)||!c.origin[8])return false;
 for(const unsigned char* p=(const unsigned char*)c.origin+8;*p;++p)
  if(*p<33||*p>126||*p=='/'||*p=='@'||*p=='?'||*p=='#'||*p=='\\')return false;
 return true;
}
void begin();
void get(Config&);
bool save(const Config&);
bool endpoint(const char* path,char* output,size_t capacity);
}
