#include <array>
#include <algorithm>
#include <cassert>
#include "Report.h"

using namespace logid::backend::dj;
using namespace logid::backend;

static const std::array<uint8_t, 34> DJReportDesc = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x20,        //   Report ID (32)
        0x95, 0x0E,        //   Report Count (14)
        0x75, 0x08,        //   Report Size (8)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x09, 0x41,        //   Usage (0x41)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x41,        //   Usage (0x41)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0x85, 0x21,        //   Report ID (33)
        0x95, 0x1F,        //   Report Count (31)
        0x09, 0x42,        //   Usage (0x42)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x42,        //   Usage (0x42)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0               // End Collection
};

bool dj::supportsDjReports(std::vector<uint8_t>&& rdesc)
{
    auto it = std::search(rdesc.begin(), rdesc.end(), DJReportDesc.begin(), DJReportDesc.end());
    return it != rdesc.end();
}

Report::Report(std::vector<uint8_t>& data) : _data (data)
{
    switch(data[Offset::Type])
    {
        case ReportType::Short:
            _data.resize(HeaderLength+ShortParamLength);
            break;
        case ReportType::Long:
            _data.resize(HeaderLength+LongParamLength);
            break;
        default:
            assert(false);
    }
}

Report::Report(Report::Type type, hidpp::DeviceIndex index, uint8_t feature)
{
    switch(type)
    {
        case ReportType::Short:
            _data.resize(HeaderLength+ShortParamLength);
            break;
        case ReportType::Long:
            _data.resize(HeaderLength+LongParamLength);
            break;
        default:
            assert(false);
    }

    _data[Offset::Type] = type;
    _data[Offset::DeviceIndex] = index;
    _data[Offset::Feature] = feature;
}


Report::Type Report::type() const
{
    return static_cast<Type>(_data[Offset::Type]);
}

hidpp::DeviceIndex Report::index() const
{
    return static_cast<hidpp::DeviceIndex>(_data[Offset::DeviceIndex]);
}

uint8_t Report::feature() const
{
    return _data[Offset::Feature];
}

std::vector<uint8_t>::iterator Report::paramBegin()
{
    return _data.begin() + Offset::Parameters;
}

std::vector<uint8_t> Report::rawData() const
{
    return _data;
}
