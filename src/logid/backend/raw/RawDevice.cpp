#include "RawDevice.h"
#include "../Error.h"
#include "../hidpp/defs.h"
#include "../dj/defs.h"

#include <string>
#include <system_error>
#include <cassert>

#define MAX_DATA_LENGTH 32

extern "C"
{
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
}

using namespace logid::backend::raw;
using namespace logid::backend;
using namespace std::chrono;

bool RawDevice::supportedReportID(uint8_t id)
{
    return (hidpp::ReportType::Short == id) || (hidpp::ReportType::Long == id) ||
           (dj::ReportType::Short == id) || (dj::ReportType::Long == id);
}

RawDevice::RawDevice(std::string path) : path (path), continue_listen (false)
{
    int ret;

    fd = ::open(path.c_str(), O_RDWR);
    if (fd == -1)
        throw std::system_error(errno, std::system_category(), "RawDevice open failed");

    hidraw_devinfo devinfo{};
    if (-1 == ::ioctl(fd, HIDIOCGRAWINFO, &devinfo))
    {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(), "RawDevice HIDIOCGRAWINFO failed");
    }
    vid = devinfo.vendor;
    pid = devinfo.product;

    char name_buf[256];
    if (-1 == (ret = ::ioctl(fd, HIDIOCGRAWNAME(sizeof(name_buf)), name_buf)))
    {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(), "RawDevice HIDIOCGRAWNAME failed");
    }
    name.assign(name_buf, ret - 1);

    hidraw_report_descriptor _rdesc{};
    if (-1 == ::ioctl(fd, HIDIOCGRDESCSIZE, &_rdesc.size))
    {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(), "RawDevice HIDIOCGRDESCSIZE failed");
    }
    if (-1 == ::ioctl(fd, HIDIOCGRDESC, &_rdesc))
    {
        int err = errno;
        ::close(fd);
        throw std::system_error(err, std::system_category(), "RawDevice HIDIOCGRDESC failed");
    }
    rdesc = std::vector<uint8_t>(_rdesc.value, _rdesc.value + _rdesc.size);

    if (-1 == ::pipe(dev_pipe))
    {
        int err = errno;
        close(fd);
        throw std::system_error(err, std::system_category(), "RawDevice pipe open failed");
    }
}

RawDevice::~RawDevice()
{
    if(fd != -1)
    {
        ::close(fd);
        ::close(dev_pipe[0]);
        ::close(dev_pipe[1]);
    }
}

std::vector<uint8_t> RawDevice::sendReport(const std::vector<uint8_t>& report)
{
    assert(supportedReportID(report[0]));

        /* If the listener will stop, handle I/O manually.
     * Otherwise, push to queue and wait for result. */
    if(continue_listen)
    {
        std::packaged_task<std::vector<uint8_t>()> task( [this, report]() {
            return this->_respondToReport(report);
        });
        auto f = task.get_future();
        write_queue.push(&task);
        return f.get();
    }
    else
        return _respondToReport(report);
}

std::vector<uint8_t> RawDevice::_respondToReport
    (const std::vector<uint8_t>& request)
{
    _sendReport(request);
    while(true)
    {
        std::vector<uint8_t> response;
        _readReport(response, MAX_DATA_LENGTH);

        // All reports have the device index at byte 2
        if(response[1] != request[1])
        {
            std::thread([this](std::vector<uint8_t> report) {
                this->handleEvent(report);
            }, request).detach();
            continue;
        }

        if(hidpp::ReportType::Short == request[0] ||
            hidpp::ReportType::Long == request[0])
        {
            if(hidpp::ReportType::Short != response[0] &&
               hidpp::ReportType::Long != response[0])
            {
                std::thread([this](std::vector<uint8_t> report) {
                    this->handleEvent(report);
                }, request).detach();
                continue;
            }

            // Error; leave to device to handle
            if(response[2] == 0x8f || response[2] == 0xff)
                return response;

            bool others_match = true;
            for(int i = 2; i < 4; i++)
            {
                if(response[i] != request[i])
                    others_match = false;
            }

            if(others_match)
                return response;
        }
        else if(dj::ReportType::Short == request[0] ||
            dj::ReportType::Long == request[0])
        {
            //Error; leave to device ot handle
            if(0x7f == response[2])
                return response;
            else if(response[2] == request[2])
                return response;
        }

        std::thread([this](std::vector<uint8_t> report) {
            this->handleEvent(report);
        }, request).detach();
    }
}

int RawDevice::_sendReport(const std::vector<uint8_t>& report)
{
    std::lock_guard<std::mutex> lock(dev_io);
    int ret = ::write(fd, report.data(), report.size());
    if(ret == -1)
        throw std::system_error(errno, std::system_category(), "_sendReport write failed");

    return ret;
}

int RawDevice::_readReport(std::vector<uint8_t>& report, std::size_t maxDataLength)
{
    std::lock_guard<std::mutex> lock(dev_io);
    int ret;
    report.resize(maxDataLength);

    timeval timeout = { duration_cast<milliseconds>(HIDPP_IO_TIMEOUT).count(),
                       duration_cast<microseconds>(HIDPP_IO_TIMEOUT).count() };

    fd_set fds;
    do {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(dev_pipe[0], &fds);

        ret = select(std::max(fd, dev_pipe[0]) + 1,
                     &fds, nullptr, nullptr,
                     (HIDPP_IO_TIMEOUT.count() > 0 ? nullptr : &timeout));
    } while(ret == -1 && errno == EINTR);

    if(ret == -1)
        throw std::system_error(errno, std::system_category(), "_readReport select failed");

    if(FD_ISSET(fd, &fds))
    {
        ret = read(fd, report.data(), report.size());
        if(ret == -1)
            throw std::system_error(errno, std::system_category(), "_readReport read failed");
        report.resize(ret);
    }

    if(FD_ISSET(dev_pipe[0], &fds))
    {
        char c;
        ret = read(dev_pipe[0], &c, sizeof(char));
        if(ret == -1)
            throw std::system_error(errno, std::system_category(), "_readReport read pipe failed");
    }

    if(0 == ret)
        throw backend::TimeoutError();

    return ret;
}

void RawDevice::interruptRead()
{
    char c = 0;
    if(-1 == write(dev_pipe[1], &c, sizeof(char)))
        throw std::system_error(errno, std::system_category(), "interruptRead write pipe failed");

    // Ensure I/O has halted
    std::lock_guard<std::mutex> lock(dev_io);
}

void RawDevice::listen()
{
    std::lock_guard<std::mutex> lock(listening);

    continue_listen = true;
    while(continue_listen)
    {
        while(!write_queue.empty())
        {
            auto task = write_queue.front();
            (*task)();
            write_queue.pop();
        }
        std::vector<uint8_t> report;
        _readReport(report, MAX_DATA_LENGTH);
        std::thread([this](std::vector<uint8_t> report) {
            this->handleEvent(report);
        }, report).detach();
    }

    continue_listen = false;
}

void RawDevice::stopListener()
{
    continue_listen = false;
    interruptRead();
}

void RawDevice::addEventHandler(const std::string& nickname, RawEventHandler& handler)
{
    auto it = event_handlers.find(nickname);
    assert(it == event_handlers.end());
    event_handlers.emplace(nickname, handler);
}

void RawDevice::removeEventHandler(const std::string &nickname)
{
    event_handlers.erase(nickname);
}

void RawDevice::handleEvent(std::vector<uint8_t> &report)
{
    for(auto& handler : event_handlers)
        if(handler.second.condition(report))
            handler.second.callback(report);
}

bool RawDevice::isListening()
{
    bool ret = listening.try_lock();

    if(ret)
        listening.unlock();

    return ret;
}
