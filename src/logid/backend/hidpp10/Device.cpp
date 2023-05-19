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

#include <backend/hidpp10/Device.h>
#include <backend/Error.h>
#include <cassert>
#include <utility>

using namespace logid::backend;
using namespace logid::backend::hidpp10;

hidpp::Report setupRegReport(hidpp::DeviceIndex index,
                             uint8_t sub_id, uint8_t address,
                             const std::vector<uint8_t>& params) {
    hidpp::Report::Type type = params.size() <= hidpp::ShortParamLength ?
                               hidpp::Report::Type::Short : hidpp::Report::Type::Long;

    if (sub_id == SetRegisterLong) {
        // When setting a long register, the report must be long.
        type = hidpp::Report::Type::Long;
    }

    hidpp::Report request(type, index, sub_id, address);
    std::copy(params.begin(), params.end(), request.paramBegin());

    return request;
}

Device::Device(const std::string& path, hidpp::DeviceIndex index,
               const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout) :
        hidpp::Device(path, index, monitor, timeout) {
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_dev, hidpp::DeviceIndex index,
               double timeout) : hidpp::Device(std::move(raw_dev), index, timeout) {
}

Device::Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceIndex index,
               double timeout)
        : hidpp::Device(receiver, index, timeout) {
}

void Device::ResponseSlot::reset() {
    response.reset();
    sub_id.reset();
}

hidpp::Report Device::sendReport(const hidpp::Report& report) {
    auto& response_slot = _responses[report.subId() % SubIDCount];

    std::unique_lock<std::mutex> lock(_response_mutex);
    _response_cv.wait(lock, [&response_slot]() {
        return !response_slot.sub_id.has_value();
    });
    response_slot.sub_id = report.subId();

    _sendReport(report);
    bool valid = _response_cv.wait_for(lock, io_timeout, [&response_slot]() {
        return response_slot.response.has_value();
    });

    if (!valid) {
        response_slot.reset();
        throw TimeoutError();
    }

    auto response = response_slot.response.value();
    response_slot.reset();

    if (std::holds_alternative<hidpp::Report>(response)) {
        return std::get<hidpp::Report>(response);
    } else { // if(std::holds_alternative<hidpp::Report::Hidpp10Error>(response))
        auto error = std::get<hidpp::Report::Hidpp10Error>(response);
        throw Error(error.error_code, error.device_index);
    }
}

bool Device::responseReport(const hidpp::Report& report) {
    std::lock_guard<std::mutex> lock(_response_mutex);
    uint8_t sub_id;

    bool is_error = false;
    hidpp::Report::Hidpp10Error hidpp10_error{};
    if (report.isError10(hidpp10_error)) {
        sub_id = hidpp10_error.sub_id;
        is_error = true;
    } else {
        sub_id = report.subId();
    }

    auto& response_slot = _responses[sub_id % SubIDCount];

    if (!response_slot.sub_id.has_value() || response_slot.sub_id.value() != sub_id)
        return false;

    if (is_error) {
        response_slot.response = hidpp10_error;
    } else {
        response_slot.response = report;
    }

    _response_cv.notify_all();
    return true;
}

std::vector<uint8_t> Device::getRegister(uint8_t address,
                                         const std::vector<uint8_t>& params,
                                         hidpp::Report::Type type) {
    assert(params.size() <= hidpp::LongParamLength);

    uint8_t sub_id = type == hidpp::Report::Type::Short ?
                     GetRegisterShort : GetRegisterLong;

    return accessRegister(sub_id, address, params);
}

std::vector<uint8_t> Device::setRegister(uint8_t address,
                                         const std::vector<uint8_t>& params,
                                         hidpp::Report::Type type) {
    assert(params.size() <= hidpp::LongParamLength);

    uint8_t sub_id = type == hidpp::Report::Type::Short ?
                     SetRegisterShort : SetRegisterLong;

    return accessRegister(sub_id, address, params);
}

void Device::setRegisterNoResponse(uint8_t address,
                                   const std::vector<uint8_t>& params,
                                   hidpp::Report::Type type) {
    assert(params.size() <= hidpp::LongParamLength);

    uint8_t sub_id = type == hidpp::Report::Type::Short ?
                     SetRegisterShort : SetRegisterLong;

    return accessRegisterNoResponse(sub_id, address, params);
}

std::vector<uint8_t> Device::accessRegister(uint8_t sub_id, uint8_t address,
                                            const std::vector<uint8_t>& params) {
    auto response = sendReport(setupRegReport(deviceIndex(), sub_id, address, params));
    return {response.paramBegin(), response.paramEnd()};
}

void Device::accessRegisterNoResponse(uint8_t sub_id, uint8_t address,
                                      const std::vector<uint8_t>& params) {
    sendReportNoACK(setupRegReport(deviceIndex(), sub_id, address, params));
}
