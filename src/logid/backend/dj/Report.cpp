/*
 * Copyright 2019-2023 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <backend/dj/Report.h>
#include <algorithm>
#include <array>
#include <cassert>

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

static const std::array<uint8_t, 39> DJReportDesc2 = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x20,        //   Report ID (32)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x0E,        //   Report Count (14)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x09, 0x41,        //   Usage (0x41)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x41,        //   Usage (0x41)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0x85, 0x21,        //   Report ID (33)
        0x95, 0x1F,        //   Report Count (31)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x09, 0x42,        //   Usage (0x42)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x42,        //   Usage (0x42)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0               // End Collection
};

bool dj::supportsDjReports(const std::vector<uint8_t>& report_desc) {
    auto it = std::search(report_desc.begin(), report_desc.end(),
                          DJReportDesc.begin(), DJReportDesc.end());
    if (it == report_desc.end())
        it = std::search(report_desc.begin(), report_desc.end(),
                         DJReportDesc2.begin(), DJReportDesc2.end());
    return it != report_desc.end();
}

Report::Report(const std::vector<uint8_t>& data) : _data(data) {
    switch (data[Offset::Type]) {
        case ReportType::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case ReportType::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            assert(false);
    }
}

Report::Report(Report::Type type, hidpp::DeviceIndex index, uint8_t feature) {
    switch (type) {
        case ReportType::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case ReportType::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            assert(false);
    }

    _data[Offset::Type] = type;
    _data[Offset::DeviceIndex] = index;
    _data[Offset::Feature] = feature;
}


Report::Type Report::type() const {
    return static_cast<Type>(_data[Offset::Type]);
}

hidpp::DeviceIndex Report::index() const {
    return static_cast<hidpp::DeviceIndex>(_data[Offset::DeviceIndex]);
}

uint8_t Report::feature() const {
    return _data[Offset::Feature];
}

std::vector<uint8_t>::iterator Report::paramBegin() {
    return _data.begin() + Offset::Parameters;
}

const std::vector<uint8_t>& Report::rawData() const {
    return _data;
}
