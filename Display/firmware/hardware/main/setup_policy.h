#pragma once
namespace setuppolicy {
inline bool automatic(bool networkReady,bool missingConfig,bool webReady,bool otaBusy,bool otaPending){return networkReady&&missingConfig&&webReady&&!otaBusy&&!otaPending;}
}
