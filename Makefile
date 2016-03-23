#Name: Conrad Bailey
#Course: Fundamentals of Computing II
#Assignment: Lab 5 - Sudoku Puzzle
#Purpose: Makefile - Rules for compiling all parts of the project

#lots of tricks from http://scottmcpeak.com/autodepend/autodepend.html

SUDOKU=sudoku.exe
OBJS=sudoku.o puzzle.o

CXX=g++ -std=gnu++11
CXXFLAGS=-O3

LINKER=g++ -std=gnu++11
LINKERFLAGS=
LINKS=-lpthread

all: $(OBJS) $(SUDOKU)

debug: CXXFLAGS=-W -Wall -Wextra -Werror -std=gnu++11 -O0 -ggdb3
debug: all

# Link objects into the executable
%.exe: $(OBJS)
	$(LINKER) $(LINKERFLAGS) $(LINKS) $(OBJS) -o $@

# Compile an object file
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.exe *.o
