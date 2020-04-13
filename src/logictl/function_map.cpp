#include "logictl.h"

std::map<std::string, void(*)()> logictl::functions =
        {
                {"reload", &(logictl::reload)}
        };