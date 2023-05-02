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

#include <backend/hidpp/Report.h>
#include <array>
#include <algorithm>
#include <cassert>
#include <backend/hidpp10/Error.h>
#include <backend/hidpp20/Error.h>

using namespace logid::backend::hidpp;
using namespace logid::backend;

/* Report descriptors were sourced from cvuchener/hidpp */
static const std::array<uint8_t, 22> ShortReportDesc = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x10,        //   Report ID (16)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x06,        //   Report Count (6)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,    //   Logical Maximum (255)
        0x09, 0x01,        //   Usage (0001 - Vendor)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x01,        //   Usage (0001 - Vendor)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0            // End Collection
};

static const std::array<uint8_t, 22> LongReportDesc = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x11,        //   Report ID (17)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x13,        //   Report Count (19)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,    //   Logical Maximum (255)
        0x09, 0x02,        //   Usage (0002 - Vendor)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x02,        //   Usage (0002 - Vendor)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0            // End Collection
};

/* Alternative versions from the G602 */
static const std::array<uint8_t, 22> ShortReportDesc2 = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x10,        //   Report ID (16)
        0x95, 0x06,        //   Report Count (6)
        0x75, 0x08,        //   Report Size (8)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,    //   Logical Maximum (255)
        0x09, 0x01,        //   Usage (0001 - Vendor)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x01,        //   Usage (0001 - Vendor)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0            // End Collection
};

static const std::array<uint8_t, 22> LongReportDesc2 = {
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x11,        //   Report ID (17)
        0x95, 0x13,        //   Report Count (19)
        0x75, 0x08,        //   Report Size (8)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,    //   Logical Maximum (255)
        0x09, 0x02,        //   Usage (0002 - Vendor)
        0x81, 0x00,        //   Input (Data, Array, Absolute)
        0x09, 0x02,        //   Usage (0002 - Vendor)
        0x91, 0x00,        //   Output (Data, Array, Absolute)
        0xC0            // End Collection
};

uint8_t hidpp::getSupportedReports(const std::vector<uint8_t>& report_desc) {
    uint8_t ret = 0;

    auto it = std::search(report_desc.begin(), report_desc.end(),
                          ShortReportDesc.begin(), ShortReportDesc.end());
    if (it == report_desc.end())
        it = std::search(report_desc.begin(), report_desc.end(),
                         ShortReportDesc2.begin(), ShortReportDesc2.end());
    if (it != report_desc.end())
        ret |= ShortReportSupported;

    it = std::search(report_desc.begin(), report_desc.end(),
                     LongReportDesc.begin(), LongReportDesc.end());
    if (it == report_desc.end())
        it = std::search(report_desc.begin(), report_desc.end(),
                         LongReportDesc2.begin(), LongReportDesc2.end());
    if (it != report_desc.end())
        ret |= LongReportSupported;

    return ret;
}

const char* Report::InvalidReportID::what() const noexcept {
    return "Invalid report ID";
}

const char* Report::InvalidReportLength::what() const noexcept {
    return "Invalid report length";
}

Report::Report(Report::Type type, DeviceIndex device_index,
               uint8_t sub_id, uint8_t address) {
    switch (type) {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }

    _data[Offset::Type] = type;
    _data[Offset::DeviceIndex] = device_index;
    _data[Offset::SubID] = sub_id;
    _data[Offset::Address] = address;
}

Report::Report(Report::Type type, DeviceIndex device_index,
               uint8_t feature_index, uint8_t function, uint8_t sw_id) {
    assert(function <= 0x0f);
    assert(sw_id <= 0x0f);

    switch (type) {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }

    _data[Offset::Type] = type;
    _data[Offset::DeviceIndex] = device_index;
    _data[Offset::Feature] = feature_index;
    _data[Offset::Function] = (function & 0x0f) << 4 |
                              (sw_id & 0x0f);
}

Report::Report(const std::vector<uint8_t>& data) :
        _data(data) {
    _data.resize(HeaderLength + LongParamLength);

    // Truncating data is entirely valid here.
    switch (_data[Offset::Type]) {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }
}

Report::Type Report::type() const {
    return static_cast<Report::Type>(_data[Offset::Type]);
}

void Report::setType(Report::Type type) {
    switch (type) {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }

    _data[Offset::Type] = type;
}

hidpp::DeviceIndex Report::deviceIndex() const {
    return static_cast<hidpp::DeviceIndex>(_data[Offset::DeviceIndex]);
}

[[maybe_unused]] void Report::setDeviceIndex(hidpp::DeviceIndex index) {
    _data[Offset::DeviceIndex] = index;
}

uint8_t Report::feature() const {
    return _data[Offset::Feature];
}

[[maybe_unused]] void Report::setFeature(uint8_t feature) {
    _data[Offset::Parameters] = feature;
}

uint8_t Report::subId() const {
    return _data[Offset::SubID];
}

[[maybe_unused]] void Report::setSubId(uint8_t sub_id) {
    _data[Offset::SubID] = sub_id;
}

uint8_t Report::function() const {
    return (_data[Offset::Function] >> 4) & 0x0f;
}

[[maybe_unused]] void Report::setFunction(uint8_t function) {
    _data[Offset::Function] &= 0x0f;
    _data[Offset::Function] |= (function & 0x0f) << 4;
}

uint8_t Report::swId() const {
    return _data[Offset::Function] & 0x0f;
}

void Report::setSwId(uint8_t sw_id) {
    _data[Offset::Function] &= 0xf0;
    _data[Offset::Function] |= sw_id & 0x0f;
}

uint8_t Report::address() const {
    return _data[Offset::Address];
}

[[maybe_unused]] void Report::setAddress(uint8_t address) {
    _data[Offset::Address] = address;
}

std::vector<uint8_t>::iterator Report::paramBegin() {
    return _data.begin() + Offset::Parameters;
}

std::vector<uint8_t>::iterator Report::paramEnd() {
    return _data.end();
}

std::vector<uint8_t>::const_iterator Report::paramBegin() const {
    return _data.begin() + Offset::Parameters;
}

std::vector<uint8_t>::const_iterator Report::paramEnd() const {
    return _data.end();
}

void Report::setParams(const std::vector<uint8_t>& _params) {
    assert(_params.size() <= _data.size() - HeaderLength);

    for (std::size_t i = 0; i < _params.size(); i++)
        _data[Offset::Parameters + i] = _params[i];
}

bool Report::isError10(Report::Hidpp10Error& error) const {
    if (_data[Offset::Type] != Type::Short ||
        _data[Offset::SubID] != hidpp10::ErrorID)
        return false;

    error.device_index = deviceIndex();
    error.sub_id = _data[3];
    error.address = _data[4];
    error.error_code = _data[5];

    return true;
}

bool Report::isError20(Report::Hidpp20Error& error) const {
    if (_data[Offset::Type] != Type::Long ||
        _data[Offset::Feature] != hidpp20::ErrorID)
        return false;

    error.device_index = deviceIndex();
    error.feature_index = _data[3];
    error.function = (_data[4] >> 4) & 0x0f;
    error.software_id = _data[4] & 0x0f;
    error.error_code = _data[5];

    return true;
}

const std::vector<uint8_t>& Report::rawReport() const {
    return _data;
}
