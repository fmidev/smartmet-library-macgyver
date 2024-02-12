SUBNAME = macgyver
LIB = smartmet-$(SUBNAME)
SPEC = smartmet-library-$(SUBNAME)
INCDIR = smartmet/$(SUBNAME)

# Installation directories

processor := $(shell uname -p)

# Build configuration parameters

# NOTE: USE_OS_TZDB is determined by the presence of the USE_OS_TZDB macro in date_time/Base.h
#       and its value

# [USE_OS_TZDB==0 only]: If TZDB_REMOTE_API!=0 is defined, the tz.cpp file will try to download latest
# time zone database version from IANA's ftp server. If TZDB_REMOTE_API==0, the tz.cpp file will assume
# that the time zone database (source files) is already available
TZDB_REMOTE_API := 1

# [USE_OS_TZDB==1 and TBZD_REMOTE_API=0 only]: The folder where to downoad the time zone database files
# Files are downloaded from IANA's server and extracted to the subdirectory "tznames" in the specified
# directory. Value $(HOME)/Download is used when not specified (empty) or when TZBD_REMOTE_API==0
TZDB_FOLDER := $(datadir)/smartmet/timezones

# Compiler options

DEFINES = -DUNIX -D_REENTRANT -DPQXX_HIDE_EXP_OPTIONAL

REQUIRES := libpqxx icu-i18n fmt ctpp2

include $(shell smartbuildcfg --prefix)/share/smartmet/devel/makefile.inc

LIBS += -L$(libdir) \
	-lboost_date_time \
	-lboost_regex \
	-lboost_filesystem \
	-lboost_chrono \
	-lboost_iostreams \
	-lboost_thread \
	-lboost_system \
	-ldouble-conversion \
	$(REQUIRED_LIBS) \
	-ldate-tz \
	-lpthread -lrt

RPMBUILD_OPT ?=

# What to install

LIBFILE = libsmartmet-$(SUBNAME).so

# Compilation directories

INTERNAL_HDRS = date_time/Internal.h date_time/date/tz_private.h
SRC_DIRS = $(SUBNAME) $(SUBNAME)/date_time $(SUBNAME)/date_time/date

vpath %.cpp $(SRC_DIRS)
vpath %.h $(SRC_DIRS)

# The files to be compiled

SRCS = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
HDRS = $(filter-out $(INTERNAL_HEADERS), $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.h)))
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

ifneq ($(words $(sort $(OBJS))),$(words $(OBJS)))
$(info $(shell for f in $(OBJS); do echo $$f; done | sort | uniq -c | grep -v '^ *1 ' | sed -e 's/^ *//' -e 's/ /: /'))
$(error Source file name conflict detected)
endif

INCLUDES := -Iinclude $(INCLUDES)

# Detects presence of USE_OS_TZDB in date_time/Base.h to use the correct value
# when compiling date_time/date/tz.cpp
USE_OS_TZDB := $(shell awk '/^#define USE_OS_TZDB/ {print $$3}' $(SUBNAME)/date_time/Base.h)
ifeq ($(USE_OS_TZDB),)
$(error USE_OS_TZDB not defined in $(SUBNAME)/date_time/Base.h)
endif

ifeq ($(USE_OS_TZDB),0)
TZ_CFLAGS = -DUSE_OS_TZDB=0
ifeq ($(TZDB_REMOTE_API),0)
TZ_CFLAGS += -DHAS_REMOTE_API=1 -DAUTO_DOWNLOAD=0
else
LIBS += -lcurl
endif
ifneq ($(TZDB_FOLDER),)
TZ_CFLAGS += -DTZ_SOURCE_FOLDER="\"$(datadir)/smartmet/timezones\""
endif
else
TZ_CFLAGS = -DUSE_OS_TZDB=1
endif

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE)
debug: all
release: all
profile: all

$(LIBFILE): $(OBJS)
	$(CXX) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)
	@echo Checking $(LIBFILE) for unresolved references
	@if ldd -r $(LIBFILE) 2>&1 | c++filt | grep ^undefined\ symbol |\
			grep -Pv ':\ __(?:(?:a|t|ub)san_|sanitizer_)'; \
		then rm -v $(LIBFILE); \
		exit 1; \
	fi

clean:  $(CLEAN_TARGETS)
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf $(objdir)
	$(MAKE) -C test clean
	$(MAKE) -C examples clean

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp test/*.cpp

examples:
	$(MAKE) -C examples

install:
	@mkdir -p $(includedir)/$(INCDIR)
	@list='$(HDRS)'; set -x; \
	for hdr in $$list; do \
	  HDR=$$(echo $$hdr | sed -e 's:^$(SUBNAME)/::'); \
	  subdir=$$(dirname $$HDR); \
	  echo $(INSTALL_DATA) -D $$hdr $(includedir)/$(INCDIR)/$(subdir)/$$HDR; \
	  $(INSTALL_DATA) -D $$hdr $(includedir)/$(INCDIR)/$(subdir)/$$HDR; \
	set +x; done
	@mkdir -p $(libdir)
	$(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)

test test-installed:
	$(MAKE) -C test $@

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz $(RPMBUILD_OPT)
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

objdir:
	mkdir -p $(objdir)

obj/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(INCLUDES) -c -MD -MF $(patsubst obj/%.o, obj/%.d, $@) -MT $@ -o $@ $<

obj/tz.o: CFLAGS += $(TZ_CFLAGS)

ifneq ($(wildcard obj/*.d),)
-include $(wildcard obj/*.d)
endif

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile-abicheck.inc
