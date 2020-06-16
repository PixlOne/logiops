
#ifndef LOGID_DEVICE_H
#define LOGID_DEVICE_H

#include "Actions.h"
#include "DeviceMonitor.h"
#include "Configuration.h"

#include <map>
#include <memory>
#include <atomic>
#include <hidpp/Dispatcher.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/IReceiver.h>
#include <hidpp20/IWirelessDeviceStatus.h>

namespace logid
{
    class EventListener;
    class DeviceConfig;

    class BlacklistedDevice : public std::exception
    {
    public:
        BlacklistedDevice() = default;
        virtual const char* what()
        {
            return "Blacklisted device";
        }
    };

    class Device
    {
    public:
        Device(std::string p, const HIDPP::DeviceIndex i);
        ~Device();

        std::string name;
        uint16_t pid;

        bool init();
        void configure();
        void reset();

        void pressButton(uint16_t cid);
        void releaseButton(uint16_t cid);
        void moveDiverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move move);

        void waitForReceiver();
        void start();
        void stop();
        bool testConnection();

        std::map<uint16_t, uint8_t> getFeatures();

        std::map<uint16_t, uint8_t> features;

        const std::string path;
        const HIDPP::DeviceIndex index;
        HIDPP::Dispatcher* dispatcher;
        HIDPP20::Device* hidpp_dev;

        std::mutex configuring;
        std::atomic_bool disconnected;
        bool initialized = false;
        bool waiting_for_receiver = false;

    protected:
        DeviceConfig* config;
        EventListener* listener;

        void divert_buttons();
        void printCIDs();
        void setSmartShift(HIDPP20::ISmartShift::SmartshiftStatus ops);
        void setHiresScroll(uint8_t flags);
        void setDPI(int dpi);
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
        ButtonHandler (Device *d) : dev (d), _irc (HIDPP20::IReprogControls::auto_version(d->hidpp_dev)) { }
        const HIDPP20::FeatureInterface *feature () const
        {
            return &_irc;
        }
        void handleEvent (const HIDPP::Report &event);
    protected:
        Device* dev;
        HIDPP20::IReprogControls _irc;
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
    class WirelessStatusHandler : public EventHandler
    {
    public:
        WirelessStatusHandler (Device *d) : dev (d), _iws (d->hidpp_dev) { }
        const HIDPP20::FeatureInterface *feature () const
        {
            return &_iws;
        }
        void handleEvent (const HIDPP::Report &event);
    protected:
        Device* dev;
        HIDPP20::IWirelessDeviceStatus _iws;
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

        bool stopped = false;
        virtual void start();
        virtual void stop();

    protected:
        virtual bool event (EventHandler* handler, const HIDPP::Report &report);
    };

}

#endif //LOGID_DEVICE_H
