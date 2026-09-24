#include "setup_portal.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <fcntl.h>
#include <unistd.h>
namespace setupportal {
void tick(bool enabled){
 static int fd=-1;static int64_t retryAt=0;
 if(!enabled){if(fd>=0){close(fd);fd=-1;ESP_LOGI("SETUP_PORTAL","DNS stopped");}retryAt=0;return;}
 if(fd<0){
  const auto now=esp_timer_get_time();if(now<retryAt)return;retryAt=now+30000000;
  fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
  sockaddr_in local={};local.sin_family=AF_INET;local.sin_port=htons(53);local.sin_addr.s_addr=inet_addr("192.168.4.1");
  if(fd<0||fcntl(fd,F_SETFL,O_NONBLOCK)<0||bind(fd,(sockaddr*)&local,sizeof(local))<0){if(fd>=0)close(fd);fd=-1;ESP_LOGW("SETUP_PORTAL","DNS unavailable; manual browser address remains usable");return;}
  ESP_LOGI("SETUP_PORTAL","DNS started on setup interface");
 }
 // Bounded work on the existing main task, no additional task/stack.
 for(unsigned i=0;i<2;++i){
  uint8_t packet[528];sockaddr_in peer={};socklen_t len=sizeof(peer);
  int n=recvfrom(fd,packet,sizeof(packet),0,(sockaddr*)&peer,&len);if(n<0)break;
  if((ntohl(peer.sin_addr.s_addr)&0xffffff00)!=0xc0a80400)continue;
  size_t count=dns(packet,n,sizeof(packet));if(count)sendto(fd,packet,count,0,(sockaddr*)&peer,len);
 }
}
}
