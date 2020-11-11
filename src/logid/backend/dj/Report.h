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

#ifndef LOGID_BACKEND_DJ_REPORT_H
#define LOGID_BACKEND_DJ_REPORT_H

#include <cstdint>
#include "../raw/RawDevice.h"
#include "defs.h"
#include "../hidpp/defs.h"

namespace logid {
namespace backend {
namespace dj
{
    namespace Offset
    {
        static constexpr uint8_t Type = 0;
        static constexpr uint8_t DeviceIndex = 1;
        static constexpr uint8_t Feature = 2;
        static constexpr uint8_t Parameters = 3;
    }

    bool supportsDjReports(std::vector<uint8_t>&& rdesc);
    class Report
    {
    public:
        typedef ReportType::ReportType Type;

        explicit Report(std::vector<uint8_t>& data);
        Report(Type type, hidpp::DeviceIndex index, uint8_t feature);

        Type type() const;
        hidpp::DeviceIndex index() const;
        uint8_t feature() const;
        std::vector<uint8_t>::iterator paramBegin();
        std::vector<uint8_t> rawData() const;
    private:
        std::vector<uint8_t> _data;
    };
}}}

#endif //LOGID_BACKEND_DJ_REPORT_H
