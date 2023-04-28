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
#include "ChangeDPI.h"
#include "../Device.h"
#include "../util/task.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;

const char* ChangeDPI::interface_name = "ChangeDPI";

ChangeDPI::ChangeDPI(
        Device* device, config::ChangeDPI& config,
        [[maybe_unused]] const std::shared_ptr<ipcgull::node>& parent) :
        Action(device, interface_name, {
                {
                        {"GetConfig", {this, &ChangeDPI::getConfig, {"change", "sensor"}}},
                        {"SetChange", {this, &ChangeDPI::setChange, {"change"}}},
                        {"SetSensor", {this, &ChangeDPI::setSensor, {"sensor", "reset"}}},
                },
                {},
                {}}), _config(config) {
    _dpi = _device->getFeature<features::DPI>("dpi");
    if (!_dpi)
        logPrintf(WARN, "%s:%d: DPI feature not found, cannot use "
                        "ChangeDPI action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().deviceIndex());
}

std::tuple<int16_t, uint16_t> ChangeDPI::getConfig() const {
    return {_config.inc.value_or(0), _config.sensor.value_or(0)};
}

void ChangeDPI::setChange(int16_t change) {
    _config.inc = change;
}

void ChangeDPI::setSensor(uint8_t sensor, bool reset) {
    if (reset) {
        _config.sensor.reset();
    } else {
        _config.sensor = sensor;
    }
}

void ChangeDPI::press() {
    _pressed = true;
    if (_dpi && _config.inc.has_value()) {
        spawn_task(
                [this] {
                    try {
                        uint16_t last_dpi = _dpi->getDPI(_config.sensor.value_or(0));
                        _dpi->setDPI(last_dpi + _config.inc.value(),
                                     _config.sensor.value_or(0));
                    } catch (backend::hidpp20::Error& e) {
                        if (e.code() == backend::hidpp20::Error::InvalidArgument)
                            logPrintf(WARN, "%s:%d: Could not get/set DPI for sensor "
                                            "%d",
                                      _device->hidpp20().devicePath().c_str(),
                                      _device->hidpp20().deviceIndex(),
                                      _config.sensor.value_or(0));
                        else
                            throw e;
                    }
                });
    }
}

void ChangeDPI::release() {
    _pressed = false;
}

uint8_t ChangeDPI::reprogFlags() const {
    return backend::hidpp20::ReprogControls::TemporaryDiverted;
}
