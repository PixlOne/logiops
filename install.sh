#!/bin/sh
echo "Installing logiops by PixlOne..."

# Check sudo privileges:
if [ `id | sed -e 's/(.*//'` != "uid=0" ]; then
  echo "You need superuser privileges to run this install script, please run as superuser."
  echo "Exiting..."
  exit 1
fi

# Install dependencies:
if [ -d "/etc/apt" ]; then
  apt install -y g++ cmake libevdev-dev libudev-dev libconfig++-dev
elif [ -f "/etc/arch-release" ]; then
  pacman -S g++ cmake libevdev libconfig pkgconf
else
  echo "Install failed: System is not Debian or Arch."
  echo "Exiting..."
  exit 1
fi

# Build
echo "Building program..."
if [ `echo $?` = "0" ]; then
  mkdir -p build
else
  echo "Dependencies installation failed."
  echo "Exiting..."
  exit 1
fi

cd build
cmake ..
make
if [ `echo $?` = "0" ]; then
  make install
else
  echo "Make failed."
  echo "Exiting..."
  exit 1
fi

# Start, autostart
systemctl enable logid
systemctl start logid

touch /etc/logid.cfg

# Finish up
echo "-------------------------"
echo "Install complete!"
echo "-------------------------"
echo "Go to /etc/logid.cfg to configure your mouse settings. Examples here: https://wiki.archlinux.org/index.php/Logitech_MX_Master"
# Print device name
echo "Your mouse name: "
logid -v
exit 0