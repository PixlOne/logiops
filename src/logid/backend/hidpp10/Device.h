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

#ifndef LOGID_BACKEND_HIDPP10_DEVICE_H
#define LOGID_BACKEND_HIDPP10_DEVICE_H

#include <optional>
#include <variant>
#include <backend/hidpp/Device.h>
#include <backend/hidpp10/Error.h>
#include <backend/hidpp10/defs.h>

namespace logid::backend::hidpp10 {
    class Device : public hidpp::Device {
    public:

        hidpp::Report sendReport(const hidpp::Report& report) final;

        std::vector<uint8_t> getRegister(uint8_t address,
                                         const std::vector<uint8_t>& params,
                                         hidpp::Report::Type type);

        std::vector<uint8_t> setRegister(uint8_t address,
                                         const std::vector<uint8_t>& params,
                                         hidpp::Report::Type type);

        void setRegisterNoResponse(uint8_t address, const std::vector<uint8_t>& params,
                                   hidpp::Report::Type type);

    protected:
        Device(const std::string& path, hidpp::DeviceIndex index,
               const std::shared_ptr<raw::DeviceMonitor>& monitor, double timeout);

        Device(std::shared_ptr<raw::RawDevice> raw_dev,
               hidpp::DeviceIndex index, double timeout);

        Device(const std::shared_ptr<hidpp10::Receiver>& receiver,
               hidpp::DeviceIndex index, double timeout);

        bool responseReport(const hidpp::Report& report) final;

    private:
        typedef std::variant<hidpp::Report, hidpp::Report::Hidpp10Error> Response;

        struct ResponseSlot {
            std::optional<Response> response;
            std::optional<uint8_t> sub_id;

            void reset();
        };

        std::array<ResponseSlot, SubIDCount> _responses;

        std::vector<uint8_t> accessRegister(
                uint8_t sub_id, uint8_t address, const std::vector<uint8_t>& params);

        void accessRegisterNoResponse(
                uint8_t sub_id, uint8_t address, const std::vector<uint8_t>& params);

    protected:
        template<typename T, typename... Args>
        static std::shared_ptr<T> makeDerived(Args... args) {
            auto device = hidpp::Device::makeDerived<T>(std::forward<Args>(args)...);

            if (std::get<0>(device->version()) != 1)
                throw std::invalid_argument("not a hid++ 1.0 device");

            return device;
        }

    public:
        template<typename... Args>
        static std::shared_ptr<Device> make(Args... args) {
            return makeDerived<Device>(std::forward<Args>(args)...);
        }
    };
}

#endif //LOGID_BACKEND_HIDPP10_DEVICE_H