/*
 * Copyright 2019-2020 PixlOne
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

#include "Device.h"
#include "../Error.h"
#include "../dj/Receiver.h"

using namespace logid::backend;
using namespace logid::backend::hidpp20;

Device::Device(const std::string& path, hidpp::DeviceIndex index,
               std::shared_ptr<raw::DeviceMonitor> monitor, double timeout) :
        hidpp::Device(path, index,
                      std::move(monitor), timeout) {
    // TODO: Fix version check
    if (std::get<0>(version()) < 2)
        throw std::runtime_error("Invalid HID++ version");
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device,
               hidpp::DeviceIndex index, double timeout) :
        hidpp::Device(std::move(raw_device), index, timeout) {
    if (std::get<0>(version()) < 2)
        throw std::runtime_error("Invalid HID++ version");
}

Device::Device(const std::shared_ptr<dj::Receiver>& receiver,
               hidpp::DeviceConnectionEvent event, double timeout) :
        hidpp::Device(receiver, event, timeout) {
    if (std::get<0>(version()) < 2)
        throw std::runtime_error("Invalid HID++ version");
}

Device::Device(const std::shared_ptr<dj::Receiver>& receiver,
               hidpp::DeviceIndex index, double timeout)
        : hidpp::Device(receiver, index, timeout) {
    if (std::get<0>(version()) < 2)
        throw std::runtime_error("Invalid HID++ version");
}

std::vector<uint8_t> Device::callFunction(uint8_t feature_index,
                                          uint8_t function, std::vector<uint8_t>& params) {
    hidpp::Report::Type type;

    assert(params.size() <= hidpp::LongParamLength);
    if (params.size() <= hidpp::ShortParamLength)
        type = hidpp::Report::Type::Short;
    else if (params.size() <= hidpp::LongParamLength)
        type = hidpp::Report::Type::Long;
    else
        throw hidpp::Report::InvalidReportID();

    hidpp::Report request(type, deviceIndex(), feature_index, function,
                          LOGID_HIDPP_SOFTWARE_ID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    auto response = this->sendReport(request);
    return {response.paramBegin(), response.paramEnd()};
}

void Device::callFunctionNoResponse(uint8_t feature_index, uint8_t function,
                                    std::vector<uint8_t>& params) {
    hidpp::Report::Type type;

    assert(params.size() <= hidpp::LongParamLength);
    if (params.size() <= hidpp::ShortParamLength)
        type = hidpp::Report::Type::Short;
    else if (params.size() <= hidpp::LongParamLength)
        type = hidpp::Report::Type::Long;
    else
        throw hidpp::Report::InvalidReportID();

    hidpp::Report request(type, deviceIndex(), feature_index, function,
                          LOGID_HIDPP_SOFTWARE_ID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    this->sendReportNoResponse(request);
}

hidpp::Report Device::sendReport(const hidpp::Report& report) {
    decltype(_responses)::iterator response_slot;

    while (true) {
        {
            std::lock_guard lock(_response_lock);
            if (_responses.empty()) {
                response_slot = _responses.emplace(
                        2, std::optional<Response>()).first;
                break;
            } else if (_responses.size() < response_slots) {
                uint8_t i = 0;
                for (auto& x: _responses) {
                    if (x.first != i + 1) {
                        ++i;
                        break;
                    }
                    i = x.first;
                }
                assert(_responses.count(i) == 0);

                response_slot = _responses.emplace(
                        i, std::optional<Response>()).first;
                break;
            }
        }

        std::unique_lock<std::mutex> lock(_response_wait_lock);
        _response_cv.wait(lock, [this, sub_id = report.subId()]() {
            std::lock_guard<std::mutex> lock(_response_lock);
            return _responses.size() < response_slots;
        });
    }

    {
        std::lock_guard<std::mutex> lock(_response_lock);
        hidpp::Report mod_report{report};
        mod_report.setSwId(response_slot->first);
        sendReportNoResponse(std::move(mod_report));
    }

    std::unique_lock<std::mutex> wait(_response_wait_lock);
    bool valid = _response_cv.wait_for(wait, io_timeout,
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
    uint8_t sw_id;

    bool is_error = false;
    hidpp::Report::Hidpp20Error hidpp20_error{};
    if (report.isError20(&hidpp20_error)) {
        is_error = true;
        sw_id = hidpp20_error.software_id;
    } else {
        sw_id = report.swId();
    }

    auto response_slot = _responses.find(sw_id);
    if (response_slot == _responses.end())
        return false;

    if (is_error) {
        response_slot->second = static_cast<Error::ErrorCode>(
                hidpp20_error.error_code);
    } else {
        response_slot->second = report;
    }

    _response_cv.notify_all();
    return true;
}
