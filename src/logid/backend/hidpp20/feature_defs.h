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

#ifndef LOGID_BACKEND_HIDPP20_FEATUREDEFS
#define LOGID_BACKEND_HIDPP20_FEATUREDEFS

#include <cstdint>

namespace logid::backend::hidpp20 {
    struct feature_info {
        uint16_t feature_id;
        bool obsolete;
        bool internal;
        bool hidden;
    };

    namespace FeatureID {
        enum FeatureID : uint16_t {
            ROOT = 0x0000,
            FEATURE_SET = 0x0001,
            FEATURE_INFO = 0x0002,
            FW_VERSION = 0x0003,
            DEVICE_NAME = 0x0005,
            DEVICE_GROUPS = 0x0006,
            DEVICE_FRIENDLY_NAME = 0x0007,
            RESET = 0x0020,
            CRYPTO_IDENTIFIER = 0x0021,
            DFUCONTROL = 0x00c0,
            DFUCONTROL_V2 = 0x00c1,
            DFUCONTROL_V3 = 0x00c2,
            DFU = 0xd000,
            BATTERY_STATUS = 0x1000,
            BATTERY_VOLTAGE = 0x1001,
            CHARGING_CONTROL = 0x1010,
            LED_CONTROL = 0x1300,
            GENERIC_TEST = 0x1800,
            DEVICE_RESET = 0x1802,
            OOB_STATE = 0x1805,
            CONFIGURABLE_DEVICE_PROPERTIES = 0x1806,
            CHANGE_HOST = 0x1814,
            HOSTS_INFO = 0x1815,
            BACKLIGHT = 0x1981,
            BACKLIGHT_V2 = 0x1982,
            BACKLIGHT_V3 = 0x1983,
            PRESENTER_CONTROL = 0x1a00,
            SENSOR_3D = 0x1a01,
            REPROG_CONTROLS = 0x1b00,
            REPROG_CONTROLS_V2 = 0x1b01,
            REPROG_CONTROLS_V2_2 = 0x1b02,
            REPROG_CONTROLS_V3 = 0x1b03,
            REPROG_CONTROLS_V4 = 0x1b04,
            PERSISTENT_REMAPPABLE_ACTION = 0x1bc0,
            WIRELESS_DEVICE_STATUS = 0x1d4b,
            ENABLE_HIDDEN_FEATURE = 0x1e00,
            FIRMWARE_PROPERTIES = 0x1f1f,
            ADC_MEASUREMENT = 0x1f20,
            LEFT_RIGHT_SWAP = 0x2001,
            SWAP_BUTTON = 0x2005,
            POINTER_AXES_ORIENTATION = 0x2006,
            VERTICAL_SCROLLING = 0x2100,
            SMART_SHIFT = 0x2110,
            SMART_SHIFT_V2 = 0x2111,
            HIRES_SCROLLING = 0x2120,
            HIRES_SCROLLING_V2 = 0x2121, // Referred to as Hi-res wheel in cvuchener/hidpp, seems to be V2?
            LORES_SCROLLING = 0x2130,
            THUMB_WHEEL = 0x2150,
            MOUSE_POINTER = 0x2200, // Possibly predecessor to 0x2201?
            ADJUSTABLE_DPI = 0x2201,
            ANGLE_SNAPPING = 0x2230,
            SURFACE_TUNING = 0x2240,
            HYBRID_TRACKING = 0x2400,
            FN_INVERSION = 0x40a0,
            FN_INVERSION_V2 = 0x40a2, // Is 0x40a1 skipped?
            FN_INVERSION_V3 = 0x40a3,
            ENCRYPTION = 0x4100,
            LOCK_KEY_STATE = 0x4220,
            SOLAR_DASHBOARD = 0x4301,
            KEYBOARD_LAYOUT = 0x4520,
            KEYBOARD_DISABLE = 0x4521,
            DISABLE_KEYS = 0x4522,
            MULTIPLATFORM = 0x4530, // Dual platform only?
            MULTIPLATFORM_V2 = 0x4531,
            KEYBOARD_LAYOUT_V2 = 0x4540,
            CROWN = 0x4600,
            TOUCHPAD_FW = 0x6010,
            TOUCHPAD_SW = 0x6011,
            TOUCHPAD_FW_WIN8 = 0x6012,
            TOUCHMOUSE_RAW = 0x6100,
            // TOUCHMOUSE_6120 = 0x6120, (Keeping this commented out until a better name is found)
            GESTURE = 0x6500,
            GESTURE_V2 = 0x6501,
            G_KEY = 0x8010,
            M_KEY = 0x8020,
            // MR = 0x8030, (Keeping this commented out until a better name is found)
            BRIGHTNESS_CONTROL = 0x8040,
            REPORT_RATE = 0x8060,
            RGB_EFFECTS = 0x8070,
            RGB_EFFECTS_V2 = 0x8071,
            PER_KEY_LIGHTING = 0x8080,
            PER_KEY_LIGHTING_V2 = 0x8081,
            MODE_STATUS = 0x8100,
            MOUSE_BUTTON_SPY = 0x8110,
            LATENCY_MONITORING = 0x8111,
            GAMING_ATTACHMENTS = 0x8120,
            FORCE_FEEDBACK = 0x8123,
            SIDETONE = 0x8300,
            EQUALIZER = 0x8310,
            HEADSET_OUT = 0x8320
        };
    }

}

#endif //LOGID_BACKEND_HIDPP20_FEATUREDEFS