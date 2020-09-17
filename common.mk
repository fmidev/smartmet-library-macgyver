-include $(HOME)/.smartmet.mk

GCC_DIAG_COLOR ?= always

FLAGS += -fdiagnostics-color=$(GCC_DIAG_COLOR) -Wall -Wextra -fno-omit-frame-pointer -fPIC

ifneq ($(findstring clang,$(CXX)),)
  USE_CLANG=yes
  CXX_STD ?= c++17
  FLAGS += -std=$(CXX_STD) -Wno-c++98-compat -Wno-float-equal -Wno-padded -Wno-missing-prototypes -Wno-unused-parameter
  FLAGS_DEBUG +=
  FLAGS_RELEASE +=
else
  USE_CLANG=no
  CXX_STD ?= c++11
  FLAGS += -std=$(CXX_STD) -Wno-unused-parameter
  FLAGS_DEBUG += -Wcast-align -Winline -Wno-multichar -Wno-pmf-conversions -Woverloaded-virtual \
	-Wpointer-arith -Wcast-qual -Wwrite-strings -Wno-sign-promo -Wno-unknown-pragmas \
	-Wno-inline
  FLAGS_RELEASE += -Wuninitialized
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
enginedir = $(datadir)/smartmet/engines
plugindir = $(datadir)/smartmet/plugins

ifneq ($(PREFIX),/usr)
  INCLUDES += -isystem $(includedir)
endif

INCLUDES += -I$(includedir)/smartmet

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -isystem /usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif

ifeq ($(REQUIRES_GDAL),yes)
  ifneq "$(wildcard /usr/gdal30/include)" ""
    INCLUDES += -isystem /usr/gdal30/include
    LIBS += -L$(PREFIX)/gdal30/lib
  else
    INCLUDES += -I/usr/include/gdal
  endif
endif

ifeq ($(REQUIRES_PGSQL),yes)
  ifneq "$(wildcard /usr/pgsql-12/lib)" ""
    LIBS += -L$(PREFIX)/pgsql12-lib -l:libpq.so.5
  else
    LIBS += -L$(PREFIX)/pgsql-9.5/lib -l:libpq.so.5
  endif
endif

# How to install

INSTALL_PROG = install -p -m 775
INSTALL_DATA = install -p -m 664

# Sanitizer support

ifeq ($(TSAN), yes)
  FLAGS += -fsanitize=thread
endif
ifeq ($(ASAN), yes)
  FLAGS += -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract \
           -fsanitize=undefined -fsanitize-address-use-after-scope
endif

# Compile options in detault, debug and profile modes
CFLAGS         = $(DEFINES) $(FLAGS) $(FLAGS_RELEASE) -DNDEBUG -O2 -g
CFLAGS_DEBUG   = $(DEFINES) $(FLAGS) $(FLAGS_DEBUG)   -Werror  -Og -g
CFLAGS_PROFILE = $(DEFINES) $(FLAGS) $(FLAGS_PROFILE) -DNDEBUG -O2 -g -pg

# Compile option overrides

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_DEBUG)
endif

ifneq (,$(findstring profile,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_PROFILE)
endif
