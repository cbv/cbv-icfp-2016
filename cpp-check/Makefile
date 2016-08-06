.PHONY : all


UNAME := $(shell uname)
HEADERS := structures.hpp rotations.hpp
#patching this out because I don't like linking code that isn't needed
#LINKED_OBJECTS := $(patsubst %.hpp,objs/%.o,${HEADERS})

ifeq ($(UNAME),Darwin)
	CPP=clang++ -std=c++11 -Wall -Werror -g
endif

ifeq ($(UNAME),Linux)
	CPP=g++ -std=c++11 -Wall -Werror -g -I../../game-libs64/out/include
	SDL_LIBS=-L../../game-libs64/out/include -lSDL2 -lGL
endif

all : check get-rect foldup

objs :
	mkdir -p objs

objs/%.o : %.cpp ${HEADERS} | objs
	$(CPP) -c $< -o $@


check : objs/check.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

get-rect : objs/get-rect.o objs/structures.o objs/rotations.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

foldup : objs/foldup.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

show : objs/show.o objs/Viz1.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL $(SDL_LIBS)
