#ifndef LOGID_BACKEND_HIDPP_REPORT_H
#define LOGID_BACKEND_HIDPP_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "defs.h"

/* Some devices only support a subset of these reports */
#define HIDPP_REPORT_SHORT_SUPPORTED      1U
#define HIDPP_REPORT_LONG_SUPPORTED	      1U<<1U
/* Very long reports exist, however they have not been encountered so far */

namespace logid::backend::hidpp
{
    uint8_t getSupportedReports(std::vector<uint8_t>&& rdesc);

    namespace Offset
    {
        static constexpr uint8_t Type = 0;
        static constexpr uint8_t DeviceIndex = 1;
        static constexpr uint8_t Feature = 2;
        static constexpr uint8_t Function = 3;
        static constexpr uint8_t Parameters = 4;
    }

    class Report
    {
    public:
        enum Type: uint8_t
        {
            Short = 0x10,
            Long = 0x11
        };

        class InvalidReportID: public std::exception
        {
        public:
            InvalidReportID() = default;
            virtual const char* what() const noexcept;
        };

        class InvalidReportLength: public std::exception
        {
        public:
            InvalidReportLength() = default;;
            virtual const char* what() const noexcept;
        };

        static constexpr std::size_t MaxDataLength = 20;
        static constexpr uint8_t swIdMask = 0x0f;
        static constexpr uint8_t functionMask = 0x0f;

        Report(Type type, DeviceIndex device_index,
                uint8_t feature_index,
                uint8_t function,
                uint8_t sw_id);
        explicit Report(const std::vector<uint8_t>& data);

        Type type() const { return static_cast<Type>(_data[Offset::Type]); };
        void setType(Report::Type type);

        std::vector<uint8_t>::const_iterator paramBegin() const { return _data.begin() + Offset::Parameters; }
        std::vector<uint8_t>::const_iterator paramEnd() const { return _data.end(); }
        void setParams(const std::vector<uint8_t>& _params);

        logid::backend::hidpp::DeviceIndex deviceIndex()
        {
            return static_cast<DeviceIndex>(_data[Offset::DeviceIndex]);
        }

        std::vector<uint8_t> rawReport () const { return _data; }
    private:
        static constexpr std::size_t HeaderLength = 4;
        std::vector<uint8_t> _data;
    };
}

#endif //LOGID_BACKEND_HIDPP_REPORT_H