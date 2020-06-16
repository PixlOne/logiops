#ifndef LOGID_BACKEND_DJ_REPORT_H
#define LOGID_BACKEND_DJ_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"

namespace logid::backend::dj
{
    bool supportsDjReports(std::vector<uint8_t>& rawDevice);
    class Report
    {
        enum Type: uint8_t
        {
            Short = 0x20, // Short DJ reports use 12 byte parameters
            Long = 0x21   // Long DJ reports use 29 byte parameters
        };
    };
}

#endif //LOGID_BACKEND_DJ_REPORT_H
