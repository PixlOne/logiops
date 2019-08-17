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
#include <hidpp20/UnsupportedFeature.h>
#include <hidpp20/IHiresScroll.h>

#include "Device.h"
#include "util.h"
#include "EvdevDevice.h"


using namespace std::chrono_literals;

Device::Device(std::string p, const HIDPP::DeviceIndex i) : path(std::move(p)), index (i)
{
    // Initialise variables
    DeviceRemoved = false;
    dispatcher = new HIDPP::SimpleDispatcher(path.c_str());
    hidpp_dev = new HIDPP20::Device(dispatcher, index);
    name = hidpp_dev->name();
    features = get_features();

    // Set config, if none is found for this device then use default
    if(global_config->devices.find(name) == global_config->devices.end())
    {
        log_printf(INFO, "Device %s not configured, using default config.", hidpp_dev->name().c_str());
        config = new DeviceConfig();
    }
    else config = global_config->devices.find(name)->second;
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

    configuring = true;
    usleep(250000);

    try
    {
        if(disconnected) {
            configuring = false; return; }

        // Divert buttons
        divert_buttons();
        if(disconnected) {
            configuring = false; return; }

        // Set DPI if it is set
        if(config->dpi != nullptr)
            set_dpi(*config->dpi);
        if(disconnected) {
            configuring = false; return; }

        // Set Smartshift if it is set
        if(config->smartshift != nullptr)
            set_smartshift(*config->smartshift);
        if(disconnected) {
            configuring = false; return; }

        // Set Hires Scroll if it is set
        if(config->hiresscroll != nullptr)
            set_hiresscroll(*config->hiresscroll);
    }
    catch(HIDPP10::Error &e)
    {
        log_printf(ERROR, "HID++ 1.0 Error whjle configuring %s: %s", name.c_str(), e.what());
    }

    configuring = false;
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
        if(disconnected) return;
        int controlCount = irc.getControlCount();
        for(int i = 0; i < controlCount; i++)
        {
            if(disconnected) return;
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
            if(disconnected) return;
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

void Device::set_smartshift(HIDPP20::ISmartShift::SmartshiftStatus ops)
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

void Device::set_hiresscroll(uint8_t ops)
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

void Device::set_dpi(int dpi)
{
    if(disconnected) return;
    HIDPP20::IAdjustableDPI iad(hidpp_dev);
    if(disconnected) return;
    try { for(unsigned int i = 0; i < iad.getSensorCount(); i++) iad.setSensorDPI(i, dpi); }
    catch (HIDPP20::Error &e) { log_printf(ERROR, "Error while setting DPI: %s", e.what()); }
}

void Device::start()
{
    configure();
    auto *d = new HIDPP::SimpleDispatcher(path.c_str());
    listener = new SimpleListener(d, index);
    listener->addEventHandler( std::make_unique<ButtonHandler>(hidpp_dev, this) );
    listener->addEventHandler( std::make_unique<ReceiverHandler>(this) );
    listener->start();
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
                    std::thread{[=]() { dev->press_button(i); }}.detach();
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
                        std::thread{[=]() { dev->press_button(i); }}.detach();
                } else
                    std::thread{[=]() { dev->release_button(i); }}.detach();
            }
            states = new_states;
            break;
        }
        case HIDPP20::IReprogControlsV4::Event::DivertedRawXYEvent:
        {
            auto raw_xy = HIDPP20::IReprogControlsV4::divertedRawXYEvent(event);

            for(uint16_t i : states)
                std::thread{[=]() { dev->move_diverted(i, raw_xy); }}.detach();
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
            // Find device, stop it, and delete it
            auto it = finder->devices.begin();
            while (it != finder->devices.end())
            {
                if(it->first->path == dev->path && it->first->index == event.deviceIndex())
                {
                    log_printf(INFO, "%s (Device %d on %s) unpaired.", it->first->name.c_str(), event.deviceIndex(), dev->path.c_str());
                    it->first->stop();
                    it->second.join();
                    finder->devices.erase(it);
                }
                else it++;
            }
            break;
        }
        case HIDPP10::IReceiver::DevicePaired:
            break;
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
                log_printf(INFO, "Connection established to %s", dev->name.c_str());
                dev->disconnected = false;
                dev->configure();
            }
            break;
        }
        default:
            break;
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
    try { dispatcher->listen(); }
    catch(std::system_error &e) { }
}

void SimpleListener::stop()
{
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

void Device::press_button(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(DEBUG, "0x%x was pressed but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->press();
}

void Device::release_button(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(DEBUG, "0x%x was released but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->release();
}

void Device::move_diverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move m)
{
    auto action = config->actions.find(cid);
    if(action == config->actions.end())
    {
        log_printf(DEBUG, "0x%x's RawXY was diverted with no action.", cid);
        return;
    }
    switch(action->second->type)
    {
        case Action::Gestures:
            ((GestureAction*)action->second)->move(m);
            break;
        default:
            break;
    }
}

std::map<uint16_t, uint8_t> Device::get_features()
{
    std::map<uint16_t, uint8_t> _features;
    HIDPP20::IFeatureSet ifs (hidpp_dev);
    uint8_t feature_count = ifs.getCount();

    for(uint8_t i = 0; i < feature_count; i++)
        _features.insert( {i, ifs.getFeatureID(i) } );

    return _features;
}