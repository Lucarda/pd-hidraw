# library name
lib.name = hidraw

SOURCE_DIR = ./hidapi

XINCLUDE = -I ${SOURCE_DIR}/hidapi 

cflags = ${XINCLUDE} -I . -DHAVE_CONFIG_H


hidraw.class.sources = \
	pd-hidraw.c \
	${SOURCE_DIR}/windows/hid.c



define forWindows
  XINCLUDE += \
    -I ${SOURCE_DIR}/windows
    ${empty}
  ldlibs += -mwindows
endef

define forDarwin
  XINCLUDE += \
    -I ${SOURCE_DIR}/mac
    ${empty}
endef

datafiles = \





# -static 




# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder