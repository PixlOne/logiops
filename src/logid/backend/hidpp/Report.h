#ifndef LOGID_BACKEND_HIDPP_REPORT_H
#define LOGID_BACKEND_HIDPP_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "defs.h"

/* Some devices only support a subset of these reports */
#define HIDPP_REPORT_SHORT_SUPPORTED      1U
#define HIDPP_REPORT_LONG_SUPPORTED	      1U<<1U
/* Very long reports exist, however they have not been encountered so far */

namespace logid {
namespace backend {
namespace hidpp
{
    uint8_t getSupportedReports(std::vector<uint8_t>&& rdesc);

    namespace Offset
    {
        static constexpr uint8_t Type = 0;
        static constexpr uint8_t DeviceIndex = 1;
        static constexpr uint8_t SubID = 2;
        static constexpr uint8_t Feature = 2;
        static constexpr uint8_t Address = 3;
        static constexpr uint8_t Function = 3;
        static constexpr uint8_t Parameters = 4;
    }

    class Report
    {
    public:
        typedef ReportType::ReportType Type;

        class InvalidReportID: public std::exception
        {
        public:
            InvalidReportID() = default;
            const char* what() const noexcept override;
        };

        class InvalidReportLength: public std::exception
        {
        public:
            InvalidReportLength() = default;;
            const char* what() const noexcept override;
        };

        static constexpr std::size_t MaxDataLength = 20;

        Report(Report::Type type, DeviceIndex device_index,
               uint8_t sub_id,
               uint8_t address);
        Report(Report::Type type, DeviceIndex device_index,
                uint8_t feature_index,
                uint8_t function,
                uint8_t sw_id);
        explicit Report(const std::vector<uint8_t>& data);

        Report::Type type() const;
        void setType(Report::Type type);

        logid::backend::hidpp::DeviceIndex deviceIndex();
        void setDeviceIndex(hidpp::DeviceIndex index);

        uint8_t feature() const;
        void setFeature(uint8_t feature);

        uint8_t subId() const;
        void setSubId(uint8_t sub_id);

        uint8_t function() const;
        void setFunction(uint8_t function);

        uint8_t swId() const;
        void setSwId(uint8_t sw_id);

        uint8_t address() const;
        void setAddress(uint8_t address);

        std::vector<uint8_t>::iterator paramBegin()
        {
            return _data.begin() + Offset::Parameters;
        }
        std::vector<uint8_t>::iterator paramEnd() { return _data.end(); }
        void setParams(const std::vector<uint8_t>& _params);

        struct Hidpp10Error
        {
            uint8_t sub_id, address, error_code;
        };
        bool isError10(Hidpp10Error* error);

        struct Hidpp20Error
        {
            uint8_t feature_index, function, software_id, error_code;
        };
        bool isError20(Hidpp20Error* error);

        std::vector<uint8_t> rawReport () const { return _data; }

        static constexpr std::size_t HeaderLength = 4;
    private:
        std::vector<uint8_t> _data;
    };
}}}

#endif //LOGID_BACKEND_HIDPP_REPORT_H