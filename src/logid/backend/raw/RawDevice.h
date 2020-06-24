#ifndef LOGID_BACKEND_RAWDEVICE_H
#define LOGID_BACKEND_RAWDEVICE_H

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>
#include <future>
#include <set>

#include "defs.h"
#include "../../util/mutex_queue.h"

#define HIDPP_IO_TIMEOUT std::chrono::seconds(2)

namespace logid {
namespace backend {
namespace raw
{
    class RawDevice
    {
    public:
        static bool supportedReport(uint8_t id, uint8_t length);

        explicit RawDevice(std::string path);
        ~RawDevice();
        std::string hidrawPath() const;

        std::string name() const;
        uint16_t vendorId() const;
        uint16_t productId() const;

        std::vector<uint8_t> reportDescriptor() const { return rdesc; }

        std::vector<uint8_t> sendReport(const std::vector<uint8_t>& report);
        void sendReportNoResponse(const std::vector<uint8_t>& report);
        void interruptRead();

        void listen();
        void stopListener();
        bool isListening();

        void addEventHandler(const std::string& nickname,
                const std::shared_ptr<RawEventHandler>& handler);
        void removeEventHandler(const std::string& nickname);
        const std::map<std::string, std::shared_ptr<RawEventHandler>>&
            eventHandlers();

    private:
        std::mutex _dev_io, _listening;
        std::string _path;
        int _fd;
        int _pipe[2];
        uint16_t _vid;
        uint16_t _pid;
        std::string _name;
        std::vector<uint8_t> rdesc;

        std::atomic<bool> _continue_listen;

        std::map<std::string, std::shared_ptr<RawEventHandler>>
            _event_handlers;
        void _handleEvent(std::vector<uint8_t>& report);

        /* These will only be used internally and processed with a queue */
        int _sendReport(const std::vector<uint8_t>& report);
        int _readReport(std::vector<uint8_t>& report, std::size_t maxDataLength);

        std::vector<uint8_t> _respondToReport(const std::vector<uint8_t>&
                request);

        mutex_queue<std::packaged_task<std::vector<uint8_t>()>*> _io_queue;
    };
}}}

#endif //LOGID_BACKEND_RAWDEVICE_H