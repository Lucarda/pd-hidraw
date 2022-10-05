# library name
lib.name = hidraw

SOURCE_DIR = ./hidapi

XINCLUDE = -I ${SOURCE_DIR}/hidapi 

cflags = ${XINCLUDE} -I . -DHAVE_CONFIG_H


hidraw.class.sources = pd-hidraw.c


define forLinux
	hidraw.class.sources += ${SOURCE_DIR}/linux/hid.c
	ldlibs += -ludev -lrt
endef

define forWindows
	hidraw.class.sources += ${SOURCE_DIR}/windows/hid.c
	XINCLUDE += -I ${SOURCE_DIR}/windows
	ldlibs += -mwindows
endef

define forDarwin
	hidraw.class.sources += ${SOURCE_DIR}/mac/hid.c
	XINCLUDE += -I ${SOURCE_DIR}/mac
	ldlibs += -framework IOKit -framework CoreFoundation -framework AppKit
endef

datafiles = \
	README.md \
	hidraw-help.pd \
    CHANGELOG.txt \
	${empty}



PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
