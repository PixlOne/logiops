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
#ifndef LOGID_WORKER_THREAD_H
#define LOGID_WORKER_THREAD_H

#include "mutex_queue.h"
#include "task.h"
#include "thread.h"

namespace logid
{
    class workqueue;

    class worker_thread
    {
    public:
        worker_thread(workqueue* parent, std::size_t worker_number);
        ~worker_thread();

        void queue(std::shared_ptr<task> t);

        bool busy();
    private:
        void _run();
        void _exception_handler(std::exception& e);

        workqueue* _parent;
        std::size_t _worker_number;

        std::mutex _run_lock;
        std::atomic<bool> _continue_run;
        std::condition_variable _queue_cv;

        std::unique_ptr<thread> _thread;
        std::mutex _busy;

        mutex_queue<std::shared_ptr<task>> _queue;
    };
}

#endif //LOGID_WORKER_THREAD_H
