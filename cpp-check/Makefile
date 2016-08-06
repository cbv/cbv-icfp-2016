.PHONY : all


UNAME := $(shell uname)
HEADERS := structures.hpp rotations.hpp folders.hpp

ifeq ($(UNAME),Darwin)
	CPP=clang++ -std=c++11 -Wall -Werror -g
endif

ifeq ($(UNAME),Linux)
	CPP=g++ -std=c++11 -Wall -Werror -g -I../../game-libs64/out/include
	SDL_LIBS=-L../../game-libs64/out/include -lSDL2 -lGL
endif

all : check get-rect get-convex foldup show

clean :
	rm -f check get-rect get-convex foldup show
	rm -rf objs

objs :
	mkdir -p objs

objs/%.o : %.cpp ${HEADERS} | objs
	$(CPP) -c $< -o $@


check : objs/check.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

get-rect : objs/get-rect.o objs/structures.o objs/rotations.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

get-convex : objs/get-convex.o objs/structures.o objs/rotations.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

foldup : objs/foldup.o objs/structures.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

show : objs/show.o objs/Viz1.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL $(SDL_LIBS)
