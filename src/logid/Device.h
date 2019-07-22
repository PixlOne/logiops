#ifndef LOGIOPS_DEVICE_H
#define LOGIOPS_DEVICE_H

#include <map>
#include <memory>
#include <hidpp/Dispatcher.h>
#include <hidpp/SimpleDispatcher.h>

#include "Actions.h"
#include "Configuration.h"

class Device
{
public:
    Device(std::string p, const HIDPP::DeviceIndex i);

    std::string name;

    void configure(bool scanning=false);

    void press_button(uint16_t cid);
    void release_button(uint16_t cid);
    void move_diverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move move);

    void start();
    void stop();

    std::map<uint16_t, uint8_t> get_features();

    std::map<uint16_t, uint8_t> features;

    std::string path;
    const HIDPP::DeviceIndex index;
    HIDPP::Dispatcher* dispatcher;
    HIDPP20::Device* hidpp_dev;

    class EventHandler
    {
    public:
        virtual const HIDPP20::FeatureInterface *feature() const = 0;
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

protected:
    DeviceConfig* config;
    bool DeviceRemoved;
    EventListener* listener;

    void divert_buttons(bool scanning=false);
    void set_smartshift(struct smartshift_options ops, bool scanning=false);
    void set_hiresscroll(bool b, bool scanning=false);
    void set_dpi(int dpi, bool scanning=false);
};


#endif //LOGIOPS_DEVICE_H
