#include "ReceiverMonitor.h"

#include <utility>

using namespace logid::backend::dj;

ReceiverMonitor::ReceiverMonitor(std::string path) : _reciever (std::move(path))
{
    Receiver::notification_flags notification_flags{
        true,
        true,
        true};
   _reciever.enableHidppNotifications(notification_flags);
}

void ReceiverMonitor::run()
{
    _reciever.listen();
    enumerate();
}

void ReceiverMonitor::stop()
{
    _reciever.stopListening();
}

void ReceiverMonitor::enumerate()
{
    _reciever.enumerate();
}