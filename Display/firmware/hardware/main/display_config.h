#pragma once
#include <cstdint>
#include <cstring>
namespace displayconfig {
struct Options {
 uint32_t version=1,rotateSeconds=20,nightStart=22,nightEnd=7,brightness=25;
 int32_t utcOffsetMinutes=60;
 bool rotate=false,night=false,interrupt=false,history=false,favoriteHome=false;
 char favorites[24][64]={};char origin[256]={};
};
struct Settings { Options view; char name[49]="PRTG Display"; };
struct Command { uint32_t version=1,revision=0;bool central=false;Settings settings; };
inline bool validOptions(const Options& c){
 if(c.version!=1||!memchr(c.origin,0,sizeof(c.origin))||c.rotateSeconds<10||c.rotateSeconds>60||c.nightStart>23||c.nightEnd>23||c.brightness<10||c.brightness>100||c.utcOffsetMinutes< -720||c.utcOffsetMinutes>840)return false;
 for(const auto& id:c.favorites){if(!memchr(id,0,sizeof(id)))return false;for(const char* p=id;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'))return false;}
 return true;
}
}

