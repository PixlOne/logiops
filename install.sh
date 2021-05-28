#!/bin/bash

#
# Simple script to install logiops
# TheJoker (github.com/thejoker3000)
#

# Check if run as root (with sudo permissions)

if [[ "$EUID" = 0 ]]; then
    echo "Installing dependencies now..."
    sleep 0.5
    # Install needed dependencies
    sudo apt install cmake libevdev-dev libudev-dev libconfig++-dev -y

    # Create the 'build' directory and install
    echo "Building the tool now..."
    sleep 1
    # brrrrr
    mkdir build
    cd build
    cmake ..
    make

    echo "Creating systemctl now..."
    sleep 0.5
    sudo systemctl enable logid

    echo "Build is done. You can test it by running 'sudo ./logid'"
    echo ""
    echo ""
    exit
else
    echo "Please run with higher privileges"
    exit
fi