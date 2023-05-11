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
#ifndef LOGID_CONFIG_GROUP_H
#define LOGID_CONFIG_GROUP_H

#include <libconfig.h++>
#include <type_traits>
#include <functional>
#include <utility>

namespace logid::config {
    template<typename T>
    void set(libconfig::Setting& parent,
             const std::string& name,
             const T& t);

    template<typename T>
    void set(libconfig::Setting& parent, const T& t);

    template<typename T>
    auto get(const libconfig::Setting& parent, const std::string& name);

    template<typename T>
    void append(libconfig::Setting& list, const T& t);

    template<typename T, typename... M>
    struct group_io {
    };

    template<typename T>
    struct group_io<T> {
        static void get(const libconfig::Setting&, T*,
                        const std::vector<std::string>&, const std::size_t) {}

        static void set(libconfig::Setting&, const T*,
                        const std::vector<std::string>&, const std::size_t) {}
    };

    template<typename T, typename A, typename... M>
    struct group_io<T, A, M...> {
        static void get(const libconfig::Setting& s, T* t,
                        const std::vector<std::string>& names,
                        const std::size_t index, A T::* arg, M T::*... rest) {
            auto& x = t->*(arg);
            A old{x};
            try {
                x = config::get<A>(s, names[index]);
                group_io<T, M...>::get(s, t, names, index + 1, rest...);
            } catch (libconfig::SettingTypeException& e) {
                x = old;
                throw;
            } catch (libconfig::SettingException& e) {
                x = old;
                throw libconfig::SettingTypeException(s);
            }
        }

        static void set(libconfig::Setting& s, const T* t,
                        const std::vector<std::string>& names,
                        const std::size_t index, A T::* arg, M T::*... rest) {
            config::set(s, names[index], t->*(arg));
            group_io<T, M...>::set(s, t, names, index + 1, rest...);
        }
    };

    template<typename Sign>
    struct signed_group;

    struct group {
    private:
        const std::vector<std::string> _names;
        const std::function<void(const libconfig::Setting&, group*,
                                 const std::vector<std::string>&)> _getter;
        const std::function<void(libconfig::Setting&, const group*,
                                 const std::vector<std::string>&)> _setter;

        template<typename Sign>
        friend
        struct signed_group;
    protected:
        template<typename T, typename... M>
        explicit group(const std::array<std::string, sizeof...(M)>& names,
                       M T::*... args) :
                _names(names.begin(), names.end()),
                _getter([args...](const libconfig::Setting& s, group* g,
                                  const std::vector<std::string>& names) {
                    T* t = dynamic_cast<T*>(g);
                    group_io<T, M...>::get(s, t, names, 0, args...);
                }),
                _setter([args...](libconfig::Setting& s, const group* g,
                                  const std::vector<std::string>& names) {
                    const T* t = dynamic_cast<const T*>(g);
                    group_io<T, M...>::set(s, t, names, 0, args...);
                }) {
            static_assert(std::is_base_of<group, T>::value);
        }

        group() : _getter([](const libconfig::Setting&, group*,
                             const std::vector<std::string>&) {}),
                  _setter([](libconfig::Setting&, const group*,
                             const std::vector<std::string>&) {}) {}

    public:
        group(const group& o) = default;

        group(group&& o) noexcept = default;

        group& operator=(const group&) {
            return *this;
        }

        group& operator=(group&&) noexcept {
            return *this;
        }

        virtual ~group() = default;

        virtual void _save(libconfig::Setting& setting) const {
            _setter(setting, this, _names);
        }

        virtual void _load(const libconfig::Setting& setting) {
            _getter(setting, this, _names);
        }
    };

    template<typename T>
    struct normalize_signature {
        static const T& make(const T& ret) { return ret; }
    };

    template<>
    struct normalize_signature<std::string> {
        static std::string make(const std::string& data) {
            std::string ret = data;
            std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
            return ret;
        }
    };

    template<typename Sign>
    struct signed_group : public group {
    private:
        const std::string _sig_field;
        const Sign _signature;
    protected:
        signed_group(std::string sign_name, const Sign& sign_data) :
                group(), _sig_field(std::move(sign_name)),
                _signature(normalize_signature<Sign>::make(sign_data)) {}

        template<typename T, typename... M>
        signed_group(
                std::string sign_name, const Sign& sign_data,
                const std::array<std::string, sizeof...(M)>& names,
                M T::*... args) : group(names, args...),
                                  _sig_field(std::move(sign_name)),
                                  _signature(normalize_signature<Sign>::make(sign_data)) {}

    public:
        signed_group(const signed_group& o) = default;

        signed_group(signed_group&& o) noexcept = default;

        signed_group& operator=(const signed_group&) {
            return *this;
        }

        signed_group& operator=(signed_group&&) noexcept {
            return *this;
        }

        void _save(libconfig::Setting& setting) const override {
            set(setting, _sig_field, _signature);
            _setter(setting, this, _names);
        }

        void _load(const libconfig::Setting& setting) override {
            if (normalize_signature<Sign>::make(get<Sign>(setting, _sig_field))
                != _signature)
                throw libconfig::SettingTypeException(setting);
            _getter(setting, this, _names);
        }
    };

}

#endif //LOGID_CONFIG_GROUP_H
