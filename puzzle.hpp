// Name: Conrad Bailey
// Course: Fundamentals of Computing II
// Assignment: Lab 6 - Solver
// Purpose: Interface - A C++ class for a storing and solving a sudoku
//                      puzzle

#ifndef PUZZLE_H
#define PUZZLE_H

#include <vector>
#include <unordered_set>
#include <set>
#include <string>
#include <iostream>
#include <algorithm>
#include <thread>

class Puzzle {
public:
  Puzzle(); // construct from input file
  Puzzle(const Puzzle&); // copy constructor

  bool isComplete() const; // return whether the puzzle is solved

  // returns whether the T value can legally be inserted at the cell
  // designated by the coordinates
  bool isValidInsertion(int, int, int) const;

  // insert a value into the board, returns success value
  bool insert(int, int, int);

  // returns whether the puzzle was successfully solved or not
  bool solve(bool);

  // printing
  void print() const;

private:
  // return whether the value represents emptiness
  inline bool isEmpty(const int& val) const;
  inline bool isEmpty(int row, int col) const;

  bool checkSingularCandidates();
  bool checkSingularPossibilities();
  bool checkSingularPossibilities(const std::unordered_set<int>,
                                  const std::set<std::pair<int,int>>&);
  bool checkPreemptiveSets();
  bool checkPreemptiveSets(const std::unordered_set<int>,
                           const std::set<std::pair<int,int>>&);
  bool checkPointers();

  static void solveThread(Puzzle*, Puzzle);

  static const std::unordered_set<int> validVals;   // all values that are valid in a puzzle
  std::vector<std::vector<int>> board; // represents the paper puzzle

  // records possible values for empty cells
  std::vector<std::vector<std::unordered_set<int>>> cellPotentialSets;
  std::vector<std::unordered_set<int>> rowRequiredSets;
  std::vector<std::unordered_set<int>> colRequiredSets;
  std::vector<std::unordered_set<int>> boxRequiredSets;
};

#endif
