SUBNAME = macgyver
LIB = smartmet-$(SUBNAME)
SPEC = smartmet-library-$(SUBNAME)
INCDIR = smartmet/$(SUBNAME)

# Installation directories

processor := $(shell uname -p)

# Compiler options

DEFINES = -DUNIX -D_REENTRANT -DPQXX_HIDE_EXP_OPTIONAL

REQUIRES := libpqxx icu-i18n fmt ctpp2

include makefile.inc

INCLUDES += $(pkg-config --cflags icu-i18n)

LIBS += -L$(libdir) \
	-lboost_date_time \
	-lboost_regex \
	-lboost_filesystem \
	-lboost_chrono \
	-lboost_thread \
	-lboost_system \
	$(REQUIRED_LIBS) \
	-lpthread -lrt

RPMBUILD_OPT ?=

# What to install

LIBFILE = libsmartmet-$(SUBNAME).so

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -Iinclude $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE)
debug: all
release: all
profile: all

$(LIBFILE): $(OBJS)
	$(CXX) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)
	@echo Checking $(LIBFILE) for unresolved references
	@if ldd -r $(LIBFILE) 2>&1 | c++filt | grep ^undefined\ symbol; \
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
	@list='$(HDRS)'; \
	for hdr in $$list; do \
	  HDR=$$(basename $$hdr); \
	  echo $(INSTALL_DATA) $$hdr $(includedir)/$(INCDIR)/$$HDR; \
	  $(INSTALL_DATA) $$hdr $(includedir)/$(INCDIR)/$$HDR; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_PROG) $(LIBFILE) $(libdir)/$(LIBFILE)
	@mkdir -p $(datadir)/smartmet/devel
	$(INSTALL_DATA) makefile.inc $(datadir)/smartmet/devel/makefile.inc
	$(INSTALL_DATA) makefile-abicheck.inc $(datadir)/smartmet/devel/makefile-abicheck.inc

test test-installed:
	$(MAKE) -C test $@

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz $(RPMBUILD_OPT)
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o: %.cpp
	@mkdir -p obj
	$(CXX) $(CFLAGS) $(INCLUDES) -c  -MD -MF $(patsubst obj/%.o, obj/%.d.new, $@) -o $@ $<
	@sed -e "s|^$(notdir $@):|$@:|" $(patsubst obj/%.o, obj/%.d.new, $@) >$(patsubst obj/%.o, obj/%.d, $@)
	@rm -f $(patsubst obj/%.o, obj/%.d.new, $@)

ifneq ($(wildcard obj/*.d),)
-include $(wildcard obj/*.d)
endif

include makefile-abicheck.inc
