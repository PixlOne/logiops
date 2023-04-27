/*
 * Copyright 2022 PixlOne
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
#ifndef LOGID_CONFIG_MAP_H
#define LOGID_CONFIG_MAP_H

#include "group.h"
#include <map>
#include <optional>
#include <utility>

namespace logid::config {
    template<size_t N>
    struct string_literal {
        constexpr string_literal(const char (& str)[N]) {
            std::copy_n(str, N, value);
        }

        char value[N]{};
    };

    template<class T>
    struct less_caseless {
        constexpr bool operator()(const T& a, const T& b) const noexcept {
            auto a_it = a.begin(), b_it = b.begin();
            for (; a_it != a.end() && b_it != b.end(); ++a_it, ++b_it) {
                if (tolower(*a_it) != tolower(*b_it))
                    return tolower(*a_it) < tolower(*b_it);
            }
            return b_it != b.end();
        }
    };

    // Warning: map must be a variant of groups or a group
    template<typename K, typename V, string_literal KeyName,
            typename Compare=typename std::map<K, V>::key_compare,
            typename Allocator=typename std::map<K, V>::allocator_type>
    class map : public std::map<K, V, Compare, Allocator> {
    public:
        template<typename... Args>
        explicit map(Args... args) :
                std::map<K, V, Compare, Allocator>(std::forward<Args>(args)...) {}
    };
}

#endif //LOGID_CONFIG_MAP_H
