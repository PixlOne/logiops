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
#include <algorithm>
#include <cmath>
#include "DPI.h"
#include "../Device.h"
#include "../util/log.h"

using namespace logid::features;
using namespace logid::backend;

uint16_t getClosestDPI(hidpp20::AdjustableDPI::SensorDPIList& dpi_list,
        uint16_t dpi)
{
    if(dpi_list.isRange) {
        const uint16_t min = *std::min_element(dpi_list.dpis.begin(),
                dpi_list.dpis.end());
        const uint16_t max = *std::max_element(dpi_list.dpis.begin(),
                dpi_list.dpis.end());
        if(!((dpi-min) % dpi_list.dpiStep) && dpi >= min && dpi <= max)
            return dpi;
        else if(dpi > max)
            return max;
        else if(dpi < min)
            return min;
        else
            return min + round((double)(dpi-min)/dpi_list.dpiStep)*dpi_list
            .dpiStep;
    } else {
        if(std::find(dpi_list.dpis.begin(), dpi_list.dpis.end(), dpi)
           != dpi_list.dpis.end())
            return dpi;
        else {
            auto it = std::min_element(dpi_list.dpis.begin(), dpi_list.dpis
                    .end(), [dpi](uint16_t a, uint16_t b) {
                return (dpi - a) < (dpi - b);
            });
            if(it == dpi_list.dpis.end())
                return 0;
            else
                return *it;
        }
    }
}

DPI::DPI(Device* device) : DeviceFeature(device), _config (device)
{
    try {
        _adjustable_dpi = std::make_shared<hidpp20::AdjustableDPI>
                (&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }
}

void DPI::configure()
{
    const uint8_t sensors = _adjustable_dpi->getSensorCount();
    for(uint8_t i = 0; i < _config.getSensorCount(); i++) {
        hidpp20::AdjustableDPI::SensorDPIList dpi_list;
        if(_dpi_lists.size() <= i) {
            dpi_list = _adjustable_dpi->getSensorDPIList(i);
            _dpi_lists.push_back(dpi_list);
        } else {
            dpi_list = _dpi_lists[i];
        }
        if(i < sensors) {
            auto dpi = _config.getDPI(i);
            if(dpi) {
                _adjustable_dpi->setSensorDPI(i, getClosestDPI(dpi_list,
                        dpi));
            }
        }
    }
}

void DPI::listen()
{
}

uint16_t DPI::getDPI(uint8_t sensor)
{
    return _adjustable_dpi->getSensorDPI(sensor);
}

void DPI::setDPI(uint16_t dpi, uint8_t sensor)
{
    hidpp20::AdjustableDPI::SensorDPIList dpi_list;
    if(_dpi_lists.size() <= sensor) {
        dpi_list = _adjustable_dpi->getSensorDPIList(sensor);
        for(std::size_t i = _dpi_lists.size(); i < sensor; i++) {
            _dpi_lists.push_back(_adjustable_dpi->getSensorDPIList(i));
        }
        _dpi_lists.push_back(dpi_list);
    }
    dpi_list = _dpi_lists[sensor];
    _adjustable_dpi->setSensorDPI(sensor, getClosestDPI(dpi_list, dpi));
}

/* Some devices have multiple sensors, but an older config format
 * only supports a single DPI. The dpi setting can be an array or
 * an integer.
 */
DPI::Config::Config(Device *dev) : DeviceFeature::Config(dev)
{
    try {
        auto& config_root = dev->config().getSetting("dpi");
        if(config_root.isNumber()) {
            int dpi = config_root;
            _dpis.push_back(dpi);
        } else if(config_root.isArray()) {
            for(int i = 0; i < config_root.getLength(); i++)
                _dpis.push_back((int)config_root[i]);
        } else {
            logPrintf(WARN, "Line %d: dpi is improperly formatted",
                    config_root.getSourceLine());
        }
    } catch(libconfig::SettingNotFoundException& e) {
        // DPI not configured, use default
    }
}

uint8_t DPI::Config::getSensorCount()
{
    return _dpis.size();
}

uint16_t DPI::Config::getDPI(uint8_t sensor)
{
    return _dpis[sensor];
}
