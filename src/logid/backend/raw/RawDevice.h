#ifndef LOGID_BACKEND_RAWDEVICE_H
#define LOGID_BACKEND_RAWDEVICE_H

#include <string>
#include <vector>
#include <mutex>

#define HIDPP_IO_TIMEOUT std::chrono::seconds(2)

namespace logid::backend::raw
{
    class RawDevice
    {
    public:
        RawDevice(std::string path);
        ~RawDevice();
        std::string hidrawPath() const { return path; }
        std::vector<uint8_t> reportDescriptor() const { return rdesc; }

        /// TODO: Process reports in a queue.
        void sendReport(std::vector<uint8_t> report);
        std::vector<uint8_t> readReport(std::size_t maxDataLength);

        void interruptRead();
    private:
        std::mutex dev_io;
        std::string path;
        int fd;
        int dev_pipe[2];
        uint16_t vid;
        uint16_t pid;
        std::string name;
        std::vector<uint8_t> rdesc;

        /* These will only be used internally and processed with a queue */
        void _sendReport(std::vector<uint8_t> report);
        std::vector<uint8_t> _readReport(std::size_t maxDataLength);
    };
}

#endif //LOGID_BACKEND_RAWDEVICE_H