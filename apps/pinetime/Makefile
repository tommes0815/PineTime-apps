# name of your application
APPLICATION = PineTime
NIMBLE_MAX_CONN = 2

WOLFBOOT_OFFSET=32768

# Quiet/verbose build
QUIET=1

# Optimizer CFLAGS
CFLAGS+=-O2

BOARD ?= pinetime

# This has to be the absolute path to the RIOT base directory:
PINETIMEBASE ?= $(CURDIR)/../..

CFLAGS += -I$(CURDIR) -DWOLFSSL_USER_SETTINGS -DWOLFSSL_AES_COUNTER -DWOLFSSL_AES_DIRECT -I$(CURDIR)/bin/pkg/pinetime/lvgl -I$(CURDIR)/bin/pkg/pinetime/wolfssl -Wno-unused-variable
#CFLAGS += -DTHREAD_STACKSIZE_MAIN=1024
CFLAGS+=-DGNRC_IPV6_STACK_SIZE=2048

DEVELHELP ?= 1

USEWIDGETS += home_time
USEWIDGETS += menu_tiles
USEWIDGETS += sysinfo
#USEWIDGETS += face_notification
#USEWIDGETS += face_sports

# basic modules
include $(CURDIR)/modules.mk
include $(CURDIR)/wolfboot.mk

include $(PINETIMEBASE)/Makefile.include

