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

check : check.cpp
	$(CPP) check.cpp -o $@ -lgmp -lCGAL
