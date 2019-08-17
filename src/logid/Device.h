#ifndef DEVICE_H
#define DEVICE_H

#include "Actions.h"
#include "DeviceFinder.h"
#include "Configuration.h"

#include <map>
#include <memory>
#include <hidpp/Dispatcher.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/IReceiver.h>

class EventListener;
class DeviceConfig;

class Device
{
public:
    Device(std::string p, const HIDPP::DeviceIndex i);
    ~Device();

    std::string name;

    void configure();
    void reset();

    void press_button(uint16_t cid);
    void release_button(uint16_t cid);
    void move_diverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move move);

    void start();
    void stop();

    std::map<uint16_t, uint8_t> get_features();

    std::map<uint16_t, uint8_t> features;

    const std::string path;
    const HIDPP::DeviceIndex index;
    HIDPP::Dispatcher* dispatcher;
    HIDPP20::Device* hidpp_dev;

    bool configuring = false;
    bool disconnected = false;

protected:
    DeviceConfig* config;
    bool DeviceRemoved;
    EventListener* listener;

    void divert_buttons();
    void set_smartshift(HIDPP20::ISmartShift::SmartshiftStatus ops);
    void set_hiresscroll(uint8_t flags);
    void set_dpi(int dpi);
};

class EventHandler
{
public:
    virtual const HIDPP20::FeatureInterface *feature() const = 0;
    virtual const std::vector<uint8_t> featureIndices() const
    {
        return {feature()->index()};
    };
    virtual void handleEvent (const HIDPP::Report &event) = 0;
};
class ButtonHandler : public EventHandler
{
public:
    ButtonHandler (HIDPP20::Device *hidppdev, Device *d) : _irc (HIDPP20::IReprogControls::auto_version(hidppdev)), dev (d) { }
    const HIDPP20::FeatureInterface *feature () const
    {
        return &_irc;
    }
    void handleEvent (const HIDPP::Report &event);
protected:
    HIDPP20::IReprogControls _irc;
    Device* dev;
    std::vector<uint16_t> states;
    std::vector<uint16_t> new_states;
};
class ReceiverHandler : public EventHandler
{
public:
    ReceiverHandler (Device *d) : dev (d) { }
    const HIDPP20::FeatureInterface *feature () const
    {
        return nullptr; // This sounds like a horrible idea
    }
    virtual const std::vector<uint8_t> featureIndices() const
    {
        return HIDPP10::IReceiver::Events;
    }
    void handleEvent (const HIDPP::Report &event);
protected:
    Device* dev;
};

class EventListener
{
    HIDPP::Dispatcher *dispatcher;
    HIDPP::DeviceIndex index;
    std::map<uint8_t, std::unique_ptr<EventHandler>> handlers;
    std::map<uint8_t, HIDPP::Dispatcher::listener_iterator> iterators;
public:
    EventListener (HIDPP::Dispatcher *dispatcher, HIDPP::DeviceIndex index): dispatcher (dispatcher), index (index) {}

    virtual void removeEventHandlers ();
    virtual ~EventListener();
    virtual void addEventHandler (std::unique_ptr<EventHandler> &&handler);

    virtual void start () = 0;
    virtual void stop () = 0;

protected:
    virtual bool event (EventHandler* handler, const HIDPP::Report &report) = 0;
};
class SimpleListener : public EventListener
{
    HIDPP::SimpleDispatcher *dispatcher;

public:
    SimpleListener (HIDPP::SimpleDispatcher* dispatcher, HIDPP::DeviceIndex index):
            EventListener (dispatcher, index),
            dispatcher (dispatcher)
    {
    }

    virtual void start();
    virtual void stop();

protected:
    virtual bool event (EventHandler* handler, const HIDPP::Report &report);
};

#endif //DEVICE_H