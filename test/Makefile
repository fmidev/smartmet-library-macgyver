PROG = $(patsubst %.cpp,%,$(wildcard *Test.cpp))

MAINFLAGS = -Wall -W -Wno-unused-parameter -fdiagnostics-color=always

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast \
	-Wshadow -Woverloaded-virtual

DIFFICULTFLAGS = -Weffc++ -Wunreachable-code -Wredundant-decls

CC = g++
CFLAGS = -DUNIX -D_REENTRANT -O0 -g -std=c++11 $(MAINFLAGS)
INCLUDES = -I ../include -I /usr/include/smartmet

LIBS = ../libsmartmet_macgyver.so \
	-lcppformat \
	-lboost_locale \
	-lboost_date_time \
	-lboost_thread

all: $(PROG)
clean:
	rm -f $(PROG) *~

test: $(PROG)
	@rm -rf /tmp/filecachetest #Filecache uses this directory
	@rm -rf /tmp/filecachetest2 #Filecache uses this directory
	@rm -rf /tmp/filecachetest3 #Filecache uses this directory
	@echo Running tests:
	@ok=true; \
	for prog in $(PROG); do \
	./$$prog || ok=false; \
	done; \
	$$ok

$(PROG) : % : %.cpp ../libsmartmet_macgyver.so
	$(CC) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)
