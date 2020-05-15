# logiops

This is an unofficial driver for Logitech mice and keyboard.

This is currently only compatible with HID++ \>2.0 devices.

## Configuration
[Refer to the wiki for details.](https://github.com/PixlOne/logiops/wiki/Configuration)

You may also refer to logid.example.cfg for an example.

## Building

This project requires a C++14 compiler, cmake, libevdev, libudev, and libconfig

To build this project, run:

```
mkdir build
cd build
cmake ..
make
```

To install, run `sudo make install` after building. You can set the daemon to start at boot by running `sudo systemctl start logid`.

## Compatible Devices

|  Device   | Compatible? |
|:---------:|:-----------:|
| MX Master |     Yes     |
|MX Vertical|     Yes     |
|   T400    |     Yes     |
|   K400r   |  Untested   |
|   K350    |  Untested   |
|   M325c   |  Untested   |

I own the MX Master, T400, K400r, K350, and M325c. Feel free to add to this list if you can test any additional devices.
