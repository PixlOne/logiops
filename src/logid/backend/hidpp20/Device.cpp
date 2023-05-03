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
#include <backend/hidpp20/Device.h>
#include <backend/Error.h>
#include <backend/hidpp10/Receiver.h>

using namespace logid::backend;
using namespace logid::backend::hidpp20;

Device::Device(const std::string& path, hidpp::DeviceIndex index,
               const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout) :
        hidpp::Device(path, index, monitor, timeout) {
}

Device::Device(std::shared_ptr<raw::RawDevice> raw_device,
               hidpp::DeviceIndex index, double timeout) :
        hidpp::Device(std::move(raw_device), index, timeout) {
}

Device::Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceConnectionEvent event, double timeout) :
        hidpp::Device(receiver, event, timeout) {
}

Device::Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceIndex index, double timeout)
        : hidpp::Device(receiver, index, timeout) {
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
                          hidpp::softwareID);
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

    hidpp::Report request(type, deviceIndex(), feature_index, function, hidpp::softwareID);
    std::copy(params.begin(), params.end(), request.paramBegin());

    this->sendReportNoACK(request);
}

hidpp::Report Device::sendReport(const hidpp::Report& report) {
    auto& response_slot = _responses[report.feature() % _responses.size()];

    std::unique_lock<std::mutex> response_lock(_response_mutex);
    _response_cv.wait(response_lock, [&response_slot]() {
        return !response_slot.feature.has_value();
    });

    response_slot.feature = report.feature();

    _sendReport(report);

    bool valid = _response_cv.wait_for(
            response_lock, io_timeout,
            [&response_slot]() {
                return response_slot.response.has_value();
            });

    if (!valid) {
        response_slot.reset();
        throw TimeoutError();
    }

    assert(response_slot.response.has_value());
    auto response = response_slot.response.value();
    response_slot.reset();

    if (std::holds_alternative<hidpp::Report>(response)) {
        return std::get<hidpp::Report>(response);
    } else { // if(std::holds_alternative<Error::ErrorCode>(response))
        auto error = std::get<hidpp::Report::Hidpp20Error>(response);
        throw Error(error.error_code, error.device_index);
    }
}

void Device::sendReportNoACK(const hidpp::Report& report) {
    hidpp::Report no_ack_report(report);
    no_ack_report.setSwId(hidpp::noAckSoftwareID);
    _sendReport(std::move(no_ack_report));
}

bool Device::responseReport(const hidpp::Report& report) {
    auto& response_slot = _responses[report.feature() % _responses.size()];
    std::lock_guard<std::mutex> lock(_response_mutex);
    uint8_t sw_id, feature;

    bool is_error = false;
    hidpp::Report::Hidpp20Error hidpp20_error{};
    if (report.isError20(hidpp20_error)) {
        is_error = true;
        sw_id = hidpp20_error.software_id;
        feature = hidpp20_error.feature_index;
    } else {
        sw_id = report.swId();
        feature = report.feature();
    }

    if (sw_id != hidpp::softwareID)
        return false;

    if (!response_slot.feature || response_slot.feature.value() != feature) {
        return false;
    }

    if (is_error) {
        response_slot.response = hidpp20_error;
    } else {
        response_slot.response = report;
    }

    _response_cv.notify_all();
    return true;
}

void Device::ResponseSlot::reset() {
    response.reset();
    feature.reset();
}
