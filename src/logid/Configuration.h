#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <map>
#include <libconfig.h++>
#include <hidpp20/ISmartShift.h>
#include "Actions.h"

class DeviceConfig;
class ButtonAction;
enum class Action;

class DeviceConfig
{
public:
    DeviceConfig();
    DeviceConfig(DeviceConfig* dc, Device* dev);
    DeviceConfig(const libconfig::Setting& root);
    const int* dpi = nullptr;
    HIDPP20::ISmartShift::SmartshiftStatus* smartshift;
    const uint8_t* hiresscroll = nullptr;
    std::map<uint16_t, ButtonAction*> actions;
    const bool baseConfig = true;
};

class Configuration
{
public:
    Configuration(const char* config_file);
    std::map<std::string, DeviceConfig*> devices;
private:
    libconfig::Config cfg;
};

ButtonAction* parse_action(Action action, const libconfig::Setting* action_config, bool is_gesture=false);

extern Configuration* global_config;

#endif //CONFIGURATION_H