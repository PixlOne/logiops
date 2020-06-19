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

    static constexpr uint8_t ErrorFeature = 0x7f;

    static constexpr std::size_t HeaderLength = 3;
    static constexpr std::size_t ShortParamLength = 12;
    static constexpr std::size_t LongParamLength = 29;
}}}

#endif //LOGID_BACKEND_DJ_DEFS_H