#include <array>
#include <algorithm>
#include "Report.h"

using namespace logid::backend::hidpp;
using namespace logid::backend;

/* Report descriptors were sourced from cvuchener/hidpp */
static const std::array<uint8_t, 22> ShortReportDesc = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x10,		//   Report ID (16)
        0x75, 0x08,		//   Report Size (8)
        0x95, 0x06,		//   Report Count (6)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

static const std::array<uint8_t, 22> LongReportDesc = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x11,		//   Report ID (17)
        0x75, 0x08,		//   Report Size (8)
        0x95, 0x13,		//   Report Count (19)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

/* Alternative versions from the G602 */
static const std::array<uint8_t, 22> ShortReportDesc2 = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x10,		//   Report ID (16)
        0x95, 0x06,		//   Report Count (6)
        0x75, 0x08,		//   Report Size (8)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

static const std::array<uint8_t, 22> LongReportDesc2 = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x11,		//   Report ID (17)
        0x95, 0x13,		//   Report Count (19)
        0x75, 0x08,		//   Report Size (8)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

uint8_t hidpp::getSupportedReports(std::vector<uint8_t>&& rdesc)
{
    uint8_t ret = 0;

    auto it = std::search(rdesc.begin(), rdesc.end(), ShortReportDesc.begin(), ShortReportDesc.end());
    if(it == rdesc.end())
        it = std::search(rdesc.begin(), rdesc.end(), ShortReportDesc2.begin(), ShortReportDesc2.end());
    if(it != rdesc.end())
        ret |= HIDPP_REPORT_SHORT_SUPPORTED;

    it = std::search(rdesc.begin(), rdesc.end(), LongReportDesc.begin(), LongReportDesc2.end());
    if(it == rdesc.end())
        it = std::search(rdesc.begin(), rdesc.end(), LongReportDesc2.begin(), LongReportDesc2.end());
    if(it != rdesc.end())
        ret |= HIDPP_REPORT_LONG_SUPPORTED;

    return ret;
}