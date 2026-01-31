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

#include <config/map.h>
#include <config/group.h>
#include <ipcgull/property.h>
#include <libconfig.h++>
#include <type_traits>
#include <optional>
#include <variant>
#include <list>
#include <set>

// Containers are chosen specifically so that no iterator is invalidated.

namespace logid::config {
    void logError(const libconfig::Setting& setting, std::exception& e);

    template<typename T>
    struct config_io {
        static_assert(std::is_base_of<group, T>::value);

        static T get(const libconfig::Setting& parent,
                     const std::string& name) {
            T t{};
            t._load(parent.lookup(name));
            return t;
        }

        static T get(const libconfig::Setting& setting) {
            T t{};
            t._load(setting);
            return t;
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const T& t) {
            if (!parent.exists(name)) {
                parent.add(name, libconfig::Setting::TypeGroup);
            } else if (parent.lookup(name).getType()
                       != libconfig::Setting::TypeGroup) {
                parent.remove(name);
                parent.add(name, libconfig::Setting::TypeGroup);
            }
            t._save(parent.lookup(name));
        }

        static void set(libconfig::Setting& setting, const T& t) {
            t._save(setting);
        }

        static void append(libconfig::Setting& list, const T& t) {
            auto& x = list.add(libconfig::Setting::TypeGroup);
            set(x, t);
        }
    };

    template<typename T>
    struct config_io<ipcgull::property<T>> : public config_io<T> {
    };

    template<typename T, libconfig::Setting::Type TypeEnum>
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
            if (!parent.exists(name)) {
                parent.add(name, TypeEnum);
            } else if (parent.lookup(name).getType() != TypeEnum) {
                parent.remove(name);
                parent.add(name, TypeEnum);
            }
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

    template<typename T, typename O, libconfig::Setting::Type TypeEnum>
    struct reinterpret_io {
        static T get(const libconfig::Setting& parent,
                     const std::string& name) {
            return static_cast<T>(primitive_io<O, TypeEnum>::get(parent, name));
        }

        static T get(const libconfig::Setting& setting) {
            return static_cast<T>(primitive_io<O, TypeEnum>::get(setting));
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const T& t) {
            primitive_io<O, TypeEnum>::set(parent, name,
                                           static_cast<O>(t));
        }

        static void set(libconfig::Setting& setting, const T& t) {
            primitive_io<O, TypeEnum>::set(setting,
                                           static_cast<O>(t));
        }

        [[maybe_unused]]
        static void append(libconfig::Setting& list, const T& t) {
            primitive_io<O, TypeEnum>::append(list,
                                              static_cast<O>(t));
        }
    };

    template<>
    struct config_io<bool> : public primitive_io<bool,
            libconfig::Setting::TypeBoolean> {
    };
    template<>
    struct config_io<int8_t> : public reinterpret_io<int8_t, int,
            libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<uint8_t> : public reinterpret_io<uint8_t, int,
            libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<short> : public reinterpret_io<short, int,
            libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<unsigned short> : public reinterpret_io<unsigned short,
            int, libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<int> : public primitive_io<int,
            libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<unsigned int> : public reinterpret_io<unsigned int,
            int, libconfig::Setting::TypeInt> {
    };
    template<>
    struct config_io<long> : public reinterpret_io<long, long long,
            libconfig::Setting::TypeInt64> {
    };
    template<>
    struct config_io<unsigned long> : public reinterpret_io<unsigned long,
            long long, libconfig::Setting::TypeInt64> {
    };
    template<>
    struct config_io<long long> : public primitive_io<long long,
            libconfig::Setting::TypeInt64> {
    };
    template<>
    struct config_io<unsigned long long> :
            public reinterpret_io<unsigned long long, long long,
                    libconfig::Setting::TypeInt64> {
    };
    template<>
    struct config_io<float> : public primitive_io<float,
            libconfig::Setting::TypeFloat> {
    };
    template<>
    struct config_io<double> : public primitive_io<double,
            libconfig::Setting::TypeFloat> {
        static double get(const libconfig::Setting& setting) {
            switch (setting.getType()) {
                case libconfig::Setting::TypeInt:
                    return (double) (int) setting;
                case libconfig::Setting::TypeInt64:
                    return (double) (long long) setting;
                case libconfig::Setting::TypeFloat:
                default:
                    return (double) setting;
            }
        }

        static double get(const libconfig::Setting& parent,
                          const std::string& name) {
            return get(parent.lookup(name));
        }
    };
    template<>
    struct config_io<std::string> : public primitive_io<std::string,
            libconfig::Setting::TypeString> {
    };

    template<typename... T>
    struct config_io<std::variant<T...>> {
    private:
        template<typename Singleton>
        static std::variant<T...> try_each(const libconfig::Setting& setting) {
            return config_io<Singleton>::get(setting);
        }

        template<typename First, typename Next, typename... Rest>
        static std::variant<T...> try_each(const libconfig::Setting& setting) {
            try {
                return config_io<First>::get(setting);
            } catch (libconfig::SettingException& e) {
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
            std::visit([&setting](auto&& arg) {
                config::set(setting, arg);
            }, t);
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const std::variant<T...>& t) {
            std::visit([&parent, &name](auto&& arg) {
                config::set(parent, name, arg);
            }, t);
        }

        [[maybe_unused]]
        static void append(libconfig::Setting& list, const std::variant<T...>& t) {
            std::visit([&list](auto&& arg) {
                config::append(list, arg);
            }, t);
        }
    };

    template<typename T>
    struct config_io<std::list<T>> {
        static std::list<T> get(const libconfig::Setting& setting) {
            const auto size = setting.getLength();
            std::list<T> t{};
            for (int i = 0; i < size; ++i) {
                try {
                    t.emplace_back(config_io<T>::get(setting[i]));
                } catch (libconfig::SettingException& e) {}
            }
            return t;
        }

        static std::list<T> get(const libconfig::Setting& parent, const std::string& name) {
            return get(parent.lookup(name));
        }

        static void set(libconfig::Setting& setting, const std::list<T>& t) {
            while (setting.getLength() != 0)
                setting.remove((int) 0);
            for (auto& x: t) {
                config_io<T>::append(setting, x);
            }
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const std::list<T>& t) {
            if (!parent.exists(name)) {
                parent.add(name, libconfig::Setting::TypeList);
            } else if (!parent.lookup(name).isList()) {
                parent.remove(name);
                parent.add(name, libconfig::Setting::TypeList);
            }
            set(parent.lookup(name), t);
        }

        [[maybe_unused]]
        static void append(libconfig::Setting& list, const std::list<T>& t) {
            auto& s = list.add(libconfig::Setting::TypeList);
            set(s, t);
        }
    };

    template<typename T>
    struct config_io<std::set<T>> {
        static std::set<T> get(const libconfig::Setting& setting) {
            const auto size = setting.getLength();
            std::set<T> t;
            for (int i = 0; i < size; ++i) {
                try {
                    t.emplace(config_io<T>::get(setting[i]));
                } catch (libconfig::SettingException& e) {}
            }
            return t;
        }

        static std::set<T> get(const libconfig::Setting& parent, const std::string& name) {
            return get(parent.lookup(name));
        }

        static void set(libconfig::Setting& setting, const std::set<T>& t) {
            while (setting.getLength() != 0)
                setting.remove((int) 0);
            for (auto& x: t) {
                auto& s = setting.add(libconfig::Setting::TypeGroup);
                config_io<T>::set(s, x);
            }
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const std::set<T>& t) {
            if (!parent.exists(name)) {
                parent.add(name, libconfig::Setting::TypeList);
            } else if (!parent.lookup(name).isArray()) {
                parent.remove(name);
                parent.add(name, libconfig::Setting::TypeList);
            }
            set(parent.lookup(name), t);
        }

        [[maybe_unused]]
        static void append(libconfig::Setting& list,
                           const std::set<T>& t) {
            auto& s = list.add(libconfig::Setting::TypeList);
            set(s, t);
        }
    };

    template<typename K, typename V, typename KeyName,
            typename Cmp, typename Alloc>
    struct config_io<map<K, V, KeyName, Cmp, Alloc>> {
        static map<K, V, KeyName, Cmp, Alloc> get(const libconfig::Setting& setting) {
            const auto size = setting.getLength();
            map<K, V, KeyName, Cmp, Alloc> t;
            for (int i = 0; i < size; ++i) {
                auto& s = setting[i];
                try {
                    t.emplace(config_io<K>::get(s.lookup(KeyName::value)),
                              config_io<V>::get(s));
                } catch (libconfig::SettingException& e) {}
            }
            return t;
        }

        static map<K, V, KeyName, Cmp, Alloc> get(
                const libconfig::Setting& parent, const std::string& name) {
            return get(parent.lookup(name));
        }

        static void set(libconfig::Setting& setting,
                        const map<K, V, KeyName, Cmp, Alloc>& t) {
            while (setting.getLength() != 0)
                setting.remove((int) 0);
            for (auto& x: t) {
                auto& s = setting.add(libconfig::Setting::TypeGroup);
                config_io<V>::set(s, x.second);
                config_io<K>::set(s, KeyName::value, x.first);
            }
        }

        static void set(libconfig::Setting& parent,
                        const std::string& name,
                        const map<K, V, KeyName, Cmp, Alloc>& t) {
            if (!parent.exists(name)) {
                parent.add(name, libconfig::Setting::TypeList);
            } else if (!parent.lookup(name).isArray()) {
                parent.remove(name);
                parent.add(name, libconfig::Setting::TypeList);
            }
            set(parent.lookup(name), t);
        }

        [[maybe_unused]]
        static void append(libconfig::Setting& list, const map<K, V, KeyName, Cmp, Alloc>& t) {
            auto& s = list.add(libconfig::Setting::TypeList);
            set(s, t);
        }
    };

    template<typename T>
    struct config_io<std::optional<T>> {
        static std::optional<T> get(const libconfig::Setting& parent,
                                    const std::string& name) {
            if (parent.exists(name)) {
                auto& setting = parent.lookup(name);
                try {
                    return config_io<T>::get(setting);
                } catch (libconfig::SettingException& e) {
                    logError(setting, e);
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
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
    template<typename T, typename... Rest>
    struct config_io<std::variant<std::optional<T>, Rest...>> {
        static_assert(!sizeof(std::optional<T>), "Invalid type");
    };

    template<typename T>
    struct config_io<std::list<std::optional<T>>> {
        static_assert(!sizeof(std::optional<T>), "Invalid type");
    };

    template<typename T>
    struct config_io<std::optional<std::optional<T>>> {
        static_assert(!sizeof(std::optional<T>), "Invalid type");
    };

    template<typename T>
    void set(libconfig::Setting& parent,
             const std::string& name,
             const T& t) {
        config_io<T>::set(parent, name, t);
    }

    template<typename T>
    void set(libconfig::Setting& setting, const T& t) {
        config_io<T>::set(setting, t);
    }


    template<typename T>
    void append(libconfig::Setting& list, const T& t) {
        config_io<T>::append(list, t);
    }

    template<typename T>
    auto get(const libconfig::Setting& setting) {
        return config_io<T>::get(setting);
    }

    template<typename T>
    auto get(const libconfig::Setting& parent, const std::string& name) {
        return config_io<T>::get(parent, name);
    }
}

#endif //LOGID_CONFIG_PRIMITIVE_H
