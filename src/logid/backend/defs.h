#ifndef LOGID_BACKEND_DEFS_H
#define LOGID_BACKEND_DEFS_H

#include <functional>

namespace logid::backend
{
    struct RawEventHandler
    {
        std::function<bool(std::vector<uint8_t>& )> condition;
        std::function<void(std::vector<uint8_t>& )> callback;
    };
}

#endif //LOGID_BACKEND_DEFS_H