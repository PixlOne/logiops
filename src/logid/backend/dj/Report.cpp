#include <array>
#include <algorithm>
#include "Report.h"

using namespace logid::backend::dj;
using namespace logid::backend;

static const std::array<uint8_t, 35> DJReportDesc = {
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

bool dj::supportsDjReports(std::vector<uint8_t>& rdesc)
{
    auto it = std::search(rdesc.begin(), rdesc.end(), DJReportDesc.begin(), DJReportDesc.end());
    return it != rdesc.end();
}