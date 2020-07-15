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
#include "thread.h"

using namespace logid;

thread::thread(const std::function<void()>& function,
        const std::function<void(std::exception&)>& exception_handler)
        : _function (std::make_shared<std::function<void()>>(function)),
        _exception_handler (std::make_shared<std::function<void
        (std::exception&)>> (exception_handler))
{
}

thread::~thread()
{
    if(_thread)
        if(_thread->joinable())
            _thread->detach();
}

void thread::spawn(const std::function<void()>& function,
        const std::function<void(std::exception&)>& exception_handler)
{
    std::thread([function, exception_handler](){
        thread t(function, exception_handler);
        t.runSync();
    }).detach();
}

void thread::run()
{
    _thread = std::make_shared<std::thread>(
            [f=this->_function,eh=this->_exception_handler]() {
        try {
            (*f)();
        } catch (std::exception& e) {
            (*eh)(e);
        }
    });
}

void thread::wait()
{
    if(_thread)
        if(_thread->joinable())
            _thread->join();
}

void thread::runSync()
{
    try {
        (*_function)();
    } catch(std::exception& e) {
        (*_exception_handler)(e);
    }
}