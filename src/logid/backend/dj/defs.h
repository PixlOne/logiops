#ifndef LOGID_BACKEND_DJ_DEFS_H
#define LOGID_BACKEND_DJ_DEFS_H

#include <cstdint>

namespace logid {
namespace backend {
namespace dj
{
    namespace ReportType
    {
        enum ReportType : uint8_t
        {
            Short = 0x20,
            Long = 0x21
        };
    }
}}}

#endif //LOGID_BACKEND_DJ_DEFS_H