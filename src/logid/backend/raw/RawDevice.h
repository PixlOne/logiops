#ifndef LOGID_BACKEND_RAWDEVICE_H
#define LOGID_BACKEND_RAWDEVICE_H

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>
#include <future>
#include <set>

#include "../defs.h"
#include "../../util/mutex_queue.h"

#define HIDPP_IO_TIMEOUT std::chrono::seconds(2)

namespace logid {
namespace backend {
namespace raw
{
    class RawDevice
    {
    public:
        static bool supportedReportID(uint8_t id);

        RawDevice(std::string path);
        ~RawDevice();
        std::string hidrawPath() const { return path; }
        std::vector<uint8_t> reportDescriptor() const { return rdesc; }

        std::vector<uint8_t> sendReport(const std::vector<uint8_t>& report);
        void interruptRead();

        void listen();
        void stopListener();
        bool isListening();

        void addEventHandler(const std::string& nickname, RawEventHandler& handler);
        void removeEventHandler(const std::string& nickname);

    private:
        std::mutex dev_io, listening;
        std::string path;
        int fd;
        int dev_pipe[2];
        uint16_t vid;
        uint16_t pid;
        std::string name;
        std::vector<uint8_t> rdesc;

        std::atomic<bool> continue_listen;

        std::map<std::string, backend::RawEventHandler> event_handlers;
        void handleEvent(std::vector<uint8_t>& report);

        /* These will only be used internally and processed with a queue */
        int _sendReport(const std::vector<uint8_t>& report);
        int _readReport(std::vector<uint8_t>& report, std::size_t maxDataLength);

        std::vector<uint8_t> _respondToReport(const std::vector<uint8_t>& request);

        mutex_queue<std::packaged_task<std::vector<uint8_t>()>*> write_queue;
    };
}}}

#endif //LOGID_BACKEND_RAWDEVICE_H