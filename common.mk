-include $(HOME)/.smartmet.mk

GCC_DIAG_COLOR ?= always
CXX_STD ?= c++11

ifneq ($(findstring clang++,$(CXX)),)
USE_CLANG=yes
else
USE_CLANG=no
endif

processor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(processor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

bindir = $(PREFIX)/bin
includedir = $(PREFIX)/include
datadir = $(PREFIX)/share
objdir = obj

ifeq ($(PREFIX),/usr)
  SYSTEM_INCLUDES =
else
  SYSTEM_INCLUDES = -isystem $(includedir)
endif

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -isystem /usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif
