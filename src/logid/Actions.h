#ifndef LOGIOPS_ACTIONS_H
#define LOGIOPS_ACTIONS_H

#include <map>
#include <hidpp20/IReprogControlsV4.h>
#include "Device.h"

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
    OnFewPixels
};

class Device;

class ButtonAction
{
public:
    Action type;
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
    virtual void press() {}
    virtual void release() {}
};
class KeyAction : public ButtonAction
{
public:
    explicit KeyAction(std::vector<unsigned int> k) : ButtonAction(Action::Keypress), keys (std::move(k)) {};
    KeyAction(const KeyAction &a, Device* d);
    //virtual KeyAction* create_instance(Device* d) { return new KeyAction(*this, d); };
    virtual void press();
    virtual void release();
private:
    std::vector<unsigned int> keys;
};
class Gesture
{
public:
    Gesture(ButtonAction* ba, GestureMode m, int pp=0) : action (ba), mode (m), per_pixel (pp) {};
    Gesture(const Gesture &g) : action (g.action), mode (g.mode), per_pixel (g.per_pixel) {};

    ButtonAction* action;

    GestureMode mode;
    int per_pixel;
};
class GestureAction : public ButtonAction
{
public:
    GestureAction(std::map<Direction, Gesture*> g) : ButtonAction(Action::Gestures), gestures (std::move(g)) {};
    std::map<Direction, Gesture*> gestures;
    virtual void press();
    void move(HIDPP20::IReprogControlsV4::Move m);
    virtual void release();
private:
    bool held;
    int x;
    int y;
};
class SmartshiftAction : public ButtonAction
{
public:
    SmartshiftAction() : ButtonAction(Action::ToggleSmartshift) {};
    virtual void press();
    virtual void release() {}
};
class HiresScrollAction : public ButtonAction
{
public:
    HiresScrollAction() : ButtonAction(Action::ToggleHiresScroll) {};
    virtual void press();
    virtual void release() {}
};
class CycleDPIAction : public ButtonAction
{
public:
    CycleDPIAction(std::vector<int> d) : ButtonAction(Action::CycleDPI), dpis (d) {};
    virtual void press();
    virtual void release() {}
private:
    const std::vector<int> dpis;
};
class ChangeDPIAction : public ButtonAction
{
public:
    ChangeDPIAction(int i) : ButtonAction(Action::ChangeDPI), dpi_inc (i) {};
    virtual void press();
    virtual void release() {}
private:
    int dpi_inc;
};

#endif //LOGIOPS_ACTIONS_H
