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
#include <vector>
#include "worker_thread.h"
#include "log.h"
#include "workqueue.h"

using namespace logid;

worker_thread::worker_thread(workqueue* parent, std::size_t worker_number) :
_parent (parent), _worker_number (worker_number), _continue_run (false),
_thread (std::make_unique<thread> ([this](){
    _run(); }, [this](std::exception& e){ _exception_handler(e); }))
{
    _thread->run();
}

worker_thread::~worker_thread()
{
    _continue_run = false;
    _queue_cv.notify_all();
    // Block until task is complete
    std::unique_lock<std::mutex> lock(_busy);

    while(!_queue.empty()) {
        _parent->queue(_queue.front());
        _queue.pop();
    }
}

void worker_thread::queue(const std::shared_ptr<task>& t)
{
    _queue.push(t);
    _queue_cv.notify_all();
}

bool worker_thread::busy()
{
    bool not_busy = _busy.try_lock();

    if(not_busy)
        _busy.unlock();

    return !not_busy;
}

void worker_thread::_run()
{
    std::unique_lock<std::mutex> lock(_run_lock);
    _continue_run = true;
    while(_continue_run) {
        _parent->busyUpdate();
        _queue_cv.wait(lock, [this]{ return !_queue.empty() ||
            !_continue_run; });
        if(!_continue_run)
            return;
        std::unique_lock<std::mutex> busy_lock(_busy);
        while(!_queue.empty()) {
            _queue.front()->run();
            _queue.pop();
        }
        _parent->notifyFree();
    }
}

void worker_thread::_exception_handler(std::exception &e)
{
    logPrintf(WARN, "Exception caught on worker thread %d, restarting: %s",
            _worker_number, e.what());
    // This action destroys the logid::thread, std::thread should detach safely.
    _thread = std::make_unique<thread>([this](){ _run(); },
            [this](std::exception& e) { _exception_handler(e); });
    _thread->run();
}