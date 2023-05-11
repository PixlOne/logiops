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

#include <Configuration.h>
#include <util/log.h>
#include <utility>
#include <filesystem>
#include <ipc_defs.h>

using namespace logid;
using namespace libconfig;
using namespace logid::config;

Configuration::Configuration(std::string config_file) :
        _config_file(std::move(config_file)) {
    if (std::filesystem::exists(_config_file)) {
        try {
            _config.readFile(_config_file.c_str());
        } catch (const FileIOException& e) {
            logPrintf(ERROR, "I/O Error while reading %s: %s", _config_file.c_str(),
                  e.what());
            throw;
        } catch (const ParseException& e) {
            logPrintf(ERROR, "Parse error in %s, line %d: %s", e.getFile(),
                  e.getLine(), e.getError());
            throw;
        }

        Config::operator=(get<Config>(_config.getRoot()));
    } else {
        logPrintf(INFO, "Config file does not exist, using empty config.");
    }

    if (!devices.has_value())
        devices.emplace();
}

Configuration::Configuration() {
    devices.emplace();
}

void Configuration::save() {
    config::set(_config.getRoot(), *this);
    try {
        _config.writeFile(_config_file.c_str());
    } catch (const FileIOException& e) {
        logPrintf(ERROR, "I/O Error while writing %s: %s",
                  _config_file.c_str(), e.what());
        throw;
    } catch (const std::exception& e) {
        logPrintf(ERROR, "Error while writing %s: %s",
                  _config_file.c_str(), e.what());
        throw;
    }
}

Configuration::IPC::IPC(Configuration* config) :
        ipcgull::interface(SERVICE_ROOT_NAME ".Config", {
                {"Save", {config, &Configuration::save}}
        }, {}, {}) {
}
