# pd-hidraw


Home of pd-hidraw is [https://github.com/Lucarda/pd-hidraw](https://github.com/Lucarda/pd-hidraw)


A [Pure-Data](https://github.com/pure-data/pure-data) external that 
provides read access to HIDs wrapping code from [https://github.com/libusb/hidapi](https://github.com/libusb/hidapi).

## building 

### git submodules

The hidapi sources are available via a git submodule in the hidapi folder.

After cloning the pd-hidraw repository, initialize the submodules via:

	git submodule update --init --recursive

### without git

If you obtained the 'pd-hidraw' sources without git (e.g. by downloading a release tarball), 
there won't be any bundled hidapi. Instead, you can manually download hidapi from
https://github.com/libusb/hidapi and put it into the main folder.

### compiling

to build on Linux first get the `libudev-dev` pkg for your distro (here shown for Debian). 

	sudo apt install libudev-dev
	
then on Linux(GCC), macOS(Xcode) or Windows(MinGW):

	make