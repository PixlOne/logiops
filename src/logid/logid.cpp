#include <hidpp/SimpleDispatcher.h>
#include <hidpp/DispatcherThread.h>
#include <hidpp20/Device.h>
#include <hidpp20/Error.h>
#include <hidpp20/IReprogControls.h>
#include <hidpp20/UnsupportedFeature.h>
#include <hid/DeviceMonitor.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "util.h"
#include "Device.h"
#include "Actions.h"
#include "Configuration.h"
#include "EvdevDevice.h"
#include "DeviceFinder.h"

#define evdev_name "logid"

LogLevel global_verbosity = DEBUG;
Configuration* global_config;
EvdevDevice* global_evdev;

int main(int argc, char** argv)
{
    // Read config
    try { global_config = new Configuration("logid.cfg"); }
    catch (std::exception &e) { return EXIT_FAILURE; }

    //Create an evdev device called 'logid'
    try { global_evdev = new EvdevDevice(evdev_name); }
    catch(std::system_error& e)
    {
        log_printf(ERROR, "Could not create evdev device: %s", e.what());
        return EXIT_FAILURE;
    }

    find_device(); // Scan devices, create listeners, handlers, etc.

    return EXIT_SUCCESS;
}