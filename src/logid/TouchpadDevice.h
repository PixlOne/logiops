#ifndef LOGID_TOUCHPADDEVICE_H
#define LOGID_TOUCHPADDEVICE_H

#include <mutex>
#include <string>
#include <vector>

struct libevdev;
struct libevdev_uinput;

namespace logid {
    class TouchpadDevice {
    public:
        /* Units per millimetre advertised to libinput. 40 u/mm is ~1016 u/in,
         * so one count of a 1000 DPI mouse moves the fingers about as far
         * as the mouse itself moved. Without a resolution libinput guesses a
         * 71x50 mm pad, which caps every gesture at a few centimetres. */
        static constexpr int resolution = 40;
        /* Contacts start in the middle of a 2 m square, so a gesture can
         * travel a metre in any direction before it reaches an edge. */
        static constexpr int size_mm = 2000;
        static constexpr int range = resolution * size_mm;
        static constexpr int default_slots = 10;

        explicit TouchpadDevice(const char* name, int slots = default_slots);

        ~TouchpadDevice();

        [[nodiscard]] int slots() const;

        void beginContacts(int fingers);
        void moveContacts(int dx, int dy);
        void endContacts();

    private:
        struct Contact {
            int x;
            int y;
            int tracking_id;
        };

        void _createDevice();
        void _destroyDevice();
        void _sendEvent(uint type, uint code, int value);
        void _sendSyn();
        void _setToolButtons(int fingers, bool down);
        void _endContactsLocked();

        std::mutex _input_mutex;
        std::string _name;
        libevdev* _device{};
        libevdev_uinput* _ui_device{};
        int _max_slots;
        bool _active;
        int _active_fingers;
        int _next_tracking_id;
        std::vector<Contact> _contacts;
    };
}

#endif // LOGID_TOUCHPADDEVICE_H
