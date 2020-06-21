#include "ReceiverMonitor.h"

#include <utility>
#include <cassert>

using namespace logid::backend::dj;

ReceiverMonitor::ReceiverMonitor(std::string path) : _receiver (
        std::make_shared<Receiver>(std::move(path)))
{
    assert(_receiver->hidppEventHandlers().find("RECVMON") ==
        _receiver->hidppEventHandlers().end());
    assert(_receiver->djEventHandlers().find("RECVMON") ==
           _receiver->djEventHandlers().end());

    Receiver::notification_flags notification_flags{
        true,
        true,
        true};
   _receiver->enableHidppNotifications(notification_flags);
}

void ReceiverMonitor::run()
{
    _receiver->listen();

    if(_receiver->hidppEventHandlers().find("RECVMON") ==
           _receiver->hidppEventHandlers().end())
    {
        std::shared_ptr<hidpp::EventHandler> eventHandler =
                std::make_shared<hidpp::EventHandler>();
        eventHandler->condition = [](hidpp::Report &report) -> bool {
            return (report.subId() == Receiver::DeviceConnection ||
                    report.subId() == Receiver::DeviceDisconnection);
        };

        eventHandler->callback = [this](hidpp::Report &report) -> void {
            /* Running in a new thread prevents deadlocks since the
             * receiver may be enumerating.
             */
            std::thread{[this](hidpp::Report report) {
                if (report.subId() == Receiver::DeviceConnection)
                    this->addDevice(this->_receiver->deviceConnectionEvent(
                            report));
                else if (report.subId() == Receiver::DeviceDisconnection)
                    this->removeDevice(this->_receiver->
                            deviceDisconnectionEvent(report));
            }, report}.detach();
        };

        _receiver->addHidppEventHandler("RECVMON", eventHandler);
    }

    enumerate();
}

void ReceiverMonitor::stop()
{
    _receiver->stopListening();
}

void ReceiverMonitor::enumerate()
{
    _receiver->enumerateHidpp();
}

std::shared_ptr<Receiver> ReceiverMonitor::receiver() const
{
    return _receiver;
}