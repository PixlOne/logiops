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
        _smartshift = hidpp20::SmartShift::autoVersion(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _torque_support = _smartshift->supportsTorque();
    _defaults = _smartshift->getDefaults();

    if (_config.get().has_value()) {
        auto& config = _config.get().value();

        if (config.threshold.has_value()) {
            auto& threshold = config.threshold.value();

            /* 0 means no change, clip to 1. */
            if (threshold == 0)
                threshold = 1;
        }

        if (config.torque.has_value()) {
            auto& torque = config.torque.value();
            /* torque is a percentage, clip between 1-100 */
            if (torque == 0)
                torque = 1;
            else if (torque > 100)
                torque = 100;
        }
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
        settings.setTorque = conf.torque.has_value();
        if (settings.setTorque)
            settings.torque = conf.torque.value();

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

const hidpp20::SmartShift::Defaults& SmartShift::getDefaults() const {
    return _defaults;
}

bool SmartShift::supportsTorque() const {
    return _torque_support;
}

SmartShift::IPC::IPC(SmartShift* parent) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".SmartShift", {
                        {"GetConfig",    {this, &IPC::getConfig,    {"active",    "threshold", "torque"}}},
                        {"SetActive",    {this, &IPC::setActive,    {"active",    "clear"}}},
                        {"SetThreshold", {this, &IPC::setThreshold, {"threshold", "clear"}}},
                        {"SetTorque",    {this, &IPC::setTorque,    {"torque",    "clear"}}},
                },
                {
                        {"TorqueSupport", ipcgull::property<bool>(
                                ipcgull::property_readable, parent->supportsTorque())},
                }, {}),
        _parent(*parent) {
}

std::tuple<uint8_t, uint8_t, uint8_t> SmartShift::IPC::getConfig() const {
    std::shared_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    auto& defaults = _parent.getDefaults();
    if (config.has_value()) {
        auto& conf_value = config.value();
        return {
                conf_value.on.has_value() ? (conf_value.on.value() ? 2 : 1) : 0,
                conf_value.threshold.value_or(defaults.autoDisengage),
                conf_value.torque.value_or(defaults.torque),
        };
    } else {
        return {0, 0, 0};
    }
}

void SmartShift::IPC::setActive(bool active, bool clear) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    if (clear) {
        if (config.has_value())
            config.value().on.reset();
    } else {
        if (!config.has_value())
            config = config::SmartShift{};
        config.value().on = active;
        Status status{};
        status.active = active, status.setActive = true;
        _parent.setStatus(status);
    }
}

void SmartShift::IPC::setThreshold(uint8_t threshold, bool clear) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    Status status{};
    status.setAutoDisengage = true;

    /* clip threshold */
    if (threshold == 0)
        threshold = 1;

    if (clear) {
        if (config.has_value())
            config.value().threshold.reset();
        status.autoDisengage = _parent.getDefaults().autoDisengage;
    } else {
        if (!config.has_value())
            config = config::SmartShift{};
        config.value().threshold = threshold;
        status.autoDisengage = threshold;
    }
    _parent.setStatus(status);
}

void SmartShift::IPC::setTorque(uint8_t torque, bool clear) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();
    Status status{};
    status.setTorque = true;

    /* clip torque */
    if (torque == 0)
        torque = 1;
    else if (torque > 100)
        torque = 100;

    if (!_parent.supportsTorque())
        throw std::invalid_argument("torque unsupported");

    if (clear) {
        if (config.has_value())
            config.value().torque.reset();
        status.torque = _parent.getDefaults().torque;
    } else {
        if (!config.has_value())
            config = config::SmartShift{};
        config.value().torque = torque;
        status.torque = torque;
    }
    _parent.setStatus(status);
}
