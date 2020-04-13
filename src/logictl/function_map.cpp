#include "logictl.h"

std::map<std::string, void(*)()> logictl::functions =
        {
                {"help", logictl::help},
                {"reload", logictl::reload}
        };