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
#include <system_error>
#include "../util.h"
#include "ExceptionHandler.h"
#include "../backend/hidpp10/Error.h"
#include "../backend/hidpp20/Error.h"

using namespace logid;

void ExceptionHandler::Default(std::exception& error)
{
    try {
        throw error;
    } catch(backend::hidpp10::Error& e) {
        log_printf(WARN, "HID++ 1.0 error ignored on detached thread: %s",
                error.what());
    } catch(backend::hidpp20::Error& e) {
        log_printf(WARN, "HID++ 2.0 error ignored on detached thread: %s",
                   error.what());
    } catch(std::system_error& e) {
        log_printf(WARN, "System error ignored on detached thread: %s",
                   error.what());
    } catch(std::exception& e) {
        log_printf(WARN, "Error ignored on detached thread: %s",
                   error.what());
    }
}