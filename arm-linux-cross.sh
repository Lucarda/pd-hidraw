#!/bin/sh

# this script builds arm versions of hidraw. 
# to be used on a Debian buster live cd/usb

sudo apt update
sudo apt install g++-arm-linux-gnueabihf g++-aarch64-linux-gnu -y
sudo dpkg --add-architecture armhf
sudo dpkg --add-architecture arm64
sudo apt update
sudo apt install libudev-dev:armhf -y
sudo apt install libudev-dev:arm64 -y
make CC=arm-linux-gnueabihf-gcc PDDIR=/home/user/Downloads/pure-data-master/ PDLIBDIR=./out extension=l_arm install
make clean
make CC=aarch64-linux-gnu-gcc PDDIR=/home/user/Downloads/pure-data-master/ PDLIBDIR=./out extension=l_arm64 install
make clean