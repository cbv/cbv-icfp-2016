.PHONY : all


UNAME := $(shell uname)
HEADERS := base.hpp structures.hpp utils.hpp rotations.hpp folders.hpp

ifeq ($(UNAME),Darwin)
	CPP=clang++ -std=c++11 -Wall -Werror -g -O2 -DCGAL_NDEBUG=1
	SDL_LIBS=-lSDL2 -framework OpenGL
endif

ifeq ($(UNAME),Linux)
	CPP=g++ -std=c++11 -Wall -Werror -g -O2 -DCGAL_NDEBUG=1
	SDL_LIBS=-lSDL2 -lGL
endif

all : check get-convex foldup show search-trees normalize

clean :
	rm -f check get-convex foldup show search-trees get-rect normalize
	rm -rf objs

objs :
	mkdir -p objs

objs/%.o : %.cpp ${HEADERS} | objs
	$(CPP) -c $< -o $@


check : objs/check.o objs/utils.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

#get-rect : objs/get-rect.o objs/utils.o objs/structures.o objs/rotations.o
#	$(CPP) $^ -o $@ -lgmp -lCGAL

get-convex : objs/get-convex.o objs/utils.o objs/structures.o objs/rotations.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

foldup : objs/foldup.o objs/utils.o objs/structures.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL

search-trees : objs/search-trees.o objs/utils.o objs/structures.o objs/sha1.o objs/Viz1.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL $(SDL_LIBS)

show : objs/show.o objs/Viz1.o objs/utils.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL $(SDL_LIBS)

normalize : objs/normalize.o objs/utils.o objs/structures.o objs/folders.o
	$(CPP) $^ -o $@ -lgmp -lCGAL
