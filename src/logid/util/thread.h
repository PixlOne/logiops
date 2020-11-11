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
#ifndef LOGID_THREAD_H
#define LOGID_THREAD_H

#include <functional>
#include <memory>
#include <thread>
#include "ExceptionHandler.h"

namespace logid
{
    class thread
    {
    public:
        explicit thread(const std::function<void()>& function,
                const std::function<void(std::exception&)>&
                exception_handler={[](std::exception& e)
                                   {ExceptionHandler::Default(e);}});

        ~thread();

        /* This function spawns a new thread and forgets about it,
         * safe equivalent to std::thread{...}.detach()
         */
        static void spawn(const std::function<void()>& function,
                const std::function<void(std::exception&)>&
                exception_handler={[](std::exception& e)
                                   {ExceptionHandler::Default(e);}});

        void run();
        void wait();
        void runSync();
    private:
        std::shared_ptr<std::function<void()>> _function;
        std::shared_ptr<std::function<void(std::exception&)>>
            _exception_handler;
        std::shared_ptr<std::thread> _thread = nullptr;
    };
}

#endif //LOGID_THREAD_H
