# logiops

This is an unofficial driver for Logitech mice and keyboard.

This is currently only compatible with HID++ \>2.0 devices.

## Configuration
[Refer to the wiki for details.](https://github.com/PixlOne/logiops/wiki/Configuration)

You may also refer to [logid.example.cfg](./logid.example.cfg) for an example.

Default location for the configuration file is /etc/logid.cfg, but another can be specified using the `-c` flag.

## Dependencies

This project requires a C++14 compiler, `cmake`, `libevdev`, `libudev`, and `libconfig`. For popular distributions, I've included commands below.

**Arch Linux:** `sudo pacman -S cmake libevdev libconfig pkgconf`

**Debian/Ubuntu:** `sudo apt install cmake libevdev-dev libudev-dev libconfig++-dev`

**Fedora:** `sudo dnf install cmake libevdev-devel systemd-devel libconfig-devel gcc-c++`

**Gentoo Linux:** `sudo emerge dev-libs/libconfig dev-libs/libevdev dev-util/cmake virtual/libudev`

**Solus:** `sudo eopkg install libevdev-devel libconfig-devel libgudev-devel`

**openSUSE:** `sudo zypper install cmake libevdev-devel systemd-devel libconfig-devel gcc-c++ libconfig++-devel libudev-devel`

## Initial setup

A critical item is to determine the name your device reports to the computer
which will placed in the [name field](https://github.com/pixlone/logiops/wiki/configuration#name)
of the configuration file.

[Build](#building) `logid` and from the repository root run it (use `Ctrl+C` to stop `logid`).
You should see something like the following:
```
$ sudo ./build/logid
[ERROR] I/O Error while reading /etc/logid.cfg: FileIOException
[INFO] Device MX Anywhere 3 not configured, using default config.
[INFO] Device found: MX Anywhere 3 on /dev/hidraw3:255
^C
```

Next, create a configuration file that contains the name, `MX Anywhere 3`, of the
device determined from executing `logid` above. A starting point is [logid.example.cfg](./logid.example.cfg),
located in the root of the repo. And copy it to `/etc/logid.cfg`, the default configuration location:
```
sudo cp ./logid.example.cfg /etc/logid.cfg
```

Now edit `/etc/logid.cfg` referring to the
[Configuration section of the wiki](https://github.com/pixlone/logiops/wiki/configuration)
for details. At a minimum change the name field to match above.
Such `name: "MX Anywhere 3"`, here is the diff:
```
$ diff -u1 logid.example.cfg /etc/logid.cfg
--- logid.example.cfg   2022-04-26 10:41:59.650418368 -0700
+++ /etc/logid.cfg      2022-04-26 10:47:49.033924388 -0700
@@ -4,3 +4,3 @@
     #   https://github.com/PixlOne/logiops/wiki/Configuration#name
-    name: "Wireless Mouse MX Master";
+    name: "MX Anywhere 3";
     smartshift:
```

Next verify that your configuration is used. Run `logid` again
and this time you should **not have** `[ERROR] I/O Error while reading /etc/logid.cfg: FileIOException`.
And instead should it look something like:
```
$ sudo ./build/logid
[INFO] Device found: MX Anywhere 3 on /dev/hidraw3:255
[WARN] /dev/hidraw3:255: SmartShift feature not found, cannot use ToggleSmartShift action.
[WARN] MX Anywhere 3: CID 0xc3 does not exist.
^C
```

The two `[WARN]` lines indicate tweaks maybe needed to your
[configuration file](https://github.com/pixlone/logiops/wiki/configuration).


Now [install](#install) `logid` and use `systemctl` so it runs as a daemon.

## Building

To build this project, run:

```bash
mkdir build
cd build
cmake ..
make
```

## Install

To install, run `sudo make install` after building. You can set the daemon to start at boot by running `sudo systemctl enable logid` or `sudo systemctl enable --now logid` if you want to enable and start the daemon.

## Donate
This program is (and will always be) provided free of charge. If you would like to support the development of this project by donating, you can donate to my Ko-Fi below.

<a href='https://ko-fi.com/R6R81QQ9M' target='_blank'><img height='36' style='border:0px;height:36px;' src='https://cdn.ko-fi.com/cdn/kofi1.png?v=2' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>

I'm also looking for contributors to help in my project; feel free to submit a pull request or e-mail me if you would like to contribute.

## Compatible Devices

[For a list of tested devices, check TESTED.md](TESTED.md)

## Credits

Logitech, Logi, and their logos are trademarks or registered trademarks of Logitech Europe S.A. and/or its affiliates in the United States and/or other countries. This software is an independent product that is not endorsed or created by Logitech.

Thanks to the following people for contributing to this repository.

- [Clément Vuchener & contributors for creating the old HID++ library](https://github.com/cvuchener/hidpp)
- [Developers of Solaar for providing information on HID++](https://github.com/pwr-Solaar/Solaar)
- [Nestor Lopez Casado for providing Logitech documentation on the HID++ protocol](http://drive.google.com/folderview?id=0BxbRzx7vEV7eWmgwazJ3NUFfQ28)
- Everyone listed in the contributors page
