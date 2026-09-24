#pragma once
#include <cstdint>
namespace resetpolicy {
constexpr uint32_t MarkerAddress=0xc12000, SectorSize=0x1000;
inline bool allowed(bool busy,bool pending){return !busy&&!pending;}
inline bool overlaps(uint32_t address,uint32_t size){return uint64_t(address)<uint64_t(MarkerAddress)+SectorSize&&uint64_t(address)+size>MarkerAddress;}
// Keep durable intent until all customer sectors have been erased and verified.
template<class Storage> bool finish(Storage& storage){return storage.eraseData()&&storage.verifyData()&&storage.eraseMarker()&&storage.verifyMarker();}
}
