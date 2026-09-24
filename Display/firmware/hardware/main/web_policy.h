#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
namespace webpolicy {
inline bool needsInitial(bool configured,bool missing,bool storageOk,bool done){return !configured&&missing&&storageOk&&!done;}
inline bool validInitial(const char (&p)[13]){if(p[12])return false;for(unsigned i=0;i<12;++i)if(p[i]<'0'||p[i]>'9')return false;return true;}
inline bool validPassword(const char* p){
 if(!p)return false;
 const size_t n=strlen(p);if(n<12||n>63)return false;
 for(size_t i=0;i<n;++i)if((unsigned char)p[i]<32||(unsigned char)p[i]==127)return false;
 return true;
}
inline bool equal(const char* a,const char* b,size_t size){unsigned diff=0;for(size_t i=0;i<size;++i)diff|=(unsigned char)a[i]^(unsigned char)b[i];return diff==0;}
struct Session {
 char id[65]={},csrf[65]={};uint32_t issued=0;
 bool valid(const char* cookie,uint32_t now) const {
  if(!*id||!cookie||uint32_t(now-issued)>=15*60*1000)return false;
  while(*cookie){while(*cookie==' '||*cookie==';')++cookie;const char* end=strchr(cookie,';');const size_t size=end?size_t(end-cookie):strlen(cookie);
   if(size==68&&!strncmp(cookie,"sid=",4)&&equal(cookie+4,id,64))return true;
   if(!end)break;
   cookie=end+1;
  }return false;
 }
 bool write(const char* cookie,const char* token,uint32_t now) const {return valid(cookie,now)&&token&&strlen(token)==64&&equal(token,csrf,64);}
 void clear(){memset(id,0,sizeof(id));memset(csrf,0,sizeof(csrf));issued=0;}
};
struct LoginLimit {
 unsigned failures=0;uint32_t last=0;
 bool allowed(uint32_t now) const {return !failures||uint32_t(now-last)>=(failures>=5?60000u:1000u);}
 void failed(uint32_t now){if(failures<5)++failures;last=now;}
 void success(){failures=0;last=0;}
};
inline bool host(const char* header,const char* ip){
 if(!header||!ip||!*ip||!strcmp(ip,"-"))return false;
 char expected[32];snprintf(expected,sizeof(expected),"%s:80",ip);return !strcmp(header,ip)||!strcmp(header,expected);
}
inline bool origin(const char* header,const char* ip){
 return header&&!strncmp(header,"http://",7)&&host(header+7,ip);
}
}
