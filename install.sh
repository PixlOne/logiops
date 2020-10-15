#!/bin/sh
echo "Installing logiops by PixlOne..."
echo "\n-------------------------\n"

# Install dependencies:
echo "Installing dependencies..."
if [ -x "$(command -v apt)" ]; then
  sudo apt install -y g++ cmake libevdev-dev libudev-dev libconfig++-dev
elif [ -x "$(command -v pacman)" ]; then
  sudo pacman -S g++ cmake libevdev libconfig pkgconf
elif [ -x "$(command -v yum)" ]; then
  sudo yum -y install cmake libevdev-devel libudev-devel libconfig-devel
else
  echo "Install failed: Package manager not supported."
  echo "Please build logiops manually. Exiting..."
  exit 1
fi

# Build
echo "\n-------------------------\n"
echo "Building logiops..."
if [ `echo $?` = "0" ]; then
  mkdir -p build
else
  echo "Dependencies installation failed. Exiting..."
  exit 1
fi

cd build
cmake ..
make

if [ `echo $?` = "0" ]; then
  sudo make install
else
  echo "Make failed. Exiting..."
  exit 1
fi

# Start, autostart
echo "\n-------------------------\n"
echo "Setting up logid to autostart on boot..."
sudo systemctl enable logid
sudo systemctl start logid

# Config file
echo "\n-------------------------\n"
echo "Creating config file..."
sudo touch /etc/logid.cfg

# Finish up
echo "\n-------------------------\n"
echo "Install complete!"
echo "Run logid -v to check your device name, then go to /etc/logid.cfg to configure your mouse settings."
echo "Detailed instructions and examples here: https://wiki.archlinux.org/index.php/Logitech_MX_Master"
echo "\n-------------------------\n"
echo "Press enter to finish..."
read nothing
exit 0
