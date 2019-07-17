#ifndef MASTEROPTIONS_CONFIGURATION_H
#define MASTEROPTIONS_CONFIGURATION_H

#include <map>
#include <libconfig.h++>

struct smartshift_options
{
    bool* on = nullptr;
    uint8_t* threshold = nullptr;
};

class DeviceConfig;
class ButtonAction;
enum class Action;

class DeviceConfig
{
public:
    DeviceConfig();
    DeviceConfig(const libconfig::Setting& root);
    const int* dpi = nullptr;
    struct smartshift_options* smartshift = nullptr;
    const bool* hiresscroll = nullptr;
    std::map<uint16_t, ButtonAction*> actions;
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

#endif //MASTEROPTIONS_CONFIGURATION_H
