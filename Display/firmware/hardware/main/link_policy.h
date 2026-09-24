#pragma once
#include "panel_model.h"
#include <cstdio>

namespace link_policy {
constexpr uint32_t PollMs=15000,RetryMs=1500;
inline bool transientHttp(int code){return code==408||code==429||(code>=500&&code<=599);}
// One retry per regular poll; a failed retry never schedules another quick retry.
class RetryPolicy {
public:
    bool pending=false;
    uint32_t delay=PollMs;
    void completed(bool transient){pending=transient&&!pending;delay=pending?RetryMs:PollMs;}
    void reset(){pending=false;delay=PollMs;}
};
inline void unavailable(panel::Snapshot& value,const char* reason){
    // Preserve values AND their original timestamps, but never old healthy badges.
    value.apiAvailable=false;value.complete=false;value.overall=panel::Status::Unknown;
    value.alertCount=0;value.alerts[0]=0;
    for(auto& category:value.categories){category.status=panel::Status::Unknown;category.good=0;}
    for(unsigned i=0;i<value.entityCount;++i)value.entities[i].status=panel::Status::Unknown;
    snprintf(value.message,sizeof(value.message),"%s",reason);
}
}
