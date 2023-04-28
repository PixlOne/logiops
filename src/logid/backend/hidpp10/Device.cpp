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

#include <cassert>
#include <utility>
#include "Device.h"
#include "defs.h"
#include "../Error.h"

using namespace logid::backend;
using namespace logid::backend::hidpp10;

Device::Device(const std::string& path,
               hidpp::DeviceIndex index,
               std::shared_ptr<raw::DeviceMonitor> monitor, double timeout) :
        hidpp::Device(path, index, std::move(monitor), timeout) {
    assert(version() == std::make_tuple(1, 0));
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_dev,
               hidpp::DeviceIndex index,
               double timeout) : hidpp::Device(std::move(raw_dev), index, timeout) {
    assert(version() == std::make_tuple(1, 0));
}

Device::Device(const std::shared_ptr<dj::Receiver>& receiver,
               hidpp::DeviceIndex index,
               double timeout)
        : hidpp::Device(receiver, index, timeout) {
    assert(version() == std::make_tuple(1, 0));
}

hidpp::Report Device::sendReport(const hidpp::Report& report) {
    decltype(_responses)::iterator response_slot;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(_response_lock);
            response_slot = _responses.find(report.subId());
            if (response_slot == _responses.end()) {
                response_slot = _responses.emplace(
                        report.subId(), std::optional<Response>()).first;
                break;
            }
        }
        std::unique_lock<std::mutex> lock(_response_wait_lock);
        _response_cv.wait(lock, [this, sub_id = report.subId()]() {
            std::lock_guard<std::mutex> lock(_response_lock);
            return _responses.find(sub_id) != _responses.end();
        });
    }

    sendReportNoResponse(report);
    std::unique_lock<std::mutex> wait(_response_wait_lock);
    bool valid = _response_cv.wait_for(
            wait, io_timeout,
            [this, &response_slot]() {
                std::lock_guard<std::mutex> lock(_response_lock);
                return response_slot->second.has_value();
            });

    if (!valid) {
        std::lock_guard<std::mutex> lock(_response_lock);
        _responses.erase(response_slot);
        throw TimeoutError();
    }

    std::lock_guard<std::mutex> lock(_response_lock);
    assert(response_slot->second.has_value());
    auto response = response_slot->second.value();
    _responses.erase(response_slot);
    if (std::holds_alternative<hidpp::Report>(response))
        return std::get<hidpp::Report>(response);
    else // if(std::holds_alternative<Error::ErrorCode>(response))
        throw Error(std::get<Error::ErrorCode>(response));
}

bool Device::responseReport(const hidpp::Report& report) {
    std::lock_guard<std::mutex> lock(_response_lock);
    uint8_t sub_id;

    bool is_error = false;
    hidpp::Report::Hidpp10Error hidpp10_error{};
    if (report.isError10(&hidpp10_error)) {
        sub_id = hidpp10_error.sub_id;
        is_error = true;
    } else {
        sub_id = report.subId();
    }

    auto response_slot = _responses.find(sub_id);
    if (response_slot == _responses.end())
        return false;

    if (is_error) {
        response_slot->second = static_cast<Error::ErrorCode>(
                hidpp10_error.error_code);
    } else {
        response_slot->second = report;
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

std::vector<uint8_t> Device::accessRegister(uint8_t sub_id, uint8_t address,
                                            const std::vector<uint8_t>& params) {
    hidpp::Report::Type type = params.size() <= hidpp::ShortParamLength ?
                               hidpp::Report::Type::Short : hidpp::Report::Type::Long;

    hidpp::Report request(type, deviceIndex(), sub_id, address);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = sendReport(request);
    return {response.paramBegin(), response.paramEnd()};
}



