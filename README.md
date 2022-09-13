# pd-hidraw


Home of pd-hidraw is [#](#)


A [Pure-Data](https://github.com/pure-data/pure-data) external that 
provides read access to HIDs wrapping code from [https://github.com/libusb/hidapi](https://github.com/libusb/hidapi).

### building 

to build on Linux first get the `libudev-dev` for your distro (here shown for Debian). 

	sudo apt install libudev-dev
	
then (MinGW, xcode or GCC)

	make