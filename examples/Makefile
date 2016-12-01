PROG = $(patsubst %.cpp,%,$(wildcard *.cpp))

MAINFLAGS = -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

CC = g++
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS)

INCLUDES = -I../include \
        -I/usr/include/smartmet

LIBS = -L.. -lsmartmet_macgyver \
	-lboost_regex-mt \
        -lboost_filesystem-mt \
        -lboost_system-mt \
        -lboost_iostreams-mt \
	-lboost_thread-mt \
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

$(PROG) : % : %.cpp ../libsmartmet_macgyver.a
	$(CC) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
