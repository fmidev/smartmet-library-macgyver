PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

-include $(HOME)/.smartmet.mk
GCC_DIAG_COLOR ?= always
CXX_STD ?= c++11

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

# Boost 1.69

ifneq "$(wildcard /usr/include/boost169)" ""
  INCLUDES += -I/usr/include/boost169
  LIBS += -L/usr/lib64/boost169
endif

INCLUDES += -I ../macgyver -I /usr/include/smartmet

LIBS += ../libsmartmet-macgyver.so \
	-lfmt \
	-lboost_filesystem \
	-lboost_locale \
	-lboost_date_time \
	-lboost_thread \
	-lboost_system \
	-Wl,-rpath=/usr/pgsql-9.5/lib -L/usr/pgsql-9.5/lib -lpq \
	-lpthread

all: $(PROG)
clean:
	rm -f $(PROG) *~ *.d

test: $(PROG)
	@echo Running tests:
	@rm -f *.err
	@for prog in $(PROG); do \
	  ( ./$$prog || touch $$prog.err ) ; \
	done
	@test `find . -name \*.err | wc -l` = "0" || ( echo ; echo "The following tests have errors:" ; \
		for i in *.err ; do echo `basename $$i .err`; done ; rm -f *.err ; false ) 

$(PROG) : % : %.cpp ../libsmartmet-macgyver.so
	$(CC) $(CFLAGS) -MD -o $@ $@.cpp $(INCLUDES) $(LIBS)

ifneq ($(wildcard *.d),)
-include $(wildcard *.d)
endif
