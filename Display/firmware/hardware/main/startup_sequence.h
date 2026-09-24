#pragma once
#include <stddef.h>
namespace startup {
struct Step { const char* name; bool (*start)(); void (*undo)(); };
// Includes the failed step in rollback: its acquisition may have been partial.
inline const char* run(const Step* steps,size_t count) {
    for(size_t i=0;i<count;i++) {
        if(steps[i].start())continue;
        for(size_t remaining=i+1;remaining;--remaining)if(steps[remaining-1].undo)steps[remaining-1].undo();
        return steps[i].name;
    }
    return nullptr;
}
}
