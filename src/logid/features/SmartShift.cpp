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
#include <features/SmartShift.h>
#include <Device.h>
#include <ipc_defs.h>

using namespace logid::features;
using namespace logid::backend;

SmartShift::SmartShift(Device* device) : DeviceFeature(device),
                                         _config(device->activeProfile().smartshift) {
    try {
        _smartshift = std::make_shared<hidpp20::SmartShift>(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _ipc_interface = _device->ipcNode()->make_interface<IPC>(this);
}

void SmartShift::configure() {
    std::shared_lock lock(_config_mutex);
    auto& config = _config.get();
    if (config.has_value()) {
        const auto& conf = config.value();
        Status settings{};
        settings.setActive = conf.on.has_value();
        if (settings.setActive)
            settings.active = conf.on.value();
        settings.setAutoDisengage = conf.threshold.has_value();
        if (settings.setAutoDisengage)
            settings.autoDisengage = conf.threshold.value();

        _smartshift->setStatus(settings);
    }
}

void SmartShift::listen() {
}

void SmartShift::setProfile(config::Profile& profile) {
    std::unique_lock lock(_config_mutex);
    _config = profile.smartshift;
}

SmartShift::Status SmartShift::getStatus() const {
    return _smartshift->getStatus();
}

void SmartShift::setStatus(Status status) {
    _smartshift->setStatus(status);
}

SmartShift::IPC::IPC(SmartShift* parent) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".SmartShift", {
                        {"GetStatus",             {this, &IPC::getStatus,           {"active",    "threshold"}}},
                        {"SetActive",             {this, &IPC::setActive,           {"active"}}},
                        {"SetThreshold",          {this, &IPC::setThreshold,        {"threshold"}}},
                        {"GetDefault",            {this, &IPC::getDefault,          {"setActive", "active", "setThreshold", "threshold"}}},
                        {"ClearDefaultActive",    {this, &IPC::clearDefaultActive}},
                        {"SetDefaultActive",      {this, &IPC::setDefaultActive,    {"active"}}},
                        {"ClearDefaultThreshold", {this, &IPC::clearDefaultThreshold}},
                        {"SetDefaultThreshold",   {this, &IPC::setDefaultThreshold, {"threshold"}}}
                }, {}, {}),
        _parent(*parent) {
}

std::tuple<bool, uint8_t> SmartShift::IPC::getStatus() const {
    auto ret = _parent.getStatus();
    return std::make_tuple(ret.active, ret.autoDisengage);
}

void SmartShift::IPC::setActive(bool active) {
    Status status{};
    status.setActive = true;
    status.active = active;
    _parent.setStatus(status);
}

void SmartShift::IPC::setThreshold(uint8_t threshold) {
    Status status{};
    status.setAutoDisengage = true;
    status.autoDisengage = threshold;
    _parent.setStatus(status);
}

std::tuple<bool, bool, bool, uint8_t> SmartShift::IPC::getDefault() const {
    std::shared_lock lock(_parent._config_mutex);

    auto& config = _parent._config.get();

    if (!config.has_value())
        return {false, false, false, 0};

    std::tuple<bool, bool, bool, uint8_t> ret;
    std::get<0>(ret) = config.value().on.has_value();
    if (std::get<0>(ret))
        std::get<1>(ret) = config.value().on.value();
    std::get<2>(ret) = config.value().threshold.has_value();
    if (std::get<2>(ret))
        std::get<3>(ret) = config.value().threshold.value();

    return ret;
}

void SmartShift::IPC::clearDefaultActive() {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    if (config.has_value())
        config.value().on.reset();
}

void SmartShift::IPC::setDefaultActive(bool active) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    if (!config.has_value())
        config = config::SmartShift{};
    config.value().on = active;
}


void SmartShift::IPC::clearDefaultThreshold() {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    if (config.has_value())
        config.value().threshold.reset();
}

void SmartShift::IPC::setDefaultThreshold(uint8_t threshold) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    if (!config.has_value())
        config = config::SmartShift{};
    config.value().threshold = threshold;
}
