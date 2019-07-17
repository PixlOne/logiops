#include <hid/DeviceMonitor.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp/Device.h>
#include <hidpp10/Error.h>
#include <hidpp20/Error.h>
#include <cstring>
#include <unistd.h>
#include <future>
#include <fstream>
#include <sstream>

#include "DeviceFinder.h"
#include "util.h"
#include "Device.h"

void find_device()
{
    auto df = new DeviceFinder();
    df->run();
}

void DeviceFinder::addDevice(const char *path)
{
    const int max_tries = 5;
    const int try_delay = 50000;
    std::string string_path(path);
    // Asynchronously scan device
    std::thread{[=]()
    {
        // Check /sys/class/hidraw/hidrawX/device/uevent for device details.
        // Continue if HIDRAW_NAME contains 'Logitech' or is non-existent/
        // Otherwise, skip.
        std::string hidraw_name;
        std::size_t spos = string_path.rfind('/');
        if(spos != std::string::npos) hidraw_name = string_path.substr(spos+1, string_path.size()-1);
        else
        {
            log_printf(ERROR, "Expected file but got directory: %s", path);
            return;
        }
        std::ifstream info_file("/sys/class/hidraw/" + hidraw_name + "/device/uevent");
        std::map<std::string, std::string> hidraw_info;
        std::string line;
        while(std::getline(info_file, line))
        {
            if(line.find('=') == std::string::npos) continue;
            hidraw_info.insert({line.substr(0, line.find('=')), line.substr(line.find('=') + 1, line.size()-1)});
        }

        if(hidraw_info.find("HID_NAME") != hidraw_info.end())
            if (hidraw_info.find("HID_NAME")->second.find("Logitech") == std::string::npos) return;

        //Check if device is an HID++ device and handle it accordingly
        try
        {
            HIDPP::SimpleDispatcher dispatcher(string_path.c_str());
            bool has_receiver_index = false;
            for(HIDPP::DeviceIndex index: {
                HIDPP::DefaultDevice, HIDPP::CordedDevice,
                HIDPP::WirelessDevice1, HIDPP::WirelessDevice2,
                HIDPP::WirelessDevice3, HIDPP::WirelessDevice4,
                HIDPP::WirelessDevice5, HIDPP::WirelessDevice6})
            {
                /// TODO: CONTINUOUSLY SCAN ALL DEVICES ON RECEIVER
                //Skip wireless devices if default device (receiver) has failed
                 if(!has_receiver_index && index == HIDPP::WirelessDevice1)
                     break;
                 for(int i = 0; i < max_tries; i++)
                 {
                     try
                     {
                         HIDPP::Device d(&dispatcher, index);
                         auto version = d.protocolVersion();
                         if(index == HIDPP::DefaultDevice && version == std::make_tuple(1, 0))
                             has_receiver_index = true;
                         uint major, minor;
                         std::tie(major, minor) = d.protocolVersion();
                         if(major > 1) // HID++ 2.0 devices only
                         {
                             auto dev = new Device(string_path.c_str(), index);
                             handlers.insert({
                                     dev, std::async(std::launch::async, &Device::start, *dev)
                             });
                             log_printf(INFO, "%s detected: device %d on %s", d.name().c_str(), index, string_path.c_str());
                         }
                         break;
                     }
                     catch(HIDPP10::Error &e)
                     {
                         if(e.errorCode() != HIDPP10::Error::UnknownDevice)
                         {
                             if(i == max_tries - 1)
                                 log_printf(WARN, "Error while querying %s, wireless device %d: %s", string_path.c_str(), index, e.what());
                             else usleep(try_delay);
                         }
                     }
                     catch(HIDPP20::Error &e)
                     {
                         if(e.errorCode() != HIDPP20::Error::UnknownDevice)
                         {
                             if(i == max_tries - 1)
                                 log_printf(WARN, "Error while querying %s, device %d: %s", string_path.c_str(), index, e.what());
                             else usleep(try_delay);
                         }
                     }
                     catch(HIDPP::Dispatcher::TimeoutError &e)
                     {
                         if(i == max_tries - 1)
                            log_printf(WARN, "Device %s (index %d) timed out.", string_path.c_str(), index);
                        else usleep(try_delay);
                     }
                     catch(std::runtime_error &e)
                     {
                         if(i == max_tries - 1)
                            log_printf(WARN, "Runtime error on device %d on %s: %s", index, string_path.c_str(), e.what());
                         else usleep(try_delay);
                     }
                 }
            }
        }
        catch(HIDPP::Dispatcher::NoHIDPPReportException &e) {}
        catch(std::system_error &e) { log_printf(WARN, "Failed to open %s: %s", string_path.c_str(), e.what()); }
    }}.detach();
}

void DeviceFinder::removeDevice(const char* path)
{
    // Iterate through Devices, stop all in path
    auto it = handlers.begin();
    while (it != handlers.end())
    {
        if(it->first->path == path)
        {
            log_printf(INFO, "%s on %s disconnected.", it->first->hidpp_dev->name(), path);
            it->first->stop();
            it->second.wait();
            //handlers.erase(it);
            it++;
        }
        else
            it++;
    }
}