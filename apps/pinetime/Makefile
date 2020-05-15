# name of your application
APPLICATION = PineTime

WOLFBOOT_OFFSET=32768

# Quiet/verbose build
QUIET=1

# Optimizer CFLAGS
CFLAGS+=-Os

BOARD ?= pinetime

# This has to be the absolute path to the RIOT base directory:
PINETIMEBASE ?= $(CURDIR)/../..

CFLAGS += -I$(CURDIR) -DWOLFSSL_USER_SETTINGS -DWOLFSSL_AES_COUNTER -DWOLFSSL_AES_DIRECT -I$(CURDIR)/bin/pkg/pinetime/lvgl -I$(CURDIR)/bin/pkg/pinetime/wolfssl -Wno-unused-variable

DEVELHELP ?= 1

USEWIDGETS += home_time
USEWIDGETS += menu_tiles
USEWIDGETS += sysinfo
USEWIDGETS += face_notification
USEWIDGETS += face_sports

# basic modules
include $(CURDIR)/modules.mk
include $(CURDIR)/wolfboot.mk

include $(PINETIMEBASE)/Makefile.include

