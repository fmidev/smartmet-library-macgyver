##############################################################################
#
#  This file is part of smartmet-library-macgyver
#
#  It is installed as $PREFIX/share/smartmet/devel/makefile.inc
#
#  For use in other projects put into the Makefile
#
#  include $$(PREFIX-/usr)/share/smartmet/devel/makefile.inc
#
###############################################################################

-include $(HOME)/.smartmet.mk

# Installation

INSTALL_PROG = install -p -m 775
INSTALL_DATA = install -p -m 664

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

processor := $(shell uname -p)
ifeq ($(processor), x86_64)
  libdir ?= $(PREFIX)/lib64
else
  libdir ?= $(PREFIX)/lib
endif

bindir ?= $(PREFIX)/bin
includedir ?= $(PREFIX)/include
datadir ?= $(PREFIX)/share
sbindir ?= $(PREFIX)/sbin
objdir = obj

enginedir ?= $(datadir)/smartmet/engines
plugindir ?= $(datadir)/smartmet/plugins

# Compiler flags

GCC_DIAG_COLOR ?= always

CXX_VERSION_STRING := $(shell $(CXX) --version | head -n 1)
CXX_ID_ := $(shell echo "$(CXX_VERSION_STRING)" | sed -e 's:[\ -].*::')

ifneq ($(filter $(CXX_ID_), clang),)
  USE_CLANG=yes
  CXX_STD ?= c++17
else
  USE_CLANG=no
  CXX_STD ?= c++11
endif

FLAGS += -std=$(CXX_STD) -fdiagnostics-color=$(GCC_DIAG_COLOR) \
	-ggdb3 -fPIC -fno-omit-frame-pointer \
	-Wall -Wextra \
	-Wno-unused-parameter

FLAGS_DEBUG = -Og -Werror -Wpedantic -Wundef
FLAGS_RELEASE = -O2 -Wuninitialized -DNDEBUG
FLAGS_PROFILE = -O2 -g -pg -DNDEBUG

ifeq ($(USE_CLANG), yes)
  FLAGS_DEBUG += -Wshadow -Wweak-vtables -Wzero-as-null-pointer-constant
  # clang does not by default provide parameter --build-id to linker. Add it directly
  LDFLAGS += -Wl,--build-id=sha1
endif

# Sanitizer support

ifeq ($(TSAN), yes)
  FLAGS += -fsanitize=thread
endif
ifeq ($(ASAN), yes)
  FLAGS += -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract \
           -fsanitize=undefined -fsanitize-address-use-after-scope
endif

# Static code analyzer support (warning: currently only gcc-10+)

ifeq ($(ANALYZER), yes)
  ifeq ($(USE_CLANG), yes)
     $(error "TODO: Implement static code analyzer support for CLANG")
  else
     FLAGS += -fanalyzer
  endif
endif


FLAGS += $(EXTRA_FLAGS)

# Compile modes (debug / release)

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(DEFINES) $(FLAGS) $(FLAGS_DEBUG)
else
  ifneq (,$(findstring profile,$(MAKECMDGOALS)))
    CFLAGS = $(DEFINED) $(FILES) $(FLAGS_PROFILE)
  else
    CFLAGS = $(DEFINES) $(FLAGS) $(FLAGS_RELEASE)
  endif
endif

# Include paths and libs

ifneq ($(PREFIX),/usr)
  INCLUDES += -isystem $(includedir)
endif

INCLUDES += -I$(includedir)/smartmet


REQUIRES ?=

ifneq ($(filter gdal,$(REQUIRES)),)
  REQUIRES_GDAL := yes
endif

ifneq ($(filter pgsql,$(REQUIRES)),)
  REQUIRES_PGSQL := yes
endif

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -isystem /usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif

ifeq ($(REQUIRES_GDAL),yes)
  ifneq "$(wildcard /usr/gdal30/include)" ""
    INCLUDES += -isystem /usr/gdal30/include
    LIBS += -L$(PREFIX)/gdal30/lib
  else
    INCLUDES += -isystem /usr/include/gdal
  endif
  GDAL_LIBS = -lgdal
endif

ifeq ($(REQUIRES_PGSQL),yes)
  ifneq "$(wildcard /usr/pgsql-12/lib)" ""
    LIBS += -L$(PREFIX)/pgsql12-lib -l:libpq.so.5
  else
    LIBS += -L$(PREFIX)/pgsql-9.5/lib -l:libpq.so.5
  endif
endif

ifneq ($(filter jsoncpp,$(REQUIRES)),)
  INCLUDES += $(shell pkg-config --cflags jsoncpp 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  JSONCPP_LIBS = $(shell pkg-config --libs jsoncpp 2>/dev/null)
  ifeq ($(JSONCPP_LIBS),)
    $(error No JsonCpp available)
  endif
endif

ifneq ($(filter mysql,$(REQUIRES)),)
  INCLUDES += $(shell mysql_config --cflags 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  MYSQL_LIBS = $(shell mysql_config --libs_r 2>/dev/null)
  ifeq ($(MYSQL_LIBS),)
    $(error No MySQL libraries available)
  endif
endif

ifneq ($(filter icu-i18n,$(REQUIRES)),)
  INCLUDES += $(shell pkg-config --cflags icu-i18n 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  ICU_I18N_LIBS = $(shell pkg-config --libs icu-i18n 2>/dev/null)
  ifeq ($(ICU_I18N_LIBS),)
    $(error No icu-i18n library available)
  endif
endif

ifneq ($(filter configpp,$(REQUIRES)),)
  INCLUDES += $(shell pkg-config --cflags libconfig++ 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  CONFIGPP_LIBS = $(shell pkg-config --libs libconfig++ 2>/dev/null)
  ifeq ($(CONFIGPP_LIBS),)
    $(error No libconfig++ development files available)
  endif
endif

ifneq ($(filter librsvg,$(REQUIRES)),)
  INCLUDES += $(shell pkg-config --cflags librsvg-2.0 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  LIBRSVG_LIBS = $(shell pkg-config --libs librsvg-2.0)
  ifeq ($(LIBRSVG_LIBS),)
    $(error No librsvg-2 library development files found)
  endif
endif

ifneq ($(filter cairo,$(REQUIRES)),)
  INCLUDES += $(shell pkg-config --cflags cairo 2>/dev/null | sed -r 's/-I[[:space:]]*/-isystem /g')
  CAIRO_LIBS = $(shell pkg-config --libs cairo)
  ifeq ($(CAIRO_LIBS),)
    $(error No Cairo library development files found)
  endif
endif
