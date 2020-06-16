#ifndef LOGID_BACKEND_HIDPP_REPORT_H
#define LOGID_BACKEND_HIDPP_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "Device.h"

#define LOGID_HIDPP_SW_ID 0x0f

/* Some devices only support a subset of these reports */
#define HIDPP_REPORT_SHORT_SUPPORTED      1U
#define HIDPP_REPORT_LONG_SUPPORTED	      1U<<1U
/* Very long reports exist, however they have not been encountered so far */

namespace logid::backend::hidpp
{
    uint8_t getSupportedReports(std::vector<uint8_t>&& rdesc);
    class Report
    {
    public:
        enum Type: uint8_t
        {
            Short = 0x10,
            Long = 0x11
        };

        class InvalidReportID: std::exception
        {
            InvalidReportID();
            virtual const char* what() const noexcept;
        };

        class InvalidReportLength: std::exception
        {
            InvalidReportLength();
            virtual const char* what() const noexcept;
        };

        static constexpr std::size_t MaxDataLength = 32;

        Report(uint8_t report_id, const uint8_t* data, std::size_t length);
        Report(std::vector<uint8_t> data);

        Type type() const;
        void setType(Report::Type type);

        logid::backend::hidpp::DeviceIndex deviceIndex();

        std::vector<uint8_t> rawReport () const { return _data; }

    private:
        static constexpr std::size_t HeaderLength = 4;
        std::vector<uint8_t> _data;
    };
}

#endif //LOGID_BACKEND_HIDPP_REPORT_H