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
#include <cassert>
#include "../Error.h"
#include "ReprogControls.h"

using namespace logid::backend::hidpp20;

#define DEFINE_REPROG(x, base) \
x::x(Device* dev) : base(dev, ID) \
{ \
} \
x::x(Device* dev, uint16_t _id) : base(dev, _id) \
{ \
}

#define MAKE_REPROG(x, dev) \
try { \
    return std::make_shared<x>(dev); \
} catch(UnsupportedFeature &e) {\
}

// Define all of the ReprogControls versions
DEFINE_REPROG(ReprogControls, Feature);
DEFINE_REPROG(ReprogControlsV2, ReprogControls);
DEFINE_REPROG(ReprogControlsV2_2, ReprogControlsV2);
DEFINE_REPROG(ReprogControlsV3, ReprogControlsV2_2);
DEFINE_REPROG(ReprogControlsV4, ReprogControlsV3);

std::shared_ptr<ReprogControls> ReprogControls::autoVersion(Device *dev)
{
    MAKE_REPROG(ReprogControlsV4, dev);
    MAKE_REPROG(ReprogControlsV3, dev);
    MAKE_REPROG(ReprogControlsV2_2, dev);
    MAKE_REPROG(ReprogControlsV2, dev);

    // If base version cannot be made, throw error
    return std::make_shared<ReprogControls>(dev);
}

uint8_t ReprogControls::getControlCount()
{
    std::vector<uint8_t> params(0);
    auto response = callFunction(GetControlCount, params);
    return response[0];
}

ReprogControls::ControlInfo ReprogControls::getControlInfo(uint8_t index)
{
    std::vector<uint8_t> params(1);
    ControlInfo info{};
    params[0] = index;
    auto response = callFunction(GetControlInfo, params);

    info.controlID = response[1];
    info.controlID |= response[0] << 8;
    info.taskID = response[3];
    info.taskID |= response[2] << 8;
    info.flags = response[4];
    info.position = response[5];
    info.group = response[6];
    info.groupMask = response[7];
    info.additionalFlags = response[8];
    return info;
}

ReprogControls::ControlInfo ReprogControls::getControlIdInfo(uint16_t cid)
{
    if(_cids.empty()) {
        for(uint8_t i = 0; i < getControlCount(); i++) {
            auto info = getControlInfo(i);
            _cids.emplace(info.controlID, info);
        }
    }

    auto it = _cids.find(cid);
    if(it == _cids.end())
        throw Error(Error::InvalidArgument);
    else
        return it->second;
}

ReprogControls::ControlInfo ReprogControls::getControlReporting(uint16_t cid)
{
    // Emulate this function, only Reprog controls v4 supports this
    auto info = getControlIdInfo(cid);

    ControlInfo report{};
    report.controlID = cid;
    report.flags = 0;
    if(info.flags & TemporaryDivertable)
        report.flags |= TemporaryDiverted;
    if(info.flags & PersisentlyDivertable)
        report.flags |= PersistentlyDiverted;
    if(info.additionalFlags & RawXY)
        report.flags |= RawXYDiverted;

    return report;
}

void ReprogControls::setControlReporting(uint8_t cid, ControlInfo info)
{
    // This function does not exist pre-v4 and cannot be emulated, ignore.
}

std::set<uint16_t> ReprogControls::divertedButtonEvent(
        const hidpp::Report& report)
{
    assert(report.function() == DivertedButtonEvent);
    std::set<uint16_t> buttons;
    uint8_t cids = std::distance(report.paramBegin(), report.paramEnd())/2;
    for(uint8_t i = 0; i < cids; i++) {
        uint16_t cid = report.paramBegin()[2*i + 1];
        cid |= report.paramBegin()[2*i] << 8;
        if(cid)
            buttons.insert(cid);
        else
            break;
    }
    return buttons;
}

ReprogControls::Move ReprogControls::divertedRawXYEvent(const hidpp::Report
    &report)
{
    assert(report.function() == DivertedRawXYEvent);
    Move move{};
    move.x = report.paramBegin()[1];
    move.x |= report.paramBegin()[0] << 8;
    move.y = report.paramBegin()[3];
    move.y |= report.paramBegin()[2] << 8;
    return move;
}

ReprogControls::ControlInfo ReprogControlsV4::getControlReporting(uint16_t cid)
{
    std::vector<uint8_t> params(2);
    ControlInfo info{};
    params[0] = (cid >> 8) & 0xff;
    params[1] = cid & 0xff;
    auto response = callFunction(GetControlReporting, params);

    info.controlID = response[1];
    info.controlID |= response[0] << 8;
    info.flags = response[2];
    return info;
}

void ReprogControlsV4::setControlReporting(uint8_t cid, ControlInfo info)
{
    std::vector<uint8_t> params(5);
    params[0] = (cid >> 8) & 0xff;
    params[1] = cid & 0xff;
    params[2] = info.flags;
    params[3] = (info.controlID >> 8) & 0xff;
    params[4] = info.controlID & 0xff;
    callFunction(SetControlReporting, params);
}