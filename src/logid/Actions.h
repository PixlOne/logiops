#ifndef LOGID_ACTIONS_H
#define LOGID_ACTIONS_H
#include "Device.h"

#include <map>
#include <vector>
#include <hidpp20/IReprogControls.h>

namespace logid
{
    enum class Action
    {
        None,
        Keypress,
        Gestures,
        CycleDPI,
        ChangeDPI,
        ToggleSmartshift,
        ToggleHiresScroll
    };
    enum class Direction
    {
        None,
        Up,
        Down,
        Left,
        Right
    };
    enum class GestureMode
    {
        NoPress,
        OnRelease,
        OnFewPixels,
        Axis
    };

    class Device;

    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;

        Action type;
        virtual ButtonAction* copy(Device* dev) = 0;
        virtual void press() = 0;
        virtual void release() = 0;
    //    ButtonAction(const ButtonAction &a, Device* d) : type (a.type), device (d) {}
    //    ButtonAction* create_instance(Device* d);
    protected:
        ButtonAction(Action a) : type (a) {};
        Device* device;
    };
    class NoAction : public ButtonAction
    {
    public:
        NoAction() : ButtonAction(Action::None) {}
        virtual NoAction* copy(Device* dev);
        virtual void press() {}
        virtual void release() {}
    };
    class KeyAction : public ButtonAction
    {
    public:
        explicit KeyAction(std::vector<unsigned int> k) : ButtonAction(Action::Keypress), keys (std::move(k)) {};
        virtual KeyAction* copy(Device* dev);
        virtual void press();
        virtual void release();
    private:
        std::vector<unsigned int> keys;
    };
    class Gesture
    {
    public:
        struct axis_info {
            uint code;
            float multiplier;
        };

        Gesture(ButtonAction* ba, GestureMode m, void* aux=nullptr);
        Gesture(const Gesture &g, Device* dev)
                : action (g.action->copy(dev)), mode (g.mode), per_pixel (g.per_pixel),
                  axis (g.axis)
        {
        }

        ButtonAction* action;
        GestureMode mode;
        int per_pixel;
        int per_pixel_mod;
        axis_info axis;
    };

    class GestureAction : public ButtonAction
    {
    public:
        GestureAction(std::map<Direction, Gesture*> g) : ButtonAction(Action::Gestures), gestures (std::move(g)) {};
        std::map<Direction, Gesture*> gestures;
        virtual GestureAction* copy(Device* dev);
        virtual void press();
        virtual void release();
        void move(HIDPP20::IReprogControlsV4::Move m);
    private:
        bool held;
        int x = 0;
        int y = 0;
    };
    class SmartshiftAction : public ButtonAction
    {
    public:
        SmartshiftAction() : ButtonAction(Action::ToggleSmartshift) {};
        virtual SmartshiftAction* copy(Device* dev);
        virtual void press();
        virtual void release() {}
    };
    class HiresScrollAction : public ButtonAction
    {
    public:
        HiresScrollAction() : ButtonAction(Action::ToggleHiresScroll) {};
        virtual HiresScrollAction* copy(Device* dev);
        virtual void press();
        virtual void release() {}
    };
    class CycleDPIAction : public ButtonAction
    {
    public:
        CycleDPIAction(std::vector<int> d) : ButtonAction(Action::CycleDPI), dpis (d) {};
        virtual CycleDPIAction* copy(Device* dev);
        virtual void press();
        virtual void release() {}
    private:
        const std::vector<int> dpis;
    };
    class ChangeDPIAction : public ButtonAction
    {
    public:
        ChangeDPIAction(int i) : ButtonAction(Action::ChangeDPI), dpi_inc (i) {};
        virtual ChangeDPIAction* copy(Device* dev);
        virtual void press();
        virtual void release() {}
    private:
        int dpi_inc;
    };
}

#endif //LOGID_ACTIONS_H
