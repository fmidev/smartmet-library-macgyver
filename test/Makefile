PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

-include $(HOME)/.smartmet.mk
GCC_DIAG_COLOR ?= always
CXX_STD ?= c++11

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

FLAGS = -Wall -W -Wno-unused-parameter -fdiagnostics-color=$(GCC_DIAG_COLOR)

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

ifeq ($(TSAN), yes)
  FLAGS += -fsanitize=thread
endif
ifeq ($(ASAN), yes)
  FLAGS += -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined -fsanitize-address-use-after-scope
endif

CC = g++
CFLAGS = -DUNIX -D_REENTRANT -O0 -g -std=$(CXX_STD) $(FLAGS)

ifneq (,$(findstring test-installed,$(MAKECMDGOALS)))

MACGYVER_INCLUDES := -I$(includedir)/smartmet/macgyver
MACGYVER_LDFLAGS := -L $(libdir) -lsmartmet-macgyver
MACGYVER_TARGET :=

else

MACGYVER_INCLUDES := -I../macgyver
MACGYVER_LDFLAGS := ../libsmartmet-macgyver.so
MACGYVER_TARGET := ../libsmartmet-macgyver.so

endif

# Boost 1.69

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -I/usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif

INCLUDES += $(MACGYVER_INCLUDES) -I /usr/include/smartmet

LIBS += $(MACGYVER_LDFLAGS) \
	-lfmt \
	-lboost_filesystem \
	-lboost_locale \
	-lboost_date_time \
	-lboost_thread \
	-lboost_system \
	-L/usr/pgsql-9.5/lib -l:libpq.so.5 \
	-lpthread



all: $(PROG)
clean:
	rm -f $(PROG) *~ *.d

# FIXME: clean if switching between test and test-installed or use different directories
test test-installed: $(PROG)
	@echo Running tests:
	@rm -f *.err
	@for prog in $(PROG); do \
	  ( ./$$prog || touch $$prog.err ) ; \
	done
	@test `find . -name \*.err | wc -l` = "0" || ( echo ; echo "The following tests have errors:" ; \
		for i in *.err ; do echo `basename $$i .err`; done ; rm -f *.err ; false ) 

$(PROG) : % : %.cpp $(MACGYVER_TARGET)
	$(CC) $(CFLAGS) -MD -o $@ $@.cpp $(INCLUDES) $(LIBS)

ifneq ($(wildcard *.d),)
-include $(wildcard *.d)
endif
