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

#include <backend/hidpp10/ReceiverMonitor.h>
#include <cassert>
#include <util/task.h>
#include <util/log.h>

using namespace logid::backend::hidpp10;
using namespace logid::backend::hidpp;

ReceiverMonitor::ReceiverMonitor(const std::string& path,
                                 const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout)
        : _receiver(std::make_shared<Receiver>(path, monitor, timeout)) {

    Receiver::NotificationFlags notification_flags{true, true, true};
    _receiver->setNotifications(notification_flags);
}

ReceiverMonitor::~ReceiverMonitor() {
    if (ev_handler.has_value())
        _receiver->rawDevice()->removeEventHandler(ev_handler.value());
}

void ReceiverMonitor::ready() {
    if (!ev_handler.has_value()) {
        hidpp::EventHandler event_handler;
        ev_handler = _receiver->rawDevice()->addEventHandler(
                {[](const std::vector<uint8_t>& report) -> bool {
                    if (report[Offset::Type] == Report::Type::Short ||
                        report[Offset::Type] == Report::Type::Long) {
                        uint8_t sub_id = report[Offset::SubID];
                        return (sub_id == Receiver::DeviceConnection ||
                                sub_id == Receiver::DeviceDisconnection);
                    }
                    return false;
                }, [this](const std::vector<uint8_t>& raw) -> void {
                    /* Running in a new thread prevents deadlocks since the
                     * receiver may be enumerating.
                     */
                    hidpp::Report report(raw);

                    spawn_task([this, report, path = this->_receiver->rawDevice()->rawPath()]() {
                        if (report.subId() == Receiver::DeviceConnection) {
                            try {
                                this->addDevice(this->_receiver->deviceConnectionEvent(report));
                            } catch (std::exception& e) {
                                logPrintf(ERROR, "Failed to add device %d to receiver on %s: %s",
                                          report.deviceIndex(), path.c_str(), e.what());
                            }
                        } else if (report.subId() == Receiver::DeviceDisconnection) {
                            try {
                                this->removeDevice(
                                        this->_receiver->deviceDisconnectionEvent(report));
                            } catch (std::exception& e) {
                                logPrintf(ERROR, "Failed to remove device %d from "
                                                 "receiver on %s: %s", report.deviceIndex(),
                                          path.c_str(), e.what());
                            }
                        }
                    });
                }
                });
    }

    enumerate();
}

void ReceiverMonitor::enumerate() {
    _receiver->enumerate();
}

void ReceiverMonitor::waitForDevice(hidpp::DeviceIndex index) {
    auto handler_id = std::make_shared<raw::RawDevice::EvHandlerId>();

    *handler_id = _receiver->rawDevice()->addEventHandler(
            {[index](const std::vector<uint8_t>& report) -> bool {
                return report[Offset::DeviceIndex] == index;
            },
             [this, index, handler_id]([[maybe_unused]] const std::vector<uint8_t>& report) {
                 hidpp::DeviceConnectionEvent event{};
                 event.withPayload = false;
                 event.linkEstablished = true;
                 event.index = index;
                 event.fromTimeoutCheck = true;

                 spawn_task([this, event, handler_id]() {
                     assert(handler_id);
                     try {
                         _receiver->rawDevice()->removeEventHandler(*handler_id);
                         addDevice(event);
                     } catch (std::exception& e) {
                         logPrintf(ERROR, "Failed to add device %d to receiver on %s: %s",
                                   event.index, _receiver->rawDevice()->rawPath().c_str(),
                                   e.what());
                     }
                 });
             }
            });
}

std::shared_ptr<Receiver> ReceiverMonitor::receiver() const {
    return _receiver;
}