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
#include "task.h"
#include "workqueue.h"

using namespace logid;

task::task(const std::function<void()>& function,
     const std::function<void(std::exception&)>& exception_handler) :
     _function (std::make_shared<std::function<void()>>(function)),
     _exception_handler (std::make_shared<std::function<void(std::exception&)>>
             (exception_handler)), _status (Waiting),
     _task_pkg ([this](){
         try {
             (*_function)();
         } catch(std::exception& e) {
             (*_exception_handler)(e);
         }
     }), _future (_task_pkg.get_future())
{
}

void task::run()
{
    _status = Running;
    _status_cv.notify_all();
    _task_pkg();
    _status = Completed;
    _status_cv.notify_all();
}

task::Status task::getStatus()
{
    return _status;
}

void task::wait()
{
    if(_future.valid())
        _future.wait();
    else {
        std::mutex wait_start;
        std::unique_lock<std::mutex> lock(wait_start);
        _status_cv.wait(lock, [this](){ return _status == Completed; });
    }
}

void task::waitStart()
{
    std::mutex wait_start;
    std::unique_lock<std::mutex> lock(wait_start);
    _status_cv.wait(lock, [this](){ return _status != Waiting; });
}

std::future_status task::waitFor(std::chrono::milliseconds ms)
{
    return _future.wait_for(ms);
}

void task::spawn(std::shared_ptr<workqueue> wq,
                 const std::function<void ()>& function,
                 const std::function<void (std::exception&)>& exception_handler)
{
    auto t = std::make_shared<task>(function, exception_handler);
    wq->queue(t);
}