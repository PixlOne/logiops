/*
 * Copyright 2019-2023 PixlOne
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

#include <backend/dj/ReceiverMonitor.h>
#include <utility>
#include <cassert>
#include <util/task.h>
#include <util/log.h>

using namespace logid::backend::dj;

ReceiverMonitor::ReceiverMonitor(std::string path,
                                 const std::shared_ptr<raw::DeviceMonitor>& monitor,
                                 double timeout) :
        _receiver(std::make_shared<Receiver>(
                std::move(path), monitor, timeout)) {
    assert(!_receiver->hidppEventHandlers().contains(ev_handler_name));
    assert(!_receiver->djEventHandlers().contains(ev_handler_name));

    Receiver::NotificationFlags notification_flags{
            true,
            true,
            true};
    _receiver->enableHidppNotifications(notification_flags);
}

ReceiverMonitor::~ReceiverMonitor() {
    _receiver->removeHidppEventHandler(ev_handler_name);
}

void ReceiverMonitor::ready() {
    if (!_receiver->hidppEventHandlers().contains(ev_handler_name)) {
        std::shared_ptr<hidpp::EventHandler> event_handler =
                std::make_shared<hidpp::EventHandler>();
        event_handler->condition = [](hidpp::Report& report) -> bool {
            return (report.subId() == Receiver::DeviceConnection ||
                    report.subId() == Receiver::DeviceDisconnection);
        };

        event_handler->callback = [this](hidpp::Report& report) -> void {
            /* Running in a new thread prevents deadlocks since the
             * receiver may be enumerating.
             */
            spawn_task([this, report,
                               path = this->_receiver->rawDevice()->rawPath()]() {
                if (report.subId() == Receiver::DeviceConnection) {
                    try {
                        this->addDevice(this->_receiver->deviceConnectionEvent
                                (report));
                    } catch (std::exception& e) {
                        logPrintf(ERROR, "Failed to add device %d to receiver "
                                         "on %s: %s", report.deviceIndex(),
                                  path.c_str(), e.what());
                    }
                } else if (report.subId() == Receiver::DeviceDisconnection) {
                    try {
                        this->removeDevice(this->_receiver->
                                deviceDisconnectionEvent(report));
                    } catch (std::exception& e) {
                        logPrintf(ERROR, "Failed to remove device %d from "
                                         "receiver on %s: %s", report.deviceIndex(),
                                  path.c_str(), e.what());
                    }
                }
            });
        };

        _receiver->addHidppEventHandler(ev_handler_name, event_handler);
    }

    enumerate();
}

void ReceiverMonitor::enumerate() {
    _receiver->enumerateHidpp();
}

void ReceiverMonitor::waitForDevice(hidpp::DeviceIndex index) {
    auto handler_id = std::make_shared<raw::RawDevice::EvHandlerId>();

    *handler_id = _receiver->rawDevice()->addEventHandler(
            {
                    [index](const std::vector<uint8_t>& report) -> bool {
                        return report[Offset::DeviceIndex] ==
                               index;
                    },
                    [this, index, handler_id](
                            [[maybe_unused]] const std::vector<uint8_t>& report) {
                        hidpp::DeviceConnectionEvent event{};
                        event.withPayload = false;
                        event.linkEstablished = true;
                        event.index = index;
                        event.fromTimeoutCheck = true;

                        spawn_task(
                                [this, event, handler_id]() {
                                    assert(handler_id);
                                    try {
                                        _receiver->rawDevice()->removeEventHandler(
                                                *handler_id);
                                        addDevice(
                                                event);
                                    } catch (
                                            std::exception& e) {
                                        logPrintf(
                                                ERROR,
                                                "Failed to add device %d to receiver "
                                                "on %s: %s",
                                                event.index,
                                                _receiver->rawDevice()->rawPath().c_str(),
                                                e.what());
                                    }
                                });
                    }
            });
}

std::shared_ptr<Receiver> ReceiverMonitor::receiver() const {
    return _receiver;
}