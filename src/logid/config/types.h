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
#ifndef LOGID_CONFIG_PRIMITIVE_H
#define LOGID_CONFIG_PRIMITIVE_H

#include <libconfig.h++>
#include <type_traits>
#include <optional>
#include <variant>
#include "group.h"
#include "map.h"
#include "../util/log.h"

/// TODO: A single element failing should not cause the container to be invalid.

namespace logid::config {
    namespace {
        template <typename T>
        struct config_io {
            static_assert(std::is_base_of<group, T>::value);

            static T get(const libconfig::Setting& parent,
                         const std::string& name) {
                T t {};
                t.load(parent.lookup(name));
                return t;
            }

            static T get(const libconfig::Setting& setting) {
                T t {};
                t.load(setting);
                return t;
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const T& t) {
                if(!parent.exists(name)) {
                    parent.add(name, libconfig::Setting::TypeGroup);
                } else if(parent.lookup(name).getType()
                != libconfig::Setting::TypeGroup) {

                }
                t.save(parent.lookup(name));
            }

            static void set(libconfig::Setting& setting, const T& t) {
                t.save(setting);
            }

            static void append(libconfig::Setting& list, const T& t) {
                auto& x = list.add(libconfig::Setting::TypeGroup);
                set(x, t);
            }
        };

        template <typename T, libconfig::Setting::Type TypeEnum>
        struct primitive_io {
            static T get(const libconfig::Setting& parent,
                         const std::string& name) {
                return parent.lookup(name);
            }

            static T get(const libconfig::Setting& setting) {
                return setting;
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const T& t) {
                if(!parent.exists(name))
                    parent.add(name, TypeEnum);
                set(parent.lookup(name), t);
            }

            static void set(libconfig::Setting& setting, const T& t) {
                setting = t;
            }

            static void append(libconfig::Setting& list, const T& t) {
                auto& x = list.add(TypeEnum);
                set(x, t);
            }
        };

        template <>
        struct config_io<bool> : public primitive_io<bool,
                libconfig::Setting::TypeBoolean> { };
        template <>
        struct config_io<int8_t> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<uint8_t> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<short> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<unsigned short> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<int> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<unsigned int> : public primitive_io<int,
                libconfig::Setting::TypeInt> { };
        template <>
        struct config_io<long> : public primitive_io<long,
                libconfig::Setting::TypeInt64> { };
        template <>
        struct config_io<unsigned long> : public primitive_io<long,
                libconfig::Setting::TypeInt64> { };
        template <>
        struct config_io<long long> : public primitive_io<long long,
                libconfig::Setting::TypeInt64> { };
        template <>
        struct config_io<unsigned long long> : public primitive_io<long long,
                libconfig::Setting::TypeInt64> { };
        template <>
        struct config_io<float> : public primitive_io<float,
                libconfig::Setting::TypeFloat> { };
        template <>
        struct config_io<double> : public primitive_io<double,
                libconfig::Setting::TypeFloat> { };
        template <>
        struct config_io<std::string> : public primitive_io<std::string,
                libconfig::Setting::TypeString> { };

        template <typename... T>
        struct config_io<std::variant<T...>> {
        private:
            template <typename Singleton>
            static std::variant<T...> try_each(
                    const libconfig::Setting& setting) {
                try {
                    return config_io<Singleton>::get(setting);
                } catch(libconfig::SettingTypeException& e) {
                    throw;
                }
            }

            template <typename First, typename Next, typename... Rest>
            static std::variant<T...> try_each(
                    const libconfig::Setting& setting) {
                try {
                    return config_io<First>::get(setting);
                } catch(libconfig::SettingException& e) {
                    return try_each<Next, Rest...>(setting);
                }
            }
        public:
            static std::variant<T...> get(const libconfig::Setting& setting) {
                return try_each<T...>(setting);
            }

            static std::variant<T...> get(const libconfig::Setting& parent,
                                          const std::string& name) {
                return get(parent.lookup(name));
            }

            static void set(libconfig::Setting& setting,
                            const std::variant<T...>& t) {
                std::visit([&setting](auto&& arg){
                    config::set(setting, arg);
                }, t);
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const std::variant<T...>& t) {
                std::visit([&parent, &name](auto&& arg){
                    config::set(parent, name, arg);
                }, t);
            }

            static void append(libconfig::Setting& list,
                               const std::variant<T...>& t) {
                std::visit([&list](auto&& arg){
                    config::append(list, arg);
                }, t);
            }
        };

        template <typename T>
        struct config_io<std::vector<T>> {
            static std::vector<T> get(const libconfig::Setting& setting) {
                const auto size = setting.getLength();
                std::vector<T> t(size);
                for(int i = 0; i < size; ++i)
                    t[i] = config_io<T>::get(setting[i]);
                return t;
            }

            static std::vector<T> get(const libconfig::Setting& parent,
                                      const std::string& name) {
                return get(parent.lookup(name));
            }

            static void set(libconfig::Setting& setting,
                            const std::vector<T>& t) {
                const auto orig_size = setting.getLength();
                for(int i = 0; i < orig_size; ++i)
                    config_io<T>::set(setting[i], t[i]);
                for(int i = orig_size; i < t.size(); ++i)
                    config_io<T>::append(setting, t[i]);
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const std::vector<T>& t) {
                if (!parent.exists(name)) {
                    parent.add(name, libconfig::Setting::TypeArray);
                } else if(!parent.lookup(name).isArray()) {
                    parent.remove(name);
                    parent.add(name, libconfig::Setting::TypeArray);
                }
                set(parent.lookup(name), t);
            }

            static void append(libconfig::Setting& list,
                               const std::vector<T>& t) {
                auto& s = list.add(libconfig::Setting::TypeArray);
                set(s, t);
            }
        };

        template <typename K, typename V, string_literal KeyName>
        struct config_io<map<K, V, KeyName>> {
            static map<K, V, KeyName> get(const libconfig::Setting& setting) {
                const auto size = setting.getLength();
                map<K, V, KeyName> t;
                for(int i = 0; i < size; ++i) {
                    auto& s = setting[i];
                    t.emplace(config_io<K>::get(s.lookup(KeyName.value)),
                              config_io<V>::get(s));
                }
                return t;
            }

            static map<K, V, KeyName> get(const libconfig::Setting& parent,
                                      const std::string& name) {
                return get(parent.lookup(name));
            }

            static void set(libconfig::Setting& setting,
                            const map<K, V, KeyName>& t) {
                while(setting.getLength() != 0)
                    setting.remove((int)0);
                for(auto& x : t) {
                    auto& s = setting.add(libconfig::Setting::TypeGroup);
                    config_io<V>::set(s, x.second);
                    config_io<K>::set(s, KeyName.value, x.first);
                }
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const map<K, V, KeyName>& t) {
                if (!parent.exists(name)) {
                    parent.add(name, libconfig::Setting::TypeList);
                } else if(!parent.lookup(name).isArray()) {
                    parent.remove(name);
                    parent.add(name, libconfig::Setting::TypeList);
                }
                set(parent.lookup(name), t);
            }

            static void append(libconfig::Setting& list,
                               const map<K, V, KeyName>& t) {
                auto& s = list.add(libconfig::Setting::TypeList);
                set(s, t);
            }
        };

        template <typename... T>
        struct config_io<std::vector<std::variant<T...>>> {
            static std::vector<std::variant<T...>> get(
                    const libconfig::Setting& setting) {
                const auto size = setting.getLength();
                std::vector<std::variant<T...>> t(size);
                for(int i = 0; i < size; ++i)
                    t[i] = config_io<std::variant<T...>>::get(setting[i]);
                return t;
            }

            static std::vector<std::variant<T...>> get(
                    const libconfig::Setting& parent, const std::string& name) {
                return get(parent.lookup(name));
            }

            static void set(libconfig::Setting& setting,
                            const std::vector<std::variant<T...>>& t) {
                while(setting.getLength() != 0)
                    setting.remove((int)0);
                for(int i = 0; i < t.size(); ++i)
                    config_io<std::variant<T...>>::append(setting, t[i]);
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const std::vector<std::variant<T...>>& t) {
                if (!parent.exists(name)) {
                    parent.add(name, libconfig::Setting::TypeList);
                } else if(!parent.lookup(name).isList()) {
                    parent.remove(name);
                    parent.add(name, libconfig::Setting::TypeList);
                }
                set(parent.lookup(name), t);
            }

            static void append(libconfig::Setting& list,
                               const std::variant<std::variant<T...>>& t) {
                auto& s = list.add(libconfig::Setting::TypeList);
                set(s, t);
            }
        };

        template <typename T>
        struct config_io<std::optional<T>> {
            static std::optional<T> get(const libconfig::Setting& parent,
                                      const std::string& name) {
                try {
                    return config_io<T>::get(parent.lookup(name));
                } catch(libconfig::SettingException& e) {
                    return {};
                }
            }

            static void set(libconfig::Setting& parent,
                            const std::string& name,
                            const std::optional<T>& t) {
                if (t.has_value())
                    config_io<T>::set(parent, name, t.value());
            }
        };

        // Optionals may not appear as part of a list or array
        template <typename T, typename... Rest>
        struct config_io<std::variant<std::optional<T>, Rest...>> {
            static_assert(!sizeof(std::optional<T>), "Invalid type");
        };

        template <typename T>
        struct config_io<std::vector<std::optional<T>>> {
            static_assert(!sizeof(std::optional<T>), "Invalid type");
        };

        template <typename T>
        struct config_io<std::optional<std::optional<T>>> {
            static_assert(!sizeof(std::optional<T>), "Invalid type");
        };
    }

    template <typename T>
    void set(libconfig::Setting& parent,
             const std::string& name,
             const T& t) {
        config_io<T>::set(parent, name, t);
    }

    template <typename T>
    void set(libconfig::Setting& setting, const T& t) {
        config_io<T>::set(setting, t);
    }


    template <typename T>
    void append(libconfig::Setting& list, const T& t) {
        config_io<T>::set(list, t);
    }

    template <typename T>
    T get(const libconfig::Setting& setting) {
        return config_io<T>::get(setting);
    }

    template <typename T>
    T get(const libconfig::Setting& parent, const std::string& name) {
        return config_io<T>::get(parent, name);
    }
}

#endif //LOGID_CONFIG_PRIMITIVE_H
