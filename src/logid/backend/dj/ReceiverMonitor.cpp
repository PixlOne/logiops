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

#include "ReceiverMonitor.h"
#include "../../util/task.h"
#include "../../util/log.h"

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

    Receiver::NotificationFlags notification_flags{
        true,
        true,
        true};
   _receiver->enableHidppNotifications(notification_flags);
}

ReceiverMonitor::~ReceiverMonitor()
{
    this->stop();
}

void ReceiverMonitor::run()
{
    _receiver->listen();

    if(_receiver->hidppEventHandlers().find("RECVMON") ==
           _receiver->hidppEventHandlers().end()) {
        std::shared_ptr<hidpp::EventHandler> event_handler =
                std::make_shared<hidpp::EventHandler>();
        event_handler->condition = [](hidpp::Report &report) -> bool {
            return (report.subId() == Receiver::DeviceConnection ||
                    report.subId() == Receiver::DeviceDisconnection);
        };

        event_handler->callback = [this](hidpp::Report &report) -> void {
            /* Running in a new thread prevents deadlocks since the
             * receiver may be enumerating.
             */
            task::spawn({[this, report]() {
                if (report.subId() == Receiver::DeviceConnection)
                    this->addDevice(this->_receiver->deviceConnectionEvent
                    (report));
                else if (report.subId() == Receiver::DeviceDisconnection)
                    this->removeDevice(this->_receiver->
                            deviceDisconnectionEvent(report));
            }}, {[report, path=this->_receiver->rawDevice()->hidrawPath()]
            (std::exception& e) {
                if(report.subId() == Receiver::DeviceConnection)
                    logPrintf(ERROR, "Failed to add device %d to receiver "
                                      "on %s: %s", report.deviceIndex(),
                                      path.c_str(), e.what());
                else if(report.subId() == Receiver::DeviceDisconnection)
                    logPrintf(ERROR, "Failed to remove device %d from "
                                      "receiver on %s: %s", report.deviceIndex()
                                      ,path.c_str(), e.what());
            }});
        };

        _receiver->addHidppEventHandler("RECVMON", event_handler);
    }

    enumerate();
}

void ReceiverMonitor::stop()
{
    _receiver->removeHidppEventHandler("RECVMON");

    _receiver->stopListening();
}

void ReceiverMonitor::enumerate()
{
    _receiver->enumerateHidpp();
}

void ReceiverMonitor::waitForDevice(hidpp::DeviceIndex index)
{
    std::string nickname = "WAIT_DEV_" + std::to_string(index);
    auto handler = std::make_shared<raw::RawEventHandler>();
    handler->condition = [index](std::vector<uint8_t>& report)->bool {
        return report[Offset::DeviceIndex] == index;
    };

    handler->callback = [this, index, nickname](std::vector<uint8_t>& report) {
        (void)report; // Suppress unused warning

        hidpp::DeviceConnectionEvent event{};
        event.withPayload = false;
        event.linkEstablished = true;
        event.index = index;
        event.fromTimeoutCheck = true;

        task::spawn({[this, event, nickname]() {
                _receiver->rawDevice()->removeEventHandler(nickname);
                this->addDevice(event);
        }}, {[path=_receiver->rawDevice()->hidrawPath(), event]
        (std::exception& e) {
            logPrintf(ERROR, "Failed to add device %d to receiver "
                             "on %s: %s", event.index,
                             path.c_str(), e.what());
        }});
    };

    _receiver->rawDevice()->addEventHandler(nickname, handler);
}

std::shared_ptr<Receiver> ReceiverMonitor::receiver() const
{
    return _receiver;
}