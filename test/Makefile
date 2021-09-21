REQUIRES = fmt libpqxx

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

CFLAGS = -DUNIX -D_REENTRANT -O0 -g $(FLAGS)

ifeq ($(USE_CLANG),yes)
CharsetToolsTest: CFLAGS += -Wno-invalid-source-encoding
endif

ifneq (,$(findstring test-installed,$(MAKECMDGOALS)))

MACGYVER_INCLUDES := -I$(includedir)/smartmet/macgyver
MACGYVER_LDFLAGS := -L $(libdir) -lsmartmet-macgyver
MACGYVER_TARGET :=

else

MACGYVER_INCLUDES := -I../macgyver
MACGYVER_LDFLAGS := ../libsmartmet-macgyver.so
MACGYVER_TARGET := ../libsmartmet-macgyver.so

endif

INCLUDES += $(MACGYVER_INCLUDES) -I /usr/include/smartmet

LIBS += $(MACGYVER_LDFLAGS) \
	$(REQUIRED_LIBS) \
	-lboost_filesystem \
	-lboost_locale \
	-lboost_date_time \
	-lboost_thread \
	-lboost_chrono \
	-lboost_system \
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
	$(CXX) $(CFLAGS) -MD -o $@ $@.cpp $(INCLUDES) $(LIBS)

ifneq ($(wildcard *.d),)
-include $(wildcard *.d)
endif
