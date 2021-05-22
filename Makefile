CXX=g++
CC=g++
CXXFLAGS=-g -Wall -MMD -std=c++11
LDLIBS=-lm -lfreeglut -lglew32 -lopengl32 -lglu32 -lmuparser
all: graph
clean:
	rm -f *.o graph
graph: shader_utils.o
.PHONY: all clean
