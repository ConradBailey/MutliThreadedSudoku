// Name: Conrad Bailey
// Course: Fundamentals of Computing II
// Assignment: Lab 6 - solver
// Purpose: Show the solution (partial or full) of a sudoku puzzle

#include <iostream>

#include "puzzle.hpp"

// pass a filename for input to the command. I've included sudoku.txt
// for convenience
int main() {
  // construct objects
  Puzzle puz;
  // solve and print
  puz.solve(true);
  puz.print();
}

