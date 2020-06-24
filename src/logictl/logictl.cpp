/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "logictl.h"

using namespace logictl;

DBus::BusDispatcher* logictl::dispatcher;
DBus::Connection* logictl::bus;

std::string logictl::exec_cmd;
std::vector<std::string> logictl::args;

void logictl::help()
{
    printf(
R"(Usage: %s [command] [args]
Commands:
    help        Show this dialog
    reload      Reloads logid config and rescans devices
)", exec_cmd.c_str());
}

int main(int argc, char** argv)
{
    dispatcher = new DBus::BusDispatcher();
    DBus::default_dispatcher = dispatcher;
    auto _bus = DBus::Connection::SessionBus();
    bus = &_bus;

    exec_cmd = argv[0];

    for(int i = 1; i < argc; i++)
        logictl::args.emplace_back(argv[i]);

    if(!args.empty())
    {
        auto function = functions.find(args[0]);
        if(function != functions.end())
            function->second();
        else
        {
            printf("%s is not a valid command. Run %s help for a list of valid commands\n",
                    args[0].c_str(), exec_cmd.c_str());
            return 1;
        }
    }
    else
        help();

    return 0;
}