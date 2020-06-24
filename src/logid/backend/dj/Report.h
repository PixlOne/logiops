#ifndef LOGID_BACKEND_DJ_REPORT_H
#define LOGID_BACKEND_DJ_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "defs.h"
#include "../hidpp/defs.h"

namespace logid {
namespace backend {
namespace dj
{
    namespace Offset
    {
        static constexpr uint8_t Type = 0;
        static constexpr uint8_t DeviceIndex = 1;
        static constexpr uint8_t Feature = 2;
        static constexpr uint8_t Parameters = 3;
    }

    bool supportsDjReports(std::vector<uint8_t>&& rdesc);
    class Report
    {
    public:
        typedef ReportType::ReportType Type;

        explicit Report(std::vector<uint8_t>& data);
        Report(Type type, hidpp::DeviceIndex index, uint8_t feature);

        Type type() const;
        hidpp::DeviceIndex index() const;
        uint8_t feature() const;
        std::vector<uint8_t>::iterator paramBegin();
        std::vector<uint8_t> rawData() const;
    private:
        std::vector<uint8_t> _data;
    };
}}}

#endif //LOGID_BACKEND_DJ_REPORT_H
