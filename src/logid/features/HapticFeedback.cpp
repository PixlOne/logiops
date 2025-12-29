/*
 * Copyright 2025 Kristóf Marussy
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
#include "config/schema.h"
#include "features/DeviceFeature.h"
#include "ipcgull/interface.h"
#include <cstdint>
#include <features/HapticFeedback.h>
#include <Device.h>
#include <ipc_defs.h>
#include <algorithm>
#include <mutex>
#include <shared_mutex>

using namespace logid::features;
using namespace logid::backend;

static const bool kDefaultEnabled = true;
static const uint8_t kDefaultStrength = 60;
static const int kMinStrength = 1;
static const int kMaxStrength = 100;
static const bool kDefaultBatterySaving = false;
static const uint8_t kMaxEffect = 14;

static uint8_t clampStrength(int strength) {
    return std::max(kMinStrength, std::min(strength, kMaxStrength));
}

HapticFeedback::HapticFeedback(Device* device) : DeviceFeature(device), _config(device->activeProfile().haptic_feedback) {
    try {
        _haptic_feedback = std::make_shared<hidpp20::HapticFeedback>(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _ipc_interface = _device->ipcNode()->make_interface<IPC>(this);
}

void HapticFeedback::configure() {
    std::shared_lock lock(_config_mutex);

    bool enabled = kDefaultEnabled;
    uint8_t strength = kDefaultStrength;
    int battery_saving = kDefaultBatterySaving;
    if (_config.get().has_value()) {
        const auto& config = _config.get().value();
        if (config.enabled.has_value()) {
            enabled = config.enabled.value();
        }
        if (config.strength.has_value()) {
            strength = clampStrength(config.strength.value());
        }
        if (config.battery_saving.has_value()) {
            battery_saving = config.battery_saving.value();
        }
    }
    setStrength(strength, enabled, battery_saving);
}

void HapticFeedback::listen() {
}

void HapticFeedback::setProfile(config::Profile& profile) {
    std::unique_lock lock(_config_mutex);
    _config = profile.haptic_feedback;
}

void HapticFeedback::setStrength(uint8_t strength, bool enabled, bool battery_saving) {
    _haptic_feedback->setStrength(clampStrength(strength), enabled, battery_saving);
}

void HapticFeedback::playEffect(uint8_t effect) {
    if (_isEnabled() && effect <= kMaxEffect) {
        _haptic_feedback->playEffect(effect);
    }
}

bool HapticFeedback::_isEnabled() const {
    std::shared_lock lock(_config_mutex);
    if (!_config.get().has_value()) {
        return kDefaultEnabled;
    }
    return _config.get().value().enabled.value_or(kDefaultEnabled);
}

HapticFeedback::IPC::IPC(HapticFeedback* parent) : ipcgull::interface(
        SERVICE_ROOT_NAME ".HapticFeedback", {
            {"GetEnabled", {this, &IPC::getEnabled, {"enabled"}}},
            {"SetEnabled", {this, &IPC::setEnabled, {"enabled"}}},
            {"GetStrength", {this, &IPC::getStrength, {"strength"}}},
            {"SetStrength", {this, &IPC::setStrength, {"strength"}}},
            {"GetBatterySaving", {this, &IPC::getBatterySaving, {"battery_saving"}}},
            {"SetBatterySaving", {this, &IPC::setBatterySaving, {"battery_saving"}}},
            {"PlayEffect", {this, &IPC::playEffect, {"effect"}}}
        }, {}, {}), _parent(*parent) {
}

bool HapticFeedback::IPC::getEnabled() const {
    return _parent._isEnabled();
}

void HapticFeedback::IPC::setEnabled(bool enabled) {
    std::shared_lock lock(_parent._config_mutex);
    if (!_parent._config.get().has_value()) {
        _parent._config.get().emplace();
    }
    _parent._config.get().value().enabled.emplace(enabled);
    _parent.configure();
}

uint8_t HapticFeedback::IPC::getStrength() const {
    std::unique_lock lock(_parent._config_mutex);
    if (!_parent._config.get().has_value()) {
        return kDefaultStrength;
    }
    return _parent._config.get().value().strength.value_or(kDefaultStrength);
}

void HapticFeedback::IPC::setStrength(uint8_t strength) {
    std::shared_lock lock(_parent._config_mutex);
    if (!_parent._config.get().has_value()) {
        _parent._config.get().emplace();
    }
    _parent._config.get().value().strength.emplace(strength);
    _parent.configure();
}

bool HapticFeedback::IPC::getBatterySaving() const {
    std::unique_lock lock(_parent._config_mutex);
    if (!_parent._config.get().has_value()) {
        return kDefaultBatterySaving;
    }
    return _parent._config.get().value().battery_saving.value_or(kDefaultBatterySaving);
}

void HapticFeedback::IPC::setBatterySaving(bool battery_saving) {
    std::shared_lock lock(_parent._config_mutex);
    if (!_parent._config.get().has_value()) {
        _parent._config.get().emplace();
    }
    _parent._config.get().value().battery_saving.emplace(battery_saving);
    _parent.configure();
}

void HapticFeedback::IPC::playEffect(uint8_t effect) {
    _parent.playEffect(effect);
}
