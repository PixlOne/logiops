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
#include "ChangeHostAction.h"
#include "../Device.h"
#include "../backend/hidpp20/features/ReprogControls.h"
#include "../util/task.h"

using namespace logid::actions;
using namespace logid::backend;

ChangeHostAction::ChangeHostAction(Device *device, libconfig::Setting&
config) : Action(device), _config (device, config)
{
    try {
        _change_host = std::make_shared<hidpp20::ChangeHost>(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        logPrintf(WARN, "%s:%d: ChangeHost feature not supported, "
                        "ChangeHostAction will not work.", device->hidpp20()
                        .devicePath().c_str(), device->hidpp20().deviceIndex());
    }
}

void ChangeHostAction::press()
{
    // Do nothing, wait until release
}

void ChangeHostAction::release()
{
    if(_change_host) {
        task::spawn([this] {
            auto host_info = _change_host->getHostInfo();
            auto next_host = _config.nextHost(host_info);
            if(next_host != host_info.currentHost)
                _change_host->setHost(next_host);
        });
    }
}

uint8_t ChangeHostAction::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}

ChangeHostAction::Config::Config(Device *device, libconfig::Setting& config)
    : Action::Config(device)
{
    try {
        auto& host = config["host"];
        if(host.getType() == libconfig::Setting::TypeInt) {
            _offset = false;
            _host = host;
            _host--; // hosts are one-indexed in config

            if(_host < 0) {
                logPrintf(WARN, "Line %d: host must be positive.",
                        host.getSourceLine());
                _offset = true;
                _host = 0;
            }

        } else if(host.getType() == libconfig::Setting::TypeString) {
            _offset = true;
            std::string hostmode = host;
            std::transform(hostmode.begin(), hostmode.end(),
                    hostmode.begin(), ::tolower);

            if(hostmode == "next")
                _host = 1;
            else if(hostmode == "prev" || hostmode == "previous")
                _host = -1;
            else {
                logPrintf(WARN, "Line %d: host must equal an integer, 'next',"
                                "or 'prev'.", host.getSourceLine());
                _host = 0;
            }
        } else {
            logPrintf(WARN, "Line %d: host must equal an integer, 'next',"
                            "or 'prev'.", host.getSourceLine());
            _offset = true;
            _host = 0;
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: host is a required field, skipping.",
                config.getSourceLine());
        _offset = true;
        _host = 0;
    }
}

uint8_t ChangeHostAction::Config::nextHost(hidpp20::ChangeHost::HostInfo info)
{
    if(_offset) {
        return (info.currentHost + _host) % info.hostCount;
    } else {
        if(_host >= info.hostCount || _host < 0) {
            logPrintf(WARN, "No such host %d, defaulting to current.",
                    _host+1);
            return info.currentHost;
        } else
            return _host;
    }
}
