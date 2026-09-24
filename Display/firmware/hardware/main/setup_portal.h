#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace setupportal {
// One ordinary IN question; reject compression, oversized labels and ambiguous packets.
// No recursive resolver, allocations, reflected additional records or unbounded loops.
inline size_t dns(uint8_t* p,size_t n,size_t capacity){
 if(n<17||n>512||capacity<n||p[2]&0xf8||p[3]&0xcf||p[4]||p[5]!=1||p[6]||p[7]||p[8]||p[9]||p[10]||p[11]>1)return 0;
 size_t pos=12;
 while(pos<n&&p[pos]){size_t len=p[pos++];if(len>63||pos+len>=n||pos+len-12>253)return 0;pos+=len;}
 if(pos>=n||pos==12||pos+5>n)return 0;
 size_t end=pos+5;
 // Accept one bounded EDNS OPT record from modern phone resolvers; never reflect it.
 if(p[11]){
  if(end+11>n||p[end]||p[end+1]||p[end+2]!=41)return 0;
  size_t extra=(p[end+9]<<8)|p[end+10];if(end+11+extra!=n)return 0;
 }else if(end!=n)return 0;
 ++pos;unsigned type=(p[pos]<<8)|p[pos+1];if(p[pos+2]||p[pos+3]!=1)return 0;
 n=end;p[11]=0;bool answer=type==1;if(answer&&n+16>capacity)return 0;
 p[2]=0x80|(p[2]&1);p[3]=0;p[7]=answer?1:0;
 if(answer){const uint8_t rr[]={0xc0,0x0c,0,1,0,1,0,0,0,0,0,4,192,168,4,1};memcpy(p+n,rr,sizeof(rr));n+=sizeof(rr);}
 return n;
}
inline bool redirect(bool apActive,bool apSocket,bool apPeer,bool get,const char* uri){
 return apActive&&apSocket&&apPeer&&get&&uri&&strncmp(uri,"/api/",5)!=0;
}
inline bool wifiPayload(const char* password,char* out,size_t size){
 if(!password||strlen(password)!=32)return false;
 for(unsigned i=0;i<32;++i)if(!((password[i]>='0'&&password[i]<='9')||(password[i]>='a'&&password[i]<='f')))return false;
 const char prefix[]="WIFI:T:WPA;S:SetupPRTGDisplay;P:";
 if(size<sizeof(prefix)+32+2)return false;
 memcpy(out,prefix,sizeof(prefix)-1);memcpy(out+sizeof(prefix)-1,password,32);memcpy(out+sizeof(prefix)-1+32,";;",3);return true;
}
void tick(bool enabled);
}
