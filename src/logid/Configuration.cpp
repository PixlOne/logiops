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

#include <utility>
#include <vector>
#include <map>

#include "Configuration.h"
#include "util/log.h"

using namespace logid;
using namespace libconfig;

Configuration::Configuration(const std::string& config_file)
{
    try {
        _config.readFile(config_file.c_str());
    } catch(const FileIOException &e) {
        logPrintf(ERROR, "I/O Error while reading %s: %s", config_file.c_str(),
                e.what());
        throw e;
    } catch(const ParseException &e) {
        logPrintf(ERROR, "Parse error in %s, line %d: %s", e.getFile(),
                e.getLine(), e.getError());
        throw e;
    }

    const Setting &root = _config.getRoot();
    Setting* devices;

    try { devices = &root["devices"]; }
    catch(const SettingNotFoundException &e)
    {
        logPrintf(WARN, "No devices listed in config file.");
        return;
    }

    for(int i = 0; i < devices->getLength(); i++)
    {
        const Setting &device = (*devices)[i];
        std::string name;
        try {
            if(!device.lookupValue("name", name)) {
                logPrintf(WARN, "Line %d: 'name' must be a string, skipping "
                                "device.", device["name"].getSourceLine());
                continue;
            }
        } catch(SettingNotFoundException &e) {
            logPrintf(WARN, "Line %d: Missing 'name' field, skipping device."
                , device.getSourceLine());
            continue;
        }
        _device_paths.insert({name, device.getPath()});
    }
}

libconfig::Setting& Configuration::getSetting(std::string path)
{
    return _config.lookup(path);
}

std::string Configuration::getDevice(std::string name)
{
    auto it = _device_paths.find(name);
    if(it == _device_paths.end())
        throw DeviceNotFound(name);
    else
        return it->second;
}

Configuration::DeviceNotFound::DeviceNotFound(std::string name) :
    _name (std::move(name))
{
}

const char * Configuration::DeviceNotFound::what()
{
    return _name.c_str();
}
