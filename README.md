# logiops

This is an unofficial driver for Logitech mice and keyboard.

This is currently only compatible with HID++ \>2.0 devices.

## Installation

Logiops is currently available in packaged form for the following distributions:

**Debian (Sid only)** `sudo apt install logiops`

**Arch Linux:**: `sudo pacman -S logiops-git`

**Fedora (>=32)**: `sudo dnf install logiops`

All packages install logiops with the logid service in a disabled state so be sure to enable it with `sudo systemctl enable --now logid` after you have configured it.

## Configuration
[Refer to the wiki for details.](https://github.com/PixlOne/logiops/wiki/Configuration)

You may also refer to [logid.example.cfg](./logid.example.cfg) for an example.

Default location for the configuration file is /etc/logid.cfg, but another can be specified using the `-c` flag.

## Build Dependencies

This project requires a C++14 compiler, `cmake`, `libevdev`, `libudev`, and `libconfig`. For popular distributions, I've included commands below.

**Arch Linux:** `sudo pacman -S cmake libevdev libconfig pkgconf`

**Debian/Ubuntu:** `sudo apt install cmake libevdev-dev libudev-dev libconfig++-dev`

**Fedora:** `sudo dnf install cmake libevdev-devel systemd-devel libconfig-devel gcc-c++`

**Gentoo Linux:** `sudo emerge dev-libs/libconfig dev-libs/libevdev dev-util/cmake virtual/libudev`

**Solus:** `sudo eopkg install libevdev-devel libconfig-devel libgudev-devel`

## Building

To build this project, run:

```bash
mkdir build
cd build
cmake ..
make
```

To install, run the following after building:
```bash
sudo make install
sudo systemctl daemon-reload
```

You can set the daemon to start at boot by running `sudo systemctl enable logid` or `sudo systemctl enable --now logid` if you want to enable and start the daemon.

## Donate
This program is (and will always be) provided free of charge. If you would like to support the development of this project by donating, you can donate to my Ko-Fi below.

<a href='https://ko-fi.com/R6R81QQ9M' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://cdn.ko-fi.com/cdn/kofi1.png?v=2' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>

I'm also looking for contributors to help in my project; feel free to submit a pull request or e-mail me if you would like to contribute.

## Compatible Devices

[For a list of tested devices, check TESTED.md](TESTED.md)

## Special Thanks
Thanks to the following people for contributing to this repository.

- [Clément Vuchener & contributors for creating the old HID++ library](https://github.com/cvuchener/hidpp)
- [Developers of Solaar for providing information on HID++](https://github.com/pwr-Solaar/Solaar)
- [Nestor Lopez Casado for providing Logitech documentation on the HID++ protocol](http://drive.google.com/folderview?id=0BxbRzx7vEV7eWmgwazJ3NUFfQ28)
- Everyone listed in the contributors page
