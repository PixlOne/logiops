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
#include <features/DPI.h>
#include <Device.h>
#include <algorithm>
#include <cmath>
#include <ipc_defs.h>

using namespace logid::features;
using namespace logid::backend;

uint16_t getClosestDPI(const hidpp20::AdjustableDPI::SensorDPIList& dpi_list,
                       uint16_t dpi) {
    if (dpi_list.isRange) {
        const uint16_t min = *std::min_element(dpi_list.dpis.begin(), dpi_list.dpis.end());
        const uint16_t max = *std::max_element(dpi_list.dpis.begin(), dpi_list.dpis.end());
        if (!((dpi - min) % dpi_list.dpiStep) && dpi >= min && dpi <= max)
            return dpi;
        else if (dpi > max)
            return max;
        else if (dpi < min)
            return min;
        else
            return (uint16_t) (min + round((double) (dpi - min) / dpi_list.dpiStep) *
                                     dpi_list.dpiStep);
    } else {
        if (std::find(dpi_list.dpis.begin(), dpi_list.dpis.end(), dpi)
            != dpi_list.dpis.end())
            return dpi;
        else {
            auto it = std::min_element(dpi_list.dpis.begin(), dpi_list.dpis.end(),
                                       [dpi](uint16_t a, uint16_t b) {
                                           return (dpi - a) < (dpi - b);
                                       });
            if (it == dpi_list.dpis.end())
                return 0;
            else
                return *it;
        }
    }
}

DPI::DPI(Device* device) : DeviceFeature(device), _config(device->activeProfile().dpi) {
    try {
        _adjustable_dpi = std::make_shared<hidpp20::AdjustableDPI>
                (&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _ipc_interface = _device->ipcNode()->make_interface<IPC>(this);
}

void DPI::configure() {
    std::shared_lock lock(_config_mutex);

    if (_config.get().has_value()) {
        const auto& config = _config.get().value();
        if (std::holds_alternative<int>(config)) {
            const auto& dpi = std::get<int>(config);
            _fillDPILists(0);
            std::shared_lock dpi_lock(_dpi_list_mutex);
            if (dpi != 0) {
                _adjustable_dpi->setSensorDPI(0, getClosestDPI(_dpi_lists.at(0), dpi));
            }
        } else {
            const auto& dpis = std::get<std::list<int>>(config);
            int i = 0;
            _fillDPILists(dpis.size() - 1);
            std::shared_lock dpi_lock(_dpi_list_mutex);
            for (const auto& dpi: dpis) {
                if (dpi != 0) {
                    _adjustable_dpi->setSensorDPI(i, getClosestDPI(_dpi_lists.at(i), dpi));
                    ++i;
                }
            }
        }
    }
}

void DPI::listen() {
}

void DPI::setProfile(config::Profile& profile) {
    std::unique_lock lock(_config_mutex);
    _config = profile.dpi;
}

uint16_t DPI::getDPI(uint8_t sensor) {
    return _adjustable_dpi->getSensorDPI(sensor);
}

void DPI::setDPI(uint16_t dpi, uint8_t sensor) {
    if (dpi == 0)
        return;
    _fillDPILists(sensor);
    std::shared_lock lock(_dpi_list_mutex);
    auto dpi_list = _dpi_lists.at(sensor);
    _adjustable_dpi->setSensorDPI(sensor, getClosestDPI(dpi_list, dpi));
}

void DPI::_fillDPILists(uint8_t sensor) {
    bool needs_fill;
    {
        std::shared_lock lock(_dpi_list_mutex);
        needs_fill = _dpi_lists.size() <= sensor;
    }
    if (needs_fill) {
        std::unique_lock lock(_dpi_list_mutex);
        for (std::size_t i = _dpi_lists.size(); i <= sensor; i++) {
            _dpi_lists.push_back(_adjustable_dpi->getSensorDPIList(i));
        }
    }
}

DPI::IPC::IPC(DPI* parent) : ipcgull::interface(
        SERVICE_ROOT_NAME ".DPI", {
                {"GetSensors", {this, &IPC::getSensors, {"sensors"}}},
                {"GetDPIs", {this, &IPC::getDPIs, {"sensor"}, {"dpis", "dpiStep", "range"}}},
                {"GetDPI", {this, &IPC::getDPI, {"sensor"}, {"dpi"}}},
                {"SetDPI", {this, &IPC::setDPI, {"dpi", "sensor"}}}
        }, {}, {}), _parent(*parent) {
}

uint8_t DPI::IPC::getSensors() const {
    return _parent._dpi_lists.size();
}

std::tuple<std::vector<uint16_t>, uint16_t, bool> DPI::IPC::getDPIs(uint8_t sensor) const {
    _parent._fillDPILists(sensor);
    std::shared_lock lock(_parent._dpi_list_mutex);
    auto& dpi_list = _parent._dpi_lists.at(sensor);
    return {dpi_list.dpis, dpi_list.dpiStep, dpi_list.isRange};
}

uint16_t DPI::IPC::getDPI(uint8_t sensor) const {
    std::shared_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();

    if (!config.has_value())
        return _parent.getDPI(sensor);

    if (std::holds_alternative<int>(config.value())) {
        if (sensor == 0)
            return std::get<int>(config.value());
        else
            return _parent.getDPI(sensor);
    }

    const auto& list = std::get<std::list<int>>(config.value());

    if (list.size() > sensor) {
        auto it = list.begin();
        std::advance(it, sensor);
        return *it;
    } else {
        return _parent.getDPI(sensor);
    }
}

void DPI::IPC::setDPI(uint16_t dpi, uint8_t sensor) {
    std::unique_lock lock(_parent._config_mutex);
    auto& config = _parent._config.get();

    if (!config.has_value())
        config.emplace(std::list<int>());

    if (std::holds_alternative<int>(config.value())) {
        if (sensor == 0) {
            config.value() = dpi;
        } else {
            auto list = std::list<int>(sensor + 1, 0);
            *list.rbegin() = dpi;
            *list.begin() = dpi;
            config.value() = list;
        }
    } else {
        auto& list = std::get<std::list<int>>(config.value());

        while (list.size() <= sensor) {
            list.emplace_back(0);
        }

        if (list.size() == (size_t) (sensor + 1)) {
            *list.rbegin() = dpi;
        } else {
            auto it = list.begin();
            std::advance(it, sensor);
            *it = dpi;
        }
    }

    _parent.setDPI(dpi, sensor);
}
