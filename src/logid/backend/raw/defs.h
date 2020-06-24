#ifndef LOGID_BACKEND_RAW_DEFS_H
#define LOGID_BACKEND_RAW_DEFS_H

#include <functional>
#include <cstdint>
#include <vector>

namespace logid {
namespace backend {
namespace raw
{
    struct RawEventHandler
    {
        std::function<bool(std::vector<uint8_t>& )> condition;
        std::function<void(std::vector<uint8_t>& )> callback;
    };
}}}

#endif //LOGID_BACKEND_RAW_DEFS_H