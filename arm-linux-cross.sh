#!/bin/sh

# this script builds arm versions of hidraw. 
# run this script on a Debian buster live cd/usb

SRC="PDDIR=/home/user/Downloads/pure-data-master/"
OUT="PDLIBDIR=./out"

sudo apt update
sudo apt install g++-arm-linux-gnueabihf g++-aarch64-linux-gnu -y
sudo dpkg --add-architecture armhf
sudo dpkg --add-architecture arm64
sudo apt update
sudo apt install libudev-dev:armhf -y
sudo apt install libudev-dev:arm64 -y
make CC=arm-linux-gnueabihf-gcc $SRC $OUT extension=l_arm install
make clean
make CC=aarch64-linux-gnu-gcc $SRC $OUT extension=l_arm64 install
make clean
sudo apt install build-essential binutils -y
sudo apt install gcc-multilib -y
sudo dpkg --add-architecture i386
sudo apt install libudev-dev libudev-dev:i386 -y
make CFLAGS=-m32 LDFLAGS=-m32 $SRC $OUT extension=l_i386 install
make clean
make $SRC $OUT extension=l_amd64 install
make clean
