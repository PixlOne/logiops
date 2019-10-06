#include <cstdint>
#include <future>
#include <unistd.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp20/IAdjustableDPI.h>
#include <hidpp20/IFeatureSet.h>
#include <hidpp20/Error.h>
#include <hidpp20/IReprogControls.h>
#include <hidpp20/IReset.h>
#include <hidpp20/ISmartShift.h>
#include <hidpp20/Device.h>
#include <hidpp10/Error.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <set>
#include <hidpp20/UnsupportedFeature.h>
#include <hidpp20/IHiresScroll.h>

#include "Device.h"
#include "util.h"
#include "EvdevDevice.h"

using namespace logid;

using namespace std::chrono_literals;

Device::Device(std::string p, const HIDPP::DeviceIndex i) : path(std::move(p)), index (i)
{
    disconnected = true;
    dispatcher = new HIDPP::SimpleDispatcher(path.c_str());
    listener = new SimpleListener(new HIDPP::SimpleDispatcher(path.c_str()), index);
    config = new DeviceConfig();
}

bool Device::init()
{
    // Initialise variables
    disconnected = false;
    try
    {
        hidpp_dev = new HIDPP20::Device(dispatcher, index);
    }
    catch(HIDPP10::Error &e) { return false; }
    catch(HIDPP20::Error &e) { return false; }

    name = hidpp_dev->name();
    features = getFeatures();
    // Set config, if none is found for this device then use default
    if(global_config->devices.find(name) == global_config->devices.end())
        log_printf(INFO, "Device %s not configured, using default config.", hidpp_dev->name().c_str());
    else
    {
        delete(config);
        config = global_config->devices.find(name)->second;
    }

    initialized = true;

    return true;
}

Device::~Device()
{
    if(!disconnected)
        this->reset();
    if(!config->baseConfig)
        delete(this->config);
}

void Device::configure()
{
    if(config->baseConfig)
        config = new DeviceConfig(config, this);

    if(!configuring.try_lock())
    {
        log_printf(DEBUG, "%s %d: skip config task", path.c_str(), index);
        return;
    }
    usleep(500000);

    try
    {
        if(disconnected)
            goto ret;
        // Divert buttons
        divert_buttons();

        if(disconnected)
            goto ret;
        // Set DPI if it is configured
        if(config->dpi != nullptr)
            setDPI(*config->dpi);

        if(disconnected)
            goto ret;
        // Set Smartshift if it is configured
        if(config->smartshift != nullptr)
            setSmartShift(*config->smartshift);

        if(disconnected)
            goto ret;
        // Set Hires Scroll if it is configured
        if(config->hiresscroll != nullptr)
            setHiresScroll(*config->hiresscroll);
    }
    catch(HIDPP10::Error &e)
    {
        log_printf(ERROR, "HID++ 1.0 Error whjle configuring %s: %s", name.c_str(), e.what());
    }

ret:
    configuring.unlock();
}

void Device::reset()
{
    try
    {
        HIDPP20::IReset iReset(hidpp_dev);
        iReset.reset();
    }
    catch(HIDPP20::UnsupportedFeature &e) { }
    catch(HIDPP10::Error &e)
    {
        log_printf(ERROR, "Failed to reset %s: %s", name.c_str(), e.what());
    }
}

void Device::divert_buttons()
{
    try
    {
        HIDPP20::IReprogControls irc = HIDPP20::IReprogControls::auto_version(hidpp_dev);
        if(disconnected)
            return;
        int controlCount = irc.getControlCount();
        for(int i = 0; i < controlCount; i++)
        {
            if(disconnected)
                return;
            uint16_t cid = irc.getControlInfo(i).control_id;
            uint8_t flags = 0;
            flags |= HIDPP20::IReprogControls::ChangeTemporaryDivert;
            flags |= HIDPP20::IReprogControls::ChangeRawXYDivert;

            auto action = config->actions.find(cid);
            if(action != config->actions.end())
            {
                flags |= HIDPP20::IReprogControls::ChangeTemporaryDivert;
                flags |= HIDPP20::IReprogControls::TemporaryDiverted;
                if(action->second->type == Action::Gestures)
                    flags |= HIDPP20::IReprogControls::RawXYDiverted;
            }
            if(disconnected)
                return;
            irc.setControlReporting(cid, flags, cid);
        }
    }
    catch(HIDPP20::UnsupportedFeature &e)
    {
        log_printf(DEBUG, "%s does not support Reprog controls, not diverting!", name.c_str());
    }
    catch(HIDPP10::Error &e)
    {
        log_printf(DEBUG, "Could not divert buttons: HID++ 1.0 Error %s!", e.what());
    }
}

void Device::setSmartShift(HIDPP20::ISmartShift::SmartshiftStatus ops)
{
    try
    {
        if(disconnected) return;
        HIDPP20::ISmartShift ss(hidpp_dev);
        if(disconnected) return;
        ss.setStatus(ops);
    }
    catch (HIDPP20::UnsupportedFeature &e)
    {
        log_printf(ERROR, "Device does not support SmartShift");
    }
    catch (HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error setting SmartShift options, code %d: %s\n", e.errorCode(), e.what());
    }
}

void Device::setHiresScroll(uint8_t ops)
{
    try
    {
        if(disconnected) return;
        HIDPP20::IHiresScroll hs(hidpp_dev);
        if(disconnected) return;
        hs.setMode(ops);
    }
    catch (HIDPP20::UnsupportedFeature &e)
    {
        log_printf(ERROR, "Device does not support Hires Scrolling");
    }
    catch (HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error setting Hires Scrolling options, code %d: %s\n", e.errorCode(), e.what());
    }
}

void Device::setDPI(int dpi)
{
    if(disconnected) return;
    HIDPP20::IAdjustableDPI iad(hidpp_dev);
    if(disconnected) return;
    try { for(unsigned int i = 0; i < iad.getSensorCount(); i++) iad.setSensorDPI(i, dpi); }
    catch (HIDPP20::Error &e) { log_printf(ERROR, "Error while setting DPI: %s", e.what()); }
}

void Device::waitForReceiver()
{
    while(true)
    {
        waiting_for_receiver = true;
        listener->addEventHandler(std::make_unique<ReceiverHandler>(this));
        listener->start();
        // Listener stopped, check if stopped or ReceiverHandler event
        if (waiting_for_receiver)
            return;

        usleep(200000);

        if(this->init()) break;

        log_printf(ERROR, "Failed to initialize device %d on %s, waiting for receiver");
        delete(listener);
        listener = new SimpleListener(new HIDPP::SimpleDispatcher(path.c_str()), index);
    }
    log_printf(INFO, "%s detected: device %d on %s", name.c_str(), index, path.c_str());
    this->start();
}

void Device::start()
{
    configure();
    try { listener->addEventHandler(std::make_unique<ButtonHandler>(this)); }
    catch(HIDPP20::UnsupportedFeature &e) { }

    if(index == HIDPP::DefaultDevice || index == HIDPP::CordedDevice)
    {
        try { listener->addEventHandler( std::make_unique<WirelessStatusHandler>(this) ); }
        catch(HIDPP20::UnsupportedFeature &e) { }
    }
    listener->start();
}

bool Device::testConnection()
{
    int i = MAX_CONNECTION_TRIES;
    do {
        try
        {
            HIDPP20::Device _hpp20dev(dispatcher, index);
            return true;
        }
        catch(HIDPP10::Error &e)
        {
            if(e.errorCode() == HIDPP10::Error::ResourceError) // Asleep, wait for next event
                return false;
            if(i == MAX_CONNECTION_TRIES-1)
                return false;
        }
        catch(std::exception &e)
        {
            if(i == MAX_CONNECTION_TRIES-1)
                return false;
        }
        i++;
    } while(i < MAX_CONNECTION_TRIES);

    return false;
}

void ButtonHandler::handleEvent (const HIDPP::Report &event)
{
    switch (event.function())
    {
        case HIDPP20::IReprogControls::Event::DivertedButtonEvent:
        {
            new_states = HIDPP20::IReprogControls::divertedButtonEvent(event);
            if (states.empty())
            {
                for (uint16_t i : new_states)
                    std::thread{[=]() { dev->pressButton(i); }}.detach();
                states = new_states;
                break;
            }
            std::vector<uint16_t>::iterator it;
            std::vector<uint16_t> cids(states.size() + new_states.size());
            it = std::set_union(states.begin(), states.end(), new_states.begin(), new_states.end(), cids.begin());
            cids.resize((ulong)(it - cids.begin()));
            for (uint16_t i : cids)
            {
                if (std::find(new_states.begin(), new_states.end(), i) != new_states.end())
                {
                    if (std::find(states.begin(), states.end(), i) == states.end())
                        std::thread{[=]() { dev->pressButton(i); }}.detach();
                } else
                    std::thread{[=]() { dev->releaseButton(i); }}.detach();
            }
            states = new_states;
            break;
        }
        case HIDPP20::IReprogControlsV4::Event::DivertedRawXYEvent:
        {
            auto raw_xy = HIDPP20::IReprogControlsV4::divertedRawXYEvent(event);

            for(uint16_t i : states)
                std::thread{[=]() { dev->moveDiverted(i, raw_xy); }}.detach();
            break;
        }
        default:
            break;
    }
}

void ReceiverHandler::handleEvent(const HIDPP::Report &event)
{
    switch(event.featureIndex())
    {
        case HIDPP10::IReceiver::DeviceUnpaired:
        {
            log_printf(INFO, "%s (Device %d on %s) unpaired from receiver", dev->name.c_str(), dev->index, dev->path.c_str());
            std::thread {[=]()
                         {
                            finder->stopAndDeleteDevice(dev->path, dev->index);
                            finder->insertNewReceiverDevice(dev->path, dev->index);
                         }}.detach();
            break;
        }
        case HIDPP10::IReceiver::DevicePaired:
        {
            log_printf(DEBUG, "Receiver on %s: Device %d paired", dev->path.c_str(), event.deviceIndex());
            if(dev->waiting_for_receiver)
            {
                if(!dev->testConnection()) return;
                dev->waiting_for_receiver = false;
                dev->stop();
            }
            //else: Likely an enumeration event, ignore.
            break;
        }
        case HIDPP10::IReceiver::ConnectionStatus:
        {
            auto status = HIDPP10::IReceiver::connectionStatusEvent(event);
            if(status == HIDPP10::IReceiver::LinkLoss)
            {
                log_printf(INFO, "Link lost to %s", dev->name.c_str());
                dev->disconnected = true;
            }
            else if (status == HIDPP10::IReceiver::ConnectionEstablished)
            {
                if(dev->waiting_for_receiver)
                {
                    log_printf(DEBUG, "Receiver on %s: Connection established to device %d", dev->path.c_str(), event.deviceIndex());
                    if(!dev->testConnection()) return;
                    dev->waiting_for_receiver = false;
                    std::thread { [=]() { dev->stop(); } }.detach();
                }
                else
                {
                    if(!dev->initialized) return;
                    dev->disconnected = false;
                    dev->configure();
                    log_printf(INFO, "Connection established to %s", dev->name.c_str());
                }
            }
            break;
        }
        default:
            break;
    }
}

void WirelessStatusHandler::handleEvent(const HIDPP::Report &event)
{
    switch(event.function())
    {
        case HIDPP20::IWirelessDeviceStatus::StatusBroadcast:
        {
            auto status = HIDPP20::IWirelessDeviceStatus::statusBroadcastEvent(event);
            if(status.ReconfNeeded)
                dev->configure();
            break;
        }
        default:
        {
            log_printf(DEBUG, "Undocumented event %02x from WirelessDeviceStatus", event.function());
            break;
        }
    }
}

void EventListener::removeEventHandlers ()
{
    for (const auto &p: iterators)
        dispatcher->unregisterEventHandler(p.second);
    handlers.clear();
    iterators.clear();
}

EventListener::~EventListener()
{
    removeEventHandlers();
}

void EventListener::addEventHandler(std::unique_ptr<EventHandler> &&handler)
{
    EventHandler *ptr = handler.get();
    for(uint8_t feature : handler->featureIndices())
    {
        handlers.emplace(feature, std::move(handler));
        dispatcher->registerEventHandler(index, feature, [=](const HIDPP::Report &report)
        {
            ptr->handleEvent(report);
            return true;
        });
    }
}

void SimpleListener::start()
{
    bool retry;
    do
    {
        retry = false;
        try { dispatcher->listen(); }
        catch(std::system_error &e)
        {
            retry = true;
            usleep(250000);
        }
    } while(retry && !stopped);

}

void SimpleListener::stop()
{
    this->stopped = true;
    dispatcher->stop();
}

bool SimpleListener::event (EventHandler *handler, const HIDPP::Report &report)
{
    handler->handleEvent (report);
    return true;
}

void Device::stop()
{
    disconnected = true;
    listener->stop();
}

void Device::pressButton(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(DEBUG, "0x%x was pressed but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->press();
}

void Device::releaseButton(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(DEBUG, "0x%x was released but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->release();
}

void Device::moveDiverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move m)
{
    auto action = config->actions.find(cid);
    if(action == config->actions.end())
        return;
    switch(action->second->type)
    {
        case Action::Gestures:
            ((GestureAction*)action->second)->move(m);
            break;
        default:
            break;
    }
}

std::map<uint16_t, uint8_t> Device::getFeatures()
{
    std::map<uint16_t, uint8_t> _features;
    HIDPP20::IFeatureSet ifs (hidpp_dev);
    uint8_t feature_count = ifs.getCount();

    for(uint8_t i = 0; i < feature_count; i++)
        _features.insert( {i, ifs.getFeatureID(i) } );

    return _features;
}
