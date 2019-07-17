#include <cstdint>
#include <future>
#include <unistd.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp20/IAdjustableDPI.h>
#include <hidpp20/IFeatureSet.h>
#include <hidpp20/Error.h>
#include <hidpp20/IReprogControlsV4.h>
#include <hidpp20/Device.h>
#include <hidpp10/Error.h>
#include <algorithm>
#include <cstring>

#include "Device.h"
#include "Actions.h"
#include "Configuration.h"
#include "util.h"
#include "EvdevDevice.h"

using namespace std::chrono_literals;

Device::Device(std::string p, const HIDPP::DeviceIndex i) : path(p), index (i)
{
    DeviceRemoved = false;
    dispatcher = new HIDPP::SimpleDispatcher(path.c_str());
    hidpp_dev = new HIDPP20::Device(dispatcher, index);
    features = get_features();

    if(global_config->devices.find(hidpp_dev->name()) == global_config->devices.end())
    {
        log_printf(INFO, "Device %s not configured, using default.", hidpp_dev->name().c_str());
        config = new DeviceConfig();
    }
    else config = global_config->devices.find(hidpp_dev->name())->second;
}

void Device::configure(bool scanning)
{
    if(config->dpi != nullptr)
        set_dpi(*config->dpi, scanning);
    if(config->smartshift != nullptr)
        set_smartshift(*config->smartshift, scanning);
    if(config->hiresscroll != nullptr)
        set_hiresscroll(*config->hiresscroll, scanning);
    divert_buttons();
}

void Device::divert_buttons(bool scanning)
{
    HIDPP20::IReprogControlsV4 irc4(hidpp_dev);
    for(auto it = config->actions.begin(); it != config->actions.end(); ++it)
    {
        uint8_t flags = 0;
        flags |= HIDPP20::IReprogControlsV4::ChangeTemporaryDivert;
        flags |= HIDPP20::IReprogControlsV4::TemporaryDiverted;
        if(it->second->type == Action::Gestures)
        {
            flags |= HIDPP20::IReprogControlsV4::ChangeRawXYDivert;
            flags |= HIDPP20::IReprogControlsV4::RawXYDiverted;
        }
        it->first;
        irc4.setControlReporting(it->first, flags, it->first);
    }
}

void Device::set_smartshift(smartshift_options ops, bool scanning)
{
    std::vector<uint8_t> parameters;
    parameters.push_back(ops.on == nullptr ? 0 : (*ops.on)? 2 : 1);
    if(ops.threshold != nullptr)
    {
        parameters.push_back(*ops.threshold);
        parameters.push_back(*ops.threshold);
    }

    if(features.find(0x2110) == features.end())
    {
        log_printf(DEBUG, "Error toggling smart shift, feature is non-existent.");
        return;
    }
    const uint8_t f_index = features.find(0x2110)->second;

    try { hidpp_dev->callFunction(f_index, 0x01, parameters); }
    catch (HIDPP20::Error &e)
    {
        if(scanning)
            throw e;
        log_printf(ERROR, "Error setting smartshift options, code %d: %s\n", e.errorCode(), e.what());
    }
}

void Device::set_hiresscroll(bool b, bool scanning)
{

}

void Device::set_dpi(int dpi, bool scanning)
{
    HIDPP20::IAdjustableDPI iad(hidpp_dev);

    try { for(int i = 0; i < iad.getSensorCount(); i++) iad.setSensorDPI(i, dpi); }
    catch (HIDPP20::Error &e)
    {
        if(scanning)
            throw e;
        log_printf(ERROR, "Error while setting DPI: %s", e.what());
    }
}

void Device::start()
{
    configure();

    auto *d = new HIDPP::SimpleDispatcher(path.c_str());
    listener = new SimpleListener(d, index);
    listener->addEventHandler( std::make_unique<ButtonHandler>(hidpp_dev, this) );
    auto listener_thread = std::thread { [=]() { listener->start(); } };
    listener_thread.detach();
    while(!DeviceRemoved)
    {
        std::mutex m;
        std::condition_variable cv;
        std::vector<uint8_t> results;

        std::thread t([&cv, &results, dev=hidpp_dev, removed=&DeviceRemoved]()
        {
            try { results = dev->callFunction(0x00, 0x00); }
            catch(HIDPP10::Error &e) { usleep(500000); }
            catch(std::system_error &e)
            {
                cv.notify_one();
                if(*removed) printf("REMOVED!\n");
                *removed = true;
            }
            cv.notify_one();
        });
        t.detach();

        std::unique_lock<std::mutex> l(m);
        if(cv.wait_for(l, 500ms) == std::cv_status::timeout)
        {
            while(!DeviceRemoved)
            {
                try
                {
                    configure(true);
                    break;
                }
                catch(std::exception &e) {} // Retry infinitely on failure
            }
        }
        usleep(200000);
    }

    listener->stop();
    listener_thread.join();
}

void Device::ButtonHandler::handleEvent (const HIDPP::Report &event)
{
    switch (event.function())
    {
        case HIDPP20::IReprogControlsV4::Event::DivertedButtonEvent:
        {
            new_states = HIDPP20::IReprogControlsV4::divertedButtonEvent(event);
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
            cids.resize(it - cids.begin());
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
    }
}

void Device::EventListener::removeEventHandlers ()
{
    for (const auto &p: iterators)
        dispatcher->unregisterEventHandler(p.second);
    handlers.clear();
    iterators.clear();
}

Device::EventListener::~EventListener()
{
    removeEventHandlers();
}

void Device::EventListener::addEventHandler(std::unique_ptr<EventHandler> &&handler)
{
    uint8_t feature = handler->feature()->index();
    EventHandler *ptr = handler.get();
    handlers.emplace(feature, std::move(handler));
    dispatcher->registerEventHandler(index, feature, [ptr](const HIDPP::Report &report)
    {
        ptr->handleEvent(report);
        return true;
    });
}

void Device::SimpleListener::start()
{
    try { dispatcher->listen(); }
    catch(std::system_error &e) { }
}

void Device::SimpleListener::stop()
{
    dispatcher->stop();
}

bool Device::SimpleListener::event (EventHandler *handler, const HIDPP::Report &report)
{
    handler->handleEvent (report);
    return true;
}

void Device::stop()
{
    DeviceRemoved = true;
}

void Device::press_button(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(WARN, "0x%x was pressed but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->press();
}

void Device::release_button(uint16_t cid)
{
    if(config->actions.find(cid) == config->actions.end())
    {
        log_printf(WARN, "0x%x was released but no action was found.", cid);
        return;
    }
    config->actions.find(cid)->second->release();
}

void Device::move_diverted(uint16_t cid, HIDPP20::IReprogControlsV4::Move m)
{
    auto action = config->actions.find(cid)->second;
    switch(action->type)
    {
        case Action::Gestures:
            ((GestureAction*)action)->move(m);
            break;
        default:
            break;
    }
}

std::map<uint16_t, uint8_t> Device::get_features()
{
    std::map<uint16_t, uint8_t> features;

    HIDPP20::IFeatureSet ifs (hidpp_dev);

    unsigned int feature_count = ifs.getCount();

    for(unsigned int i = 1; i <= feature_count; i++)
        features.insert({ifs.getFeatureID(i), i});

    return features;
}