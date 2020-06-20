#ifndef LOGID_BACKEND_DJ_RECEIVERMONITOR_H
#define LOGID_BACKEND_DJ_RECEIVERMONITOR_H

#include <cstdint>
#include <string>
#include "Receiver.h"
#include "../hidpp/defs.h"

namespace logid {
namespace backend {
namespace dj
{
    // This class will run on the RawDevice thread,
    class ReceiverMonitor
    {
    public:
        ReceiverMonitor(std::string path);

        void enumerate();
        void run();
        void stop();

    protected:
        virtual void addDevice(hidpp::DeviceIndex index, uint16_t pid) = 0;
        virtual void removeDevice(hidpp::DeviceIndex index) = 0;

        // Internal methods for derived class
        void _pair(uint8_t timeout = 0);
        void _stopPairing();

        void _unpair();
    private:
        Receiver _reciever;
    };

}}}

#endif //LOGID_BACKEND_DJ_RECEIVERMONITOR_H