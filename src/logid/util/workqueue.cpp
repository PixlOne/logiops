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
#include "workqueue.h"
#include "log.h"

using namespace logid;

workqueue::workqueue(std::size_t thread_count) : _manager_thread (
        std::make_unique<thread>(
                [this](){ _run(); }
                , [this](std::exception& e){ _exception_handler(e); }
        )), _continue_run (false), _worker_count (thread_count)
{
    _workers.reserve(_worker_count);
    for(std::size_t i = 0; i < thread_count; i++)
        _workers.push_back(std::make_unique<worker_thread>(this, i));
    _manager_thread->run();
}

workqueue::~workqueue()
{
    stop();

    while(_workers.empty())
        _workers.pop_back();

    // Queue should have been empty before, but just confirm here.
    while(!_queue.empty()) {
        thread::spawn([t=_queue.front()](){ t->run(); });
        _queue.pop();
    }
}

void workqueue::queue(std::shared_ptr<task> t)
{
    _queue.push(t);
    _queue_cv.notify_all();
}

void workqueue::busyUpdate()
{
    _busy_cv.notify_all();
}

void workqueue::stop()
{
    _continue_run = false;
    std::unique_lock<std::mutex> lock(_run_lock);
}

void workqueue::setThreadCount(std::size_t count)
{
    while(_workers.size() < count)
        _workers.push_back(std::make_unique<worker_thread>(this,
            _workers.size()));

    if(_workers.size() > count) {
        // Restart manager thread
        stop();
        while (_workers.size() > count)
            _workers.pop_back();
        _manager_thread = std::make_unique<thread>(
                [this](){ _run(); }
                , [this](std::exception& e){ _exception_handler(e); }
        );
        _manager_thread->run();
    }

    _worker_count = count;
}

std::size_t workqueue::threadCount() const
{
    return _workers.size();
}

void workqueue::_run()
{
    using namespace std::chrono_literals;

    std::unique_lock<std::mutex> lock(_run_lock);
    _continue_run = true;
    while(_continue_run) {
        _queue_cv.wait(lock, [this]{ return !(_queue.empty()); });
        while(!_queue.empty()) {

            if(_workers.empty()) {
                if(_worker_count)
                    logPrintf(DEBUG, "No workers were found, running task in"
                                 " a new thread.");
                thread::spawn([t=_queue.front()](){ t->run(); });
                _queue.pop();
                continue;
            }

            auto worker = _workers.begin();
            for(; worker != _workers.end(); worker++) {
                if(!(*worker)->busy())
                    break;
            }
            if(worker != _workers.end())
                (*worker)->queue(_queue.front());
            else {
                _busy_cv.wait_for(lock, 500ms, [this, &worker]{
                    for(worker = _workers.begin(); worker != _workers.end();
                        worker++) {
                        if (!(*worker)->busy()) {
                            return true;
                        }
                    }
                    return false;
                });

                if(worker != _workers.end())
                    (*worker)->queue(_queue.front());
                else{
                    // Workers busy, launch in new thread
                    logPrintf(DEBUG, "All workers were busy for 500ms, "
                                     "running task in new thread.");
                    thread::spawn([t = _queue.front()]() { t->run(); });
                }
            }
            _queue.pop();
        }
    }
}

void workqueue::_exception_handler(std::exception &e)
{
    logPrintf(WARN, "Exception caught on workqueue manager thread, "
                    "restarting: %s" , e.what());
    // This action destroys the logid::thread, std::thread should detach safely.
    _manager_thread = std::make_unique<thread>([this](){ _run(); },
            [this](std::exception& e) { _exception_handler(e); });
    _manager_thread->run();
}