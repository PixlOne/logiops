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

#ifndef LOGID_BACKEND_HIDPP_REPORT_H
#define LOGID_BACKEND_HIDPP_REPORT_H

#include <backend/raw/RawDevice.h>
#include <backend/hidpp/defs.h>
#include <cstdint>

namespace logid::backend::hidpp {
    uint8_t getSupportedReports(const std::vector<uint8_t>& report_desc);

    /* Some devices only support a subset of these reports */
    static constexpr uint8_t ShortReportSupported = 1U;
    static constexpr uint8_t LongReportSupported = (1U<<1);
    /* Very long reports exist, however they have not been encountered so far */

    namespace Offset {
        static constexpr uint8_t Type = 0;
        static constexpr uint8_t DeviceIndex = 1;
        static constexpr uint8_t SubID = 2;
        static constexpr uint8_t Feature = 2;
        static constexpr uint8_t Address = 3;
        static constexpr uint8_t Function = 3;
        static constexpr uint8_t Parameters = 4;
    }

    class Report {
    public:
        typedef ReportType::ReportType Type;

        class InvalidReportID : public std::exception {
        public:
            InvalidReportID() = default;

            [[nodiscard]] const char* what() const noexcept override;
        };

        class InvalidReportLength : public std::exception {
        public:
            InvalidReportLength() = default;

            [[nodiscard]] const char* what() const noexcept override;
        };

        static constexpr std::size_t MaxDataLength = 20;

        Report(Report::Type type, DeviceIndex device_index,
               uint8_t sub_id,
               uint8_t address);

        Report(Report::Type type, DeviceIndex device_index,
               uint8_t feature_index,
               uint8_t function,
               uint8_t sw_id);

        explicit Report(const std::vector<uint8_t>& data);

        [[nodiscard]] Report::Type type() const;

        void setType(Report::Type type);

        [[nodiscard]] DeviceIndex deviceIndex() const;

        [[maybe_unused]] void setDeviceIndex(DeviceIndex index);

        [[nodiscard]] uint8_t feature() const;

        [[maybe_unused]] void setFeature(uint8_t feature);

        [[nodiscard]] uint8_t subId() const;

        [[maybe_unused]] void setSubId(uint8_t sub_id);

        [[nodiscard]] uint8_t function() const;

        [[maybe_unused]] void setFunction(uint8_t function);

        [[nodiscard]] uint8_t swId() const;

        void setSwId(uint8_t sw_id);

        [[nodiscard]] uint8_t address() const;

        [[maybe_unused]] void setAddress(uint8_t address);

        [[nodiscard]] std::vector<uint8_t>::iterator paramBegin();

        [[nodiscard]] std::vector<uint8_t>::iterator paramEnd();

        [[nodiscard]] std::vector<uint8_t>::const_iterator paramBegin() const;

        [[nodiscard]] std::vector<uint8_t>::const_iterator paramEnd() const;

        void setParams(const std::vector<uint8_t>& _params);

        struct Hidpp10Error {
            hidpp::DeviceIndex device_index;
            uint8_t sub_id, address, error_code;
        };

        bool isError10(Hidpp10Error& error) const;

        struct Hidpp20Error {
            hidpp::DeviceIndex device_index;
            uint8_t feature_index, function, software_id, error_code;
        };

        bool isError20(Hidpp20Error& error) const;

        [[nodiscard]] const std::vector<uint8_t>& rawReport() const;

        static constexpr std::size_t HeaderLength = 4;
    private:
        std::vector<uint8_t> _data;
    };
}

#endif //LOGID_BACKEND_HIDPP_REPORT_H