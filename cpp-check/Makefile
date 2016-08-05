.PHONY : all


UNAME := $(shell uname)

ifeq ($(UNAME),Darwin)
	#Appologies -- I haven't actually tested this:
	CPP=clang++ -std=c++11 -Wall -Werror -g
endif

ifeq ($(UNAME),Linux)
	CPP=g++ -std=c++11 -Wall -Werror -g
endif


all : check

objs :
	mkdir -p objs

objs/%.o : %.cpp structures.hpp | objs
	$(CPP) -c $< -o $@

check : objs/check.o objs/structures.o
	$(CPP) $^ -o $@ -lgmp -lCGAL
