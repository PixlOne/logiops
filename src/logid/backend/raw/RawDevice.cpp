#include "RawDevice.h"
#include "../Error.h"

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
using namespace std::chrono;

RawDevice::RawDevice(std::string path) : path (path)
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
    std::packaged_task<std::vector<uint8_t>()> task(
            [=]() {
        _sendReport(report);
        std::vector<uint8_t> response;
        _readReport(response, MAX_DATA_LENGTH);
        return response;
    });

    /* If the listener will stop, handle I/O manually.
     * Otherwise, push to queue and wait for result. */
    if(continue_listen)
    {
        auto f = task.get_future();
        write_queue.push(&task);
        return f.get();
    }
    else
        return task.get_future().get();
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

void RawDevice::addEventHandler(const std::string &nickname, RawEventHandler &handler)
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
