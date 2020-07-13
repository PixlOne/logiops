#ifndef LOGID_LOGID_H
#define LOGID_LOGID_H

#include <mutex>

namespace logid
{
    void reload();

    extern bool kill_logid;
    extern std::mutex finder_reloading;
}

#endif //LOGID_LOGID_H