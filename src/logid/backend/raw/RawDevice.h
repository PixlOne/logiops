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

        static std::vector<uint8_t> getReportDescriptor(std::string path);
        static std::vector<uint8_t> getReportDescriptor(int fd);
        std::vector<uint8_t> reportDescriptor() const;

        std::vector<uint8_t> sendReport(const std::vector<uint8_t>& report);
        void sendReportNoResponse(const std::vector<uint8_t>& report);
        void interruptRead(bool wait_for_halt=true);

        void listen();
        void listenAsync();
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
        std::vector<uint8_t> _rdesc;

        std::atomic<bool> _continue_listen;
        std::atomic<bool> _continue_respond;
        std::condition_variable _listen_condition;

        std::map<std::string, std::shared_ptr<RawEventHandler>>
            _event_handlers;
        std::mutex _event_handler_lock;
        void _handleEvent(std::vector<uint8_t>& report);

        /* These will only be used internally and processed with a queue */
        int _sendReport(const std::vector<uint8_t>& report);
        int _readReport(std::vector<uint8_t>& report, std::size_t maxDataLength);
        int _readReport(std::vector<uint8_t>& report, std::size_t maxDataLength,
                std::chrono::milliseconds timeout);

        std::vector<uint8_t> _respondToReport(const std::vector<uint8_t>&
                request);

        mutex_queue<std::shared_ptr<std::packaged_task<std::vector<uint8_t>()>>>
            _io_queue;
    };
}}}

#endif //LOGID_BACKEND_RAWDEVICE_H