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
#include <backend/hidpp20/features/ChangeHost.h>
#include <backend/hidpp20/Device.h>

using namespace logid::backend::hidpp20;

ChangeHost::ChangeHost(Device* dev) : Feature(dev, ID), _host_count(0) {
}

ChangeHost::HostInfo ChangeHost::getHostInfo() {
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetHostInfo, params);

    HostInfo info{};
    info.hostCount = response[0];
    info.currentHost = response[1];
    info.enhancedHostSwitch = response[2] & 1;

    if (!_host_count)
        _host_count = info.hostCount;

    return info;
}

void ChangeHost::setHost(uint8_t host) {
    /* Expect connection to be severed here, send without response
     *
     * Since there is no response, we have to emulate any kind of
     * error that may be returned (i.e. InvalidArgument as per the docs)
     */
    if (!_host_count)
        getHostInfo();

    if (host >= _host_count)
        throw Error(hidpp20::Error::InvalidArgument, _device->deviceIndex());

    std::vector<uint8_t> params = {host};

    callFunctionNoResponse(SetCurrentHost, params);
}

[[maybe_unused]]
std::vector<uint8_t> ChangeHost::getCookies() {
    if (!_host_count)
        getHostInfo();

    std::vector<uint8_t> params(0);
    auto response = callFunction(GetCookies, params);

    response.resize(_host_count);

    return response;
}

[[maybe_unused]]
void ChangeHost::setCookie(uint8_t host, uint8_t cookie) {
    std::vector<uint8_t> params = {host, cookie};
    callFunction(SetCookie, params);
}