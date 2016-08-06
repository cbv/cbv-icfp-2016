.PHONY : all


UNAME := $(shell uname)
HEADERS := structures.hpp rotations.hpp
LINKED_OBJECTS := $(patsubst %.hpp,objs/%.o,${HEADERS})

ifeq ($(UNAME),Darwin)
	CPP=clang++ -std=c++11 -Wall -Werror -g
endif

ifeq ($(UNAME),Linux)
	CPP=g++ -std=c++11 -Wall -Werror -g
endif


all : check get-rect

objs :
	mkdir -p objs

objs/%.o : %.cpp ${HEADERS} | objs
	$(CPP) -c $< -o $@

check : objs/check.o ${LINKED_OBJECTS}
	$(CPP) $^ -o $@ -lgmp -lCGAL

get-rect : objs/get-rect.o ${LINKED_OBJECTS}
	$(CPP) $^ -o $@ -lgmp -lCGAL
