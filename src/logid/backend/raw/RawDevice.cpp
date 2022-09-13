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
#include "../Error.h"
#include "../hidpp/defs.h"
#include "../dj/defs.h"
#include "../../util/log.h"
#include "../hidpp/Report.h"
#include "../../Configuration.h"
#include "../../util/thread.h"
#include "../../util/task.h"
#include "../../util/workqueue.h"

#include <string>
#include <system_error>
#include <utility>

#define MAX_DATA_LENGTH 32

extern "C"
{
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/hidraw.h>
}

using namespace logid::backend::raw;
using namespace logid::backend;
using namespace std::chrono;

bool RawDevice::supportedReport(uint8_t id, uint8_t length)
{
    switch(id) {
    case hidpp::ReportType::Short:
        return length == (hidpp::ShortParamLength +
            hidpp::Report::HeaderLength);
    case hidpp::ReportType::Long:
        return length == (hidpp::LongParamLength +
            hidpp::Report::HeaderLength);
    case dj::ReportType::Short:
        return length == (dj::ShortParamLength + dj::HeaderLength);
    case dj::ReportType::Long:
        return length == (dj::LongParamLength + dj::HeaderLength);
    default:
        return false;
    }
}

RawDevice::RawDevice(std::string path) : _path (std::move(path)),
    _continue_listen (false), _continue_respond (false)
{
    int ret;

    _fd = ::open(_path.c_str(), O_RDWR);
    if (_fd == -1)
        throw std::system_error(errno, std::system_category(),
                "RawDevice open failed");

    hidraw_devinfo devinfo{};
    if (-1 == ::ioctl(_fd, HIDIOCGRAWINFO, &devinfo)) {
        int err = errno;
        ::close(_fd);
        throw std::system_error(err, std::system_category(),
                "RawDevice HIDIOCGRAWINFO failed");
    }
    _vid = devinfo.vendor;
    _pid = devinfo.product;

    char name_buf[256];
    if (-1 == (ret = ::ioctl(_fd, HIDIOCGRAWNAME(sizeof(name_buf)), name_buf)
            )) {
        int err = errno;
        ::close(_fd);
        throw std::system_error(err, std::system_category(),
                "RawDevice HIDIOCGRAWNAME failed");
    }
    _name.assign(name_buf, ret - 1);

    _rdesc = getReportDescriptor(_fd);

    if (-1 == ::pipe(_pipe)) {
        int err = errno;
        close(_fd);
        throw std::system_error(err, std::system_category(),
                "RawDevice pipe open failed");
    }

    _continue_listen = false;
}

RawDevice::~RawDevice()
{
    if(_fd != -1)
    {
        ::close(_fd);
        ::close(_pipe[0]);
        ::close(_pipe[1]);
    }
}
std::string RawDevice::hidrawPath() const
{
    return _path;
}

std::string RawDevice::name() const
{
    return _name;
}

uint16_t RawDevice::vendorId() const
{
    return _vid;
}

uint16_t RawDevice::productId() const
{
    return _pid;
}

std::vector<uint8_t> RawDevice::getReportDescriptor(std::string path)
{
    int fd = ::open(path.c_str(), O_RDWR);
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

std::vector<uint8_t> RawDevice::reportDescriptor() const
{
    return _rdesc;
}

std::vector<uint8_t> RawDevice::sendReport(const std::vector<uint8_t>& report)
{
    /* If the listener will stop, handle I/O manually.
     * Otherwise, push to queue and wait for result. */
    if(_continue_listen) {
        std::mutex send_report;
        std::unique_lock<std::mutex> lock(send_report);
        std::condition_variable cv;
        bool top_of_queue = false;
        auto task = std::make_shared<std::packaged_task<std::vector<uint8_t>()>>
                ( [this, report, &cv, &top_of_queue] () {
            top_of_queue = true;
            cv.notify_all();
            return this->_respondToReport(report);
        });
        auto f = task->get_future();
        _io_queue.push(task);
        interruptRead(false); // Alert listener to prioritise
        cv.wait(lock, [&top_of_queue]{ return top_of_queue; });
        auto status = f.wait_for(global_config->ioTimeout());
        if(status == std::future_status::timeout) {
            _continue_respond = false;
            interruptRead();
            return f.get(); // Expecting an error, but it could work
        }
        return f.get();
    }
    else {
        std::vector<uint8_t> response;
        std::exception_ptr _exception;
        std::shared_ptr<task> t = std::make_shared<task>(
                [this, report, &response]() {
                    response = _respondToReport(report);
        }, [&_exception](std::exception& e) {
                    try {
                        throw e;
                    } catch(std::exception& e) {
                        _exception = std::make_exception_ptr(e);
                    }
                });
        global_workqueue->queue(t);
        t->waitStart();
        auto status = t->waitFor(global_config->ioTimeout());
        if(_exception)
            std::rethrow_exception(_exception);
        if(status == std::future_status::timeout) {
            _continue_respond = false;
            interruptRead();
            t->wait();
            if(_exception)
                std::rethrow_exception(_exception);
            throw TimeoutError();
        } else
            return response;
    }
}

// DJ commands are not systematically acknowledged, do not expect a result.
void RawDevice::sendReportNoResponse(const std::vector<uint8_t>& report)
{
    /* If the listener will stop, handle I/O manually.
     * Otherwise, push to queue and wait for result. */
    if(_continue_listen) {
        auto task = std::make_shared<std::packaged_task<std::vector<uint8_t>()>>
                ([this, report]() {
            this->_sendReport(report);
            return std::vector<uint8_t>();
        });
        auto f = task->get_future();
        _io_queue.push(task);
        f.get();
    }
    else
        _sendReport(report);
}

std::vector<uint8_t> RawDevice::_respondToReport
    (const std::vector<uint8_t>& request)
{
    _sendReport(request);
    _continue_respond = true;

    auto start_point = std::chrono::steady_clock::now();

    while(_continue_respond) {
        std::vector<uint8_t> response;
        auto current_point = std::chrono::steady_clock::now();
        auto timeout = global_config->ioTimeout() - std::chrono::duration_cast
                <std::chrono::milliseconds>(current_point - start_point);
        if(timeout.count() <= 0)
            throw TimeoutError();
        _readReport(response, MAX_DATA_LENGTH, timeout);

        if(!_continue_respond)
            throw TimeoutError();

        // All reports have the device index at byte 2
        if(response[1] != request[1]) {
            if(_continue_listen)
                this->_handleEvent(response);
            continue;
        }

        if(hidpp::ReportType::Short == request[0] ||
            hidpp::ReportType::Long == request[0]) {
            if(hidpp::ReportType::Short != response[0] &&
               hidpp::ReportType::Long != response[0]) {
                if(_continue_listen)
                    this->_handleEvent(response);
                continue;
            }

            // Error; leave to device to handle
            if(response[2] == 0x8f || response[2] == 0xff)
                return response;

            bool others_match = true;
            for(int i = 2; i < 4; i++)
                if(response[i] != request[i])
                    others_match = false;

            if(others_match)
                return response;
        } else if(dj::ReportType::Short == request[0] ||
            dj::ReportType::Long == request[0]) {
            //Error; leave to device ot handle
            if(0x7f == response[2])
                return response;
            else if(response[2] == request[2])
                return response;
        }

        if(_continue_listen)
            this->_handleEvent(response);
    }

    return {};
}

int RawDevice::_sendReport(const std::vector<uint8_t>& report)
{
    std::lock_guard<std::mutex> lock(_dev_io);
    if(logid::global_loglevel <= LogLevel::RAWREPORT) {
        printf("[RAWREPORT] %s OUT: ", _path.c_str());
        for(auto &i : report)
            printf("%02x ", i);
        printf("\n");
    }

    assert(supportedReport(report[0], report.size()));

    int ret = ::write(_fd, report.data(), report.size());
    if(ret == -1) {
        ///TODO: This seems like a hacky solution
        // Try again before failing
        ret = ::write(_fd, report.data(), report.size());
        if(ret == -1)
            throw std::system_error(errno, std::system_category(),
                    "_sendReport write failed");
    }

    return ret;
}

int RawDevice::_readReport(std::vector<uint8_t> &report,
                           std::size_t maxDataLength)
{
    return _readReport(report, maxDataLength, global_config->ioTimeout());
}

int RawDevice::_readReport(std::vector<uint8_t> &report,
                           std::size_t maxDataLength, std::chrono::milliseconds timeout)
{
    std::lock_guard<std::mutex> lock(_dev_io);
    int ret;
    report.resize(maxDataLength);

    timeval timeout_tv{};
    timeout_tv.tv_sec = duration_cast<seconds>(global_config->ioTimeout())
            .count();
    timeout_tv.tv_usec = duration_cast<microseconds>(
            global_config->ioTimeout()).count() %
            duration_cast<microseconds>(seconds(1)).count();

    auto timeout_ms = duration_cast<milliseconds>(timeout).count();

    fd_set fds;
    do {
        FD_ZERO(&fds);
        FD_SET(_fd, &fds);
        FD_SET(_pipe[0], &fds);

        ret = select(std::max(_fd, _pipe[0]) + 1,
                     &fds, nullptr, nullptr,
                     (timeout_ms > 0 ? nullptr : &timeout_tv));
    } while(ret == -1 && errno == EINTR);

    if(ret == -1)
        throw std::system_error(errno, std::system_category(),
                "_readReport select failed");

    if(FD_ISSET(_fd, &fds)) {
        ret = read(_fd, report.data(), report.size());
        if(ret == -1)
            throw std::system_error(errno, std::system_category(),
                    "_readReport read failed");
        report.resize(ret);
    }

    if(FD_ISSET(_pipe[0], &fds)) {
        char c;
        ret = read(_pipe[0], &c, sizeof(char));
        if(ret == -1)
            throw std::system_error(errno, std::system_category(),
                    "_readReport read pipe failed");
    }

    if(0 == ret)
        throw backend::TimeoutError();

    if(logid::global_loglevel <= LogLevel::RAWREPORT) {
        printf("[RAWREPORT] %s IN:  ", _path.c_str());
        for(auto &i : report)
            printf("%02x ", i);
        printf("\n");
    }

    return ret;
}

void RawDevice::interruptRead(bool wait_for_halt)
{
    char c = 0;
    if(-1 == write(_pipe[1], &c, sizeof(char)))
        throw std::system_error(errno, std::system_category(),
                "interruptRead write pipe failed");

    // Ensure I/O has halted
    if(wait_for_halt)
        std::lock_guard<std::mutex> lock(_dev_io);
}

void RawDevice::listen()
{
    std::lock_guard<std::mutex> lock(_listening);
    _continue_listen = true;
    _listen_condition.notify_all();
    while(_continue_listen) {
        while(!_io_queue.empty()) {
            auto task = _io_queue.front();
            (*task)();
            _io_queue.pop();
        }
        std::vector<uint8_t> report;
        _readReport(report, MAX_DATA_LENGTH);

        this->_handleEvent(report);
    }

    // Listener is stopped, handle I/O queue
    while(!_io_queue.empty()) {
        auto task = _io_queue.front();
        (*task)();
        _io_queue.pop();
    }

    _continue_listen = false;
}

void RawDevice::listenAsync()
{
    std::mutex listen_check;
    std::unique_lock<std::mutex> check_lock(listen_check);
    thread::spawn({[this]() { listen(); }});

    // Block until RawDevice is listening
    _listen_condition.wait(check_lock, [this](){
        return (bool)_continue_listen;
    });
}

void RawDevice::stopListener()
{
    _continue_listen = false;
    interruptRead();
}

void RawDevice::addEventHandler(const std::string& nickname,
        const std::shared_ptr<raw::RawEventHandler>& handler)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    assert(_event_handlers.find(nickname) == _event_handlers.end());
    assert(handler);
    _event_handlers.emplace(nickname, handler);
}

void RawDevice::removeEventHandler(const std::string &nickname)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    _event_handlers.erase(nickname);
}

const std::map<std::string, std::shared_ptr<raw::RawEventHandler>>&
RawDevice::eventHandlers()
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    return _event_handlers;
}

void RawDevice::_handleEvent(std::vector<uint8_t> &report)
{
    std::unique_lock<std::mutex> lock(_event_handler_lock);
    for(auto& handler : _event_handlers)
        if(handler.second->condition(report))
            handler.second->callback(report);
}

bool RawDevice::isListening()
{
    bool ret = _listening.try_lock();

    if(ret)
        _listening.unlock();

    return !ret;
}
