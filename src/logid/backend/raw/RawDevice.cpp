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

#include "RawDevice.h"
#include "DeviceMonitor.h"
#include "IOMonitor.h"
#include "../Error.h"
#include "../../util/log.h"

#include <string>
#include <system_error>
#include <utility>

extern "C"
{
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
}

using namespace logid::backend::raw;
using namespace logid::backend;
using namespace std::chrono;

int get_fd(const std::string& path)
{
    int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
    if(fd == -1)
        throw std::system_error(errno, std::system_category(),
                                "RawDevice open failed");

    return fd;
}

RawDevice::dev_info get_devinfo(int fd)
{
    hidraw_devinfo devinfo{};
    if (-1 == ::ioctl(fd, HIDIOCGRAWINFO, &devinfo)) {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(),
                                "RawDevice HIDIOCGRAWINFO failed");
    }

    return {devinfo.vendor, devinfo.product};
}

std::string get_name(int fd)
{
    ssize_t len;
    char name_buf[256];
    if (-1 == (len = ::ioctl(fd, HIDIOCGRAWNAME(sizeof(name_buf)), name_buf))) {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(),
                                "RawDevice HIDIOCGRAWNAME failed");
    }
    return {name_buf, static_cast<size_t>(len)};
}

RawDevice::RawDevice(std::string path,
                     std::shared_ptr<DeviceMonitor> monitor) :
                     _valid (true),
                     _path (std::move(path)),
                     _fd (get_fd(_path)),
                     _devinfo (get_devinfo(_fd)),
                     _name (get_name(_fd)),
                     _rdesc (getReportDescriptor(_fd)),
                     _io_monitor (monitor->ioMonitor())
{
    _io_monitor->add(_fd, {
        [this]() { _readReports(); },
        [this]() { _valid = false; },
        [this]() { _valid = false; }
    });
}

RawDevice::~RawDevice() noexcept
{
    _io_monitor->remove(_fd);
    ::close(_fd);
}

const std::string& RawDevice::rawPath() const
{
    return _path;
}

const std::string& RawDevice::name() const
{
    return _name;
}

int16_t RawDevice::vendorId() const
{
    return _devinfo.vid;
}

int16_t RawDevice::productId() const
{
    return _devinfo.pid;
}

std::vector<uint8_t> RawDevice::getReportDescriptor(std::string path)
{
    int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1)
        throw std::system_error(errno, std::system_category(),
                                "open failed");

    auto rdesc = getReportDescriptor(fd);
    ::close(fd);
    return rdesc;
}

std::vector<uint8_t> RawDevice::getReportDescriptor(int fd)
{
    hidraw_report_descriptor rdesc{};
    if (-1 == ::ioctl(fd, HIDIOCGRDESCSIZE, &rdesc.size)) {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(),
                                "RawDevice HIDIOCGRDESCSIZE failed");
    }
    if (-1 == ::ioctl(fd, HIDIOCGRDESC, &rdesc)) {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(),
                                "RawDevice HIDIOCGRDESC failed");
    }
    return std::vector<uint8_t>(rdesc.value, rdesc.value + rdesc.size);
}

const std::vector<uint8_t>& RawDevice::reportDescriptor() const
{
    return _rdesc;
}

void RawDevice::sendReport(const std::vector<uint8_t>& report)
{
    if(!_valid) {
        // We could throw an error here, but this will likely be closed soon.
        return;
    }

    if(logid::global_loglevel <= LogLevel::RAWREPORT) {
        printf("[RAWREPORT] %s OUT: ", _path.c_str());
        for(auto &i : report)
            printf("%02x ", i);
        printf("\n");
    }

    if(write(_fd, report.data(), report.size()) == -1)
        throw std::system_error(errno, std::system_category(),
                                "sendReport write failed");
}

RawDevice::EvHandlerId RawDevice::addEventHandler(RawEventHandler handler)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    _event_handlers.emplace_front(std::move(handler));
    return _event_handlers.cbegin();
}

void RawDevice::removeEventHandler(RawDevice::EvHandlerId id)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    _event_handlers.erase(id);
}

void RawDevice::_readReports()
{
    uint8_t buf[max_data_length];
    ssize_t len;

    while(-1 != (len = ::read(_fd, buf, max_data_length))) {
        assert(len <= max_data_length);
        std::vector<uint8_t> report(buf, buf + len);

        if(logid::global_loglevel <= LogLevel::RAWREPORT) {
            printf("[RAWREPORT] %s IN:  ", _path.c_str());
            for(auto &i : report)
                printf("%02x ", i);
            printf("\n");
        }

        _handleEvent(report);
    }
}

void RawDevice::_handleEvent(const std::vector<uint8_t>& report)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    for(auto& handler : _event_handlers)
        if(handler.condition(report))
            handler.callback(report);
}
