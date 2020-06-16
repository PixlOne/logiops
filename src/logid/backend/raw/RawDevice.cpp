#include "RawDevice.h"

#include <string>
#include <system_error>
#include <utility>


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

void RawDevice::sendReport(std::vector<uint8_t> report)
{
    _sendReport(std::move(report));
}

std::vector<uint8_t> RawDevice::readReport(std::size_t maxDataLength)
{
    return _readReport(maxDataLength);
}

void RawDevice::_sendReport(std::vector<uint8_t> report)
{
    std::lock_guard<std::mutex> lock(dev_io);
    int ret = ::write(fd, report.data(), report.size());
    if(ret == -1)
        throw std::system_error(errno, std::system_category(), "_sendReport write failed");
}

std::vector<uint8_t> RawDevice::_readReport(std::size_t maxDataLength)
{
    std::lock_guard<std::mutex> lock(dev_io);
    int ret;
    std::vector<uint8_t> report(maxDataLength);

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

    return report;
}

void RawDevice::interruptRead()
{
    char c = 0;
    if(-1 == write(dev_pipe[1], &c, sizeof(char)))
        throw std::system_error(errno, std::system_category(), "interruptRead write pipe failed");

    // Ensure I/O has halted
    std::lock_guard<std::mutex> lock(dev_io);
}