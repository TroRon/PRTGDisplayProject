#pragma once
#include <ArduinoJson.h>
#include <cstring>
// Only an outage of ALL active sensors triggers the demo. Missing/stale data,
// real PRTG alarms and partial outages must remain visible as live conditions.
inline bool allSourcesUnavailable(JsonObject categories) {
    unsigned count=0;
    for(JsonPair category:categories){
        if(!(category.value()["enabled"]|false))continue;
        for(JsonObject entity:category.value()["entities"].as<JsonArray>())
            for(JsonObject sensor:entity["sensors"].as<JsonArray>()){
                ++count;const char* reason=sensor["reason"]|"";
                if(strcmp(sensor["status"]|"","unknown"))return false;
                if(strcmp(reason,"SOURCE_UNAVAILABLE")&&strcmp(reason,"PRTG_NETWORK_OR_TLS")&&
                   strcmp(reason,"PRTG_AUTH")&&strcmp(reason,"PRTG_HTTP")&&strcmp(reason,"PRTG_JSON")&&strcmp(reason,"PRTG_SHAPE"))return false;
            }
    }
    return count>0;
}
