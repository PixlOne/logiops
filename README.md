# logiops

This is an unofficial driver for Logitech mice and keyboard.

This is currently only compatible with HID++ \>2.0 devices.

## Configuration
[Refer to the wiki for details.](https://github.com/PixlOne/logiops/wiki/Configuration)

You may also refer to logid.example.cfg for an example.

## Dependencies

This project requires a C++14 compiler, cmake, libevdev, libudev, and libconfig. For popular distributions, I've included commands below. 

**Debian/Ubuntu:** `sudo apt install cmake libevdev-dev libudev-dev libconfig++-dev`

**Arch Linux:** `sudo pacman -S cmake libevdev libconfig libudev`

## Building

To build this project, run:

```
mkdir build
cd build
cmake ..
make
```

To install, run `sudo make install` after building. You can set the daemon to start at boot by running `sudo systemctl start logid`.

## Donate
This program is (and will always be) provided free of charge. If you would like to support the development of this project by donating, you can donate to my Ko-Fi below.

<a href='https://ko-fi.com/R6R81QQ9M' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://cdn.ko-fi.com/cdn/kofi1.png?v=2' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>

I'm also looking for contributors to help in my project; feel free to submit a pull request or e-mail me if you would like to contribute.

## Compatible Devices

|    Device    | Compatible? |
|:------------:|:-----------:|
| MX Master 3  |     Yes     |
| MX Master 2S |     Yes     |
|  MX Master   |     Yes     |
| MX Vertical  |     Yes     |
|   MX Ergo    |     Yes     |
|     M720     |     Yes     |
|     T400     |     Yes     |
|    K400r     |  Untested   |
|     K350     |  Untested   |
|    M325c     |  Untested   |

I own the MX Master, T400, K400r, K350, and M325c. Feel free to add to this list if you can test any additional devices.
