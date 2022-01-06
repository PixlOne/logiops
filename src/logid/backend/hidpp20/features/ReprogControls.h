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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_REPROGCONTROLS_H
#define LOGID_BACKEND_HIDPP20_FEATURE_REPROGCONTROLS_H

#include <map>

#include "../feature_defs.h"
#include "../Feature.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class ReprogControls : public Feature
    {
    public:
        enum Function {
            GetControlCount = 0,
            GetControlInfo = 1,
            GetControlReporting = 2,
            SetControlReporting = 3
        };
        enum Event {
            DivertedButtonEvent = 0,
            DivertedRawXYEvent = 1
        };

        struct ControlInfo
        {
            uint16_t controlID;
            uint16_t taskID;
            uint8_t flags;
            uint8_t position; // F key position, 0 if not an Fx key
            uint8_t group;
            uint8_t groupMask;
            uint8_t additionalFlags;
        };

        enum ControlInfoFlags: uint8_t
        {
            MouseButton = 1, //Mouse button
            FKey = 1<<1, //Fx key
            Hotkey = 1<<2,
            FnToggle = 1<<3,
            ReprogHint = 1<<4,
            TemporaryDivertable = 1<<5,
            PersisentlyDivertable = 1<<6,
            Virtual = 1<<7
        };
        enum ControlInfoAdditionalFlags: uint8_t {
            RawXY = 1<<0
        };

        enum ControlReportingFlags: uint8_t {
            TemporaryDiverted = 1<<0,
            ChangeTemporaryDivert = 1<<1,
            PersistentlyDiverted = 1<<2,
            ChangePersistentDivert = 1<<3,
            RawXYDiverted = 1<<4,
            ChangeRawXYDivert = 1<<5
        };

        struct Move
        {
            int16_t x;
            int16_t y;
        };

        static const uint16_t ID = FeatureID::REPROG_CONTROLS;
        virtual uint16_t getID() { return ID; }

        virtual bool supportsRawXY() { return false; }

        explicit ReprogControls(Device* dev);

        virtual uint8_t getControlCount();

        virtual ControlInfo getControlInfo(uint8_t cid);

        virtual ControlInfo getControlIdInfo(uint16_t cid);

        virtual void initCidMap();

        const std::map<uint16_t, ControlInfo>& getControls() const;

        // Onlu controlId and flags will be set
        virtual ControlInfo getControlReporting(uint16_t cid);

        // Only controlId (for remap) and flags will be read
        virtual void setControlReporting(uint8_t cid, ControlInfo info);

        static std::set<uint16_t> divertedButtonEvent(const hidpp::Report&
            report);

        static Move divertedRawXYEvent(const hidpp::Report& report);

        static std::shared_ptr<ReprogControls> autoVersion(Device *dev);
    protected:
        ReprogControls(Device* dev, uint16_t _id);
        std::map<uint16_t, ControlInfo> _cids;
        bool _cids_initialized = false;
        std::mutex _cids_populating;
    };

    class ReprogControlsV2 : public ReprogControls
    {
    public:
        static const uint16_t ID = FeatureID::REPROG_CONTROLS_V2;
        virtual uint16_t getID() override { return ID; }

        explicit ReprogControlsV2(Device* dev);
    protected:
        ReprogControlsV2(Device* dev, uint16_t _id);
    };

    class ReprogControlsV2_2 : public ReprogControlsV2
    {
    public:
        static const uint16_t ID = FeatureID::REPROG_CONTROLS_V2_2;
        virtual uint16_t getID() override { return ID; }

        explicit ReprogControlsV2_2(Device* dev);
    protected:
        ReprogControlsV2_2(Device* dev, uint16_t _id);
    };

    class ReprogControlsV3 : public ReprogControlsV2_2
    {
    public:
        static const uint16_t ID = FeatureID::REPROG_CONTROLS_V3;
        virtual uint16_t getID() override { return ID; }

        explicit ReprogControlsV3(Device* dev);
    protected:
        ReprogControlsV3(Device* dev, uint16_t _id);
    };

    class ReprogControlsV4 : public ReprogControlsV3
    {
    public:
        static const uint16_t ID = FeatureID::REPROG_CONTROLS_V4;
        virtual uint16_t getID() override { return ID; }

        bool supportsRawXY() override { return true; }

        ControlInfo getControlReporting(uint16_t cid) override;

        void setControlReporting(uint8_t cid, ControlInfo info) override;

        explicit ReprogControlsV4(Device* dev);
    protected:
        ReprogControlsV4(Device* dev, uint16_t _id);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_REPROGCONTROLS_H
