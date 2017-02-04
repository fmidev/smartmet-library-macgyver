SUBNAME = macgyver
LIB = smartmet-$(SUBNAME)
SPEC = smartmet-library-$(SUBNAME)
INCDIR = smartmet/$(SUBNAME)

# Using 'scons' for building (make clean|release|debug|profile)
#
#
# To build serially (helps get the error messages right): make debug SCONS_FLAGS=""

SCONS_FLAGS=-j 6

# Installation directories

prosessor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(prosessor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

bindir = $(PREFIX)/bin
includedir = $(PREFIX)/include
sharedir = $(PREFIX)/share/smartmet
objdir = obj

rpmversion := $(shell grep "^Version:" $(SPEC).spec | cut -d" " -f2 | tr . _)
rpmrelease := $(shell grep "^Release:" $(SPEC).spec | cut -d" " -f2 | tr . _)

# What to install

SOFILE        = libsmartmet-$(SUBNAME).so

# How to install. Note: Must be 0755 for shared libraries, or the
# package will require itself!

INSTALL_PROG = install -m 0755
INSTALL_DATA = install -m 0664

.PHONY: test rpm

#
# The rules
#
SCONS_FLAGS += objdir=$(objdir)

all release:
	scons $(SCONS_FLAGS) $(SOFILE)

$(SOFILE):
	scons $(SCONS_FLAGS) $@

debug:
	scons $(SCONS_FLAGS) debug=1 $(SOFILE)

profile:
	scons $(SCONS_FLAGS) profile=1 $(SOFILE)

clean:
	@#scons -c objdir=$(objdir)
	-rm -f $(SOFILE) *~ source/*~ include/*~
	-rm -rf $(objdir)

format:
	clang-format -i -style=file include/*.h source/*.cpp test/*.cpp

install:
	@mkdir -p $(includedir)/$(INCDIR)
	@list=`cd include && ls -1 *.h`; \
	for hdr in $$list; do \
	  echo $(INSTALL_DATA) include/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	  $(INSTALL_DATA) include/$$hdr $(includedir)/$(INCDIR)/$$hdr; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_PROG) $(SOFILE) $(libdir)/$(SOFILE)

test:
	$(MAKE) -C test $@

rpm: clean
	if [ -e $(SPEC).spec ]; \
	then \
	  tar -czvf $(SPEC).tar.gz --transform "s,^,$(SPEC)/," * ; \
	  rpmbuild -ta $(SPEC).tar.gz ; \
	  rm -f $(SPEC).tar.gz ; \
	else \
	  echo $(SPEC).spec file missing; \
	fi;

cppcheck:
	cppcheck -DUNIX -I include -I $(includedir) source

headertest:
	@echo "Checking self-sufficiency of each header:"
	@echo
	@for hdr in $(HDRS); do \
	echo $$hdr; \
	echo "#include \"$$hdr\"" > /tmp/$(SUBNAME).cpp; \
	echo "int main() { return 0; }" >> /tmp/$(SUBNAME).cpp; \
	$(CC) $(CFLAGS) $(INCLUDES) -o /dev/null /tmp/$(SUBNAME).cpp $(LIBS); \
	done

analysis:
	@for f in source/*.cpp; do cmd="clang++ --analyze -I include -DUNIX -DFMI_COMPRESSION -DBOOST -DBOOST_IOSTREAMS_NO_LIB $$f"; echo $$cmd; $$cmd; done; rm *.plist

ifneq ($(wildcard obj/*.d),)
-include $(wildcard obj/*.d)
endif
