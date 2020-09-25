PROG = $(patsubst %.cpp,%,$(wildcard *.cpp))

include ../makefile.inc

MAINFLAGS = -Wall -W -Wno-unused-parameter -fsanitize=thread

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

CFLAGS = -DUNIX -O0 -g $(MAINFLAGS)

INCLUDES += -I../macgyver \
        -I/usr/include/smartmet

LIBS += -L.. -lsmartmet-macgyver \
	-lboost_regex \
        -lboost_filesystem \
        -lboost_system \
        -lboost_iostreams \
	-lboost_thread \
	-lboost_system \
	-lpthread

all: $(PROG) examples
clean:
	rm -f $(PROG) *~

examples: $(PROG)
	@for prog in $(PROG); do \
	echo; \
	echo Running executable: ./$$prog; \
	./$$prog; \
	done

$(PROG) : % : %.cpp ../libsmartmet-macgyver.so
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
