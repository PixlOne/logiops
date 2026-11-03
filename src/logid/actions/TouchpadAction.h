#ifndef LOGID_ACTIONS_TOUCHPADACTION_H
#define LOGID_ACTIONS_TOUCHPADACTION_H

#include <actions/Action.h>
#include <TouchpadDevice.h>

namespace logid {
    class Device;
}

namespace logid::actions {
    class TouchpadAction : public Action {
    public:
        static const char* interface_name;

        TouchpadAction(Device* device, config::TouchpadAction& config,
                       const std::shared_ptr<ipcgull::node>& parent);

        void press() override;
        void release() override;
        void move(int16_t x, int16_t y) override;
        [[nodiscard]] uint8_t reprogFlags() const override;

    private:
        void _applyConfigLocked();

        int _fingers;
        double _scale;
        bool _invert_x;
        bool _invert_y;
        double _x_remainder;
        double _y_remainder;
        config::TouchpadAction& _config;
        std::shared_ptr<TouchpadDevice> _touchpad;
    };
}

#endif // LOGID_ACTIONS_TOUCHPADACTION_H
