#!/bin/sh
echo "Installing logiops by PixlOne..."

# Check sudo privileges:
if [ `id | sed -e 's/(.*//'` != "uid=0" ]; then
  echo "You need superuser privileges to run this install script, please run as superuser."
  exit 1
fi

# Install dependencies:
if [ -d "/etc/apt" ]; then
  apt install -y cmake libevdev-dev libudev-dev libconfig++-dev
elif [ -f "/etc/arch-release" ]; then
  pacman -S cmake libevdev libconfig pkgconf
else
  echo "Install failed: System is not Debian or Arch."
  exit 1
fi

# Build
if [ `echo $?` = "0" ]; then
  mkdir -p build
else
  echo "Dependencies installation failed."
  exit 1
fi

cd build
cmake ..
make
if [ `echo $?` = "0" ]; then
  make install
else
  echo "Make failed."
  exit 1
fi

# Start, autostart
systemctl enable logid
systemctl start logid

# Print device name
echo "Your device name: "
logid -v
touch /etc/logid.cfg

# Finish up
echo "-------------------------"
echo "Install complete!"
echo "Go to /etc/logid.cfg to configure your mouse settings. Examples here: https://wiki.archlinux.org/index.php/Logitech_MX_Master"