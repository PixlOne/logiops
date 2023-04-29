/*
 * Copyright 2019-2023 PixlOne
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
#ifndef LOGID_TASK_H
#define LOGID_TASK_H

#include <util/ExceptionHandler.h>
#include <functional>
#include <memory>
#include <future>

namespace logid {
    /* This function spawns a new task into the least used worker queue
    * and forgets about it.
    */
    void spawn_task(const std::function<void()>& function);
}

#endif //LOGID_TASK_H
