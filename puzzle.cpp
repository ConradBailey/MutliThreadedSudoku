#include "puzzle.hpp"

const std::unordered_set<int> Puzzle::validVals{1,2,3,4,5,6,7,8,9};

Puzzle::Puzzle(const Puzzle& puz) :
  // call data member copy constructors
  board(puz.board),
  cellPotentialSets(puz.cellPotentialSets),
  rowRequiredSets(puz.rowRequiredSets),
  colRequiredSets(puz.colRequiredSets),
  boxRequiredSets(puz.boxRequiredSets)
{}

Puzzle::Puzzle() :
  board(9, std::vector<int>(9)),
  cellPotentialSets(9, std::vector<std::unordered_set<int>>(9, validVals)),
  rowRequiredSets(9, validVals),
  colRequiredSets(9, validVals),
  boxRequiredSets(9, validVals)
{
  int insertion;
  for (int r{0}; r < 9; ++r) {
    for (int c{0}; c < 9; ++c) {
      std::cin >> insertion;
      // only do work if value is non-empty, otherwise skip it
      if (!isEmpty(insertion)) {
        // insert initial pieces onto board, if insertion fails then the
        // initialization file has flaws
        if(!insert(insertion, r, c)) {
          std::cerr << "Error: invalid board initialization file at ("
                    << r << ", " << c << ')' << std::endl;
          exit(EXIT_FAILURE);
        }
      }
    }
  }
}

bool Puzzle::isComplete() const {
  // Assuming all insertions have been valid, i.e. none were invalid,
  // then it is sufficient to check if the board is filled
  for (auto r = board.cbegin(); r != board.cend(); r++) {
    for (auto c = r->cbegin(); c != r->cend(); c++) {
      if (isEmpty(*c)) {
        return false;
      }
    }
  }

  return true;
}

bool Puzzle::isValidInsertion(int insertion, int r, int c) const {
  // check occupied space
  return isEmpty(r, c) &&
         insertion >= 1 && insertion <= 9 &&
         rowRequiredSets[r].count(insertion) &&
         colRequiredSets[c].count(insertion) &&
         boxRequiredSets[r - (r%3) + ((c - (c%3)) / 3)].count(insertion);
}

bool Puzzle::insert(int insertion, int r, int c) {
  // attempt to insert a value into the board, checking for
  // validity. Return the success value
  if (!isValidInsertion(insertion, r, c)) {
    return false;
  }

  board[r][c] = insertion;

  // update cells in column
  for (auto& row : cellPotentialSets) {
    row[c].erase(insertion);
  }

  // update cells in row
  for (auto& col : cellPotentialSets[r]) {
    col.erase(insertion);
  }

  // update cells in minigrid
  int top{r - (r % 3)};
  int left{c - (c % 3)};
  for (int row{top}; row < top + 3; ++row) {
    for (int col{left}; col < left + 3; ++col) {
      cellPotentialSets[row][col].erase(insertion);
    }
  }

  // clear current cell, update row, col and minigrid
  cellPotentialSets[r][c].clear();
  rowRequiredSets[r].erase(insertion);
  colRequiredSets[c].erase(insertion);
  boxRequiredSets[r - (r%3) + ((c - (c%3)) / 3)].erase(insertion);

  return true;
}

bool Puzzle::solve(bool useThreads) {
  // Alternate take turns applying various strategies until none of
  // them work anymore
  while(checkSingularCandidates() ||
        checkSingularPossibilities() ||
        checkPreemptiveSets() ||
        checkPointers());

  // if not  solved, find cell  with smallest set of  potential values
  // for backtracking
  if (!isComplete()) {

    std::size_t least{10};
    int leastR{0}, leastC{0};
    for (int r{0}; r < 9 && least != 2; ++r) {
      for (int c{0}; c < 9 && least != 2; ++c) {
        if (cellPotentialSets[r][c].size() < least &&
            cellPotentialSets[r][c].size() >= 2) {
          least = cellPotentialSets[r][c].size();
          leastR = r;
          leastC = c;
        }
      }
    }

    if (least == 10) {
      return false;
    }

    // Attain values from that set
    std::vector<std::thread> threads;
    Puzzle checker(*this);
    for (auto& value : cellPotentialSets[leastR][leastC]) {
      Puzzle temp(*this); // create backtracking copy
      temp.insert(value, leastR, leastC); // insert into backtracking copy
      if (useThreads) {
        threads.emplace_back(solveThread, &checker, temp);
      }
      else {
        // If the backtracking copy gets solved propogate those values
        // back up the stack and return solved!
        if (temp.solve(false)) {
          (*this) = temp;
          return true;
        }
      }
    }

    if (useThreads) {
      for (std::thread& t : threads) {
        t.join();
      }

      if (checker.isComplete()) {
        (*this) = checker;
        return true;
      }
    }

    return false;
  }

  // if it reaches here then the strategies solved it!
  return true;
}

void Puzzle::solveThread(Puzzle* checker, Puzzle possibility) {
  if (possibility.solve(false)) {
    (*checker) = possibility;
  }
}

// Check for cells with only one potential value
bool Puzzle::checkSingularCandidates() {
  bool changedSomething{false};
  // loop through all the cells
  for (int r{0}; r < 9; ++r) {
    for (int c{0}; c < 9; ++c) {
      // if the potential set only has one value then it must be set
      // to that value
      if (cellPotentialSets[r][c].size() == 1) {
        insert(*cellPotentialSets[r][c].begin(), r, c);
        changedSomething = true;
      }
    }
  }

  // return whether the strategy yielded results
  return changedSomething;
}

// given a region of cells (locations), and a set of values yet
// unfulfilled in the region (requiredSet), this function will find
// and fill all cells in the region which stand alone as candidates
// for those valuesb
bool Puzzle::checkSingularPossibilities(const std::unordered_set<int> requiredSet,
                                        const std::set<std::pair<int,int>>& region) {
  bool changedSomething{false};

  for (int requirement : requiredSet) { // loop over valuess required by region
    bool singleCandidate{false};
    std::pair<int, int> candidate;
    for (const std::pair<int, int>& coordinate : region) {
      // cell is viable for requirement
      if (cellPotentialSets[coordinate.first][coordinate.second].count(requirement)) {
        if (!singleCandidate) { // no candidate has been found yet
          singleCandidate = true;
          candidate = coordinate;
        }
        else { // a candidate was already found
          singleCandidate = false;
          break;
        }
      }
    }

    // only one viable candidate was found for the requirement
    if (singleCandidate) {
      insert(requirement, candidate.first, candidate.second);
      changedSomething = true;
    }
  }


  return changedSomething;
}

// check for regions with only one candidate for an unfulfilled
// requirement
bool Puzzle::checkSingularPossibilities() {
  bool changedSomething{false};
  std::set<std::pair<int, int>> region;

  //check rows
  for (int r{0}; r < 9; ++r) {
    for (int c{0}; c < 9; ++c) {
      region.insert(std::make_pair(r, c));
    }
    if (checkSingularPossibilities(rowRequiredSets[r], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  // check columns
  for (int c{0}; c < 9; ++c) {
    for (int r{0}; r < 9; ++r) {
      region.insert(std::make_pair(r, c));
    }
    if (checkSingularPossibilities(colRequiredSets[c], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  // check sub-boxes
  for (int b{0}; b < 9; ++b) {
    // copy the set because it will be modified within the loop
    for (int r{0}; r < 3; ++r) { // box coordinate
      int row{r + ((b / 3) * 3)}; // transform to board coord
      for (int c{0}; c < 3; ++c) { // box coordinate
        int col{c + ((b % 3) * 3)}; // transform to board coord
        region.insert(std::make_pair(row, col));
      }
    }
    if (checkSingularPossibilities(boxRequiredSets[b], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  return changedSomething;
}

// look for n-tuples of potential values found in n cells within a
// region. If found, then values outside of the tuple can be removed
// from those cells
bool Puzzle::checkPreemptiveSets(const std::unordered_set<int> requiredSet,
                                 const std::set<std::pair<int,int>>& region) {
  bool changedSomething{false};
  std::set<std::pair<int, int>> locations;
  std::unordered_set<int> combination;

  for (int requirement : requiredSet) {
    combination.insert(requirement);
    // find cells with potential for the current requirement
    for (const std::pair<int, int>& coordinate : region) {
      if (cellPotentialSets[coordinate.first][coordinate.second].count(requirement)) {
        locations.insert(coordinate);
      }
    }

    // put any other candidate found at those locations in the combination
    for (const std::pair<int, int>& location : locations) {
      auto& potentials = cellPotentialSets[location.first][location.second];
      combination.insert(potentials.begin(), potentials.end());
    }

    // erase those values from combination found outside of the values
    // in locations
    auto temp = combination;
    for (int val : temp) {
      for (const std::pair<int, int>& coordinate : region) {
        if (!locations.count(coordinate)) {
          if (cellPotentialSets[coordinate.first][coordinate.second].count(val)) {
            combination.erase(val);
            break;
          }
        }
      }
    }

    // if you have found a tuple
    if (combination.size() == locations.size()) {
      for (const std::pair<int, int>& coordinate : locations) {
        // remove those values of combination from the cells
        // which belong to the complement of locations
        auto temp = cellPotentialSets[coordinate.first][coordinate.second];
        for (int val : temp) {
          if (!combination.count(val)) {
            cellPotentialSets[coordinate.first][coordinate.second].erase(val);
            changedSomething = true;
          }
        }
      }
    }

  }

  return changedSomething;
}

// look for n-tuples of potential values found in n cells within a
// region. If found, then values outside of the tuple can be removed
// from those cells
bool Puzzle::checkPreemptiveSets() {
  bool changedSomething{false};
  std::set<std::pair<int, int>> region;

  // check rows
  for (int r{0}; r < 9; ++r) {
    for (int c{0}; c < 9; ++c) {
      region.insert(std::make_pair(r, c));
    }
    if (checkPreemptiveSets(rowRequiredSets[r], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  // check columns
  for (int c{0}; c < 9; ++c) {
    for (int r{0}; r < 9; ++r) {
      region.insert(std::make_pair(r, c));
    }
    if (checkPreemptiveSets(colRequiredSets[c], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  // check boxes
  for (int b{0}; b < 9; ++b) {
    for (int r{0}; r < 3; ++r) { // box coordinate
      int row{r + ((b / 3) * 3)}; // transform to board coord
      for (int c{0}; c < 3; ++c) { // box coordinate
        int col{c + ((b % 3) * 3)}; // transform to board coord
        region.insert(std::make_pair(row, col));
      }
    }
    if (checkPreemptiveSets(boxRequiredSets[b], region)) {
      changedSomething = true;
    }
    region.clear();
  }

  return changedSomething;
}

// Look for sets of potential candidates belonging to two regions. If
// all candidates for a requirement are found in one region then they
// can be eliminated from cells outside that region in the
// intersecting region
bool Puzzle::checkPointers() {
  bool somethingChanged{false};

  // check for pointers in boxes aligned in rows or columns
  for (int b{0}; b < 9; ++b) {
    for (auto val : boxRequiredSets[b]) {
      std::unordered_set<int> locationsR;
      std::unordered_set<int> locationsC;
      int numberFound{0};
      for (int r{0}; r < 3; ++r) { // box coordinate
        int row{r + ((b / 3) * 3)}; // transform to board coord
        for (int c{0}; c < 3; ++c) { // box coordinate
          int col{c + ((b % 3) * 3)}; // transform to board coord
          if (cellPotentialSets[row][col].count(val)) { // value is in cell
            locationsR.insert(row);
            locationsC.insert(col);
            ++numberFound;
          }
        }
      }

      if (numberFound == 2 || numberFound == 3) { // triple or double
        if (locationsR.size() == 1) { // In same row
          for (int c{0}; c < 9; ++c) {
            // remove this possibility from all columns outside of the
            // box in the row
            if (!locationsC.count(c)) {
              if (cellPotentialSets[*locationsR.begin()][c].count(val)) {
                cellPotentialSets[*locationsR.begin()][c].erase(val);
                somethingChanged = true;
              }
            }
          }
        }

        if (locationsC.size() == 1) { // In same column
          for (int r{0}; r < 9; ++r) {
            // remove this possibility from all rows outside of the
            // box in the column
            if (!locationsR.count(r)) {
              if (cellPotentialSets[r][*locationsC.begin()].count(val)) {
                cellPotentialSets[r][*locationsC.begin()].erase(val);
                somethingChanged = true;
              }
            }
          }
        }
      }

    }
  }

  // check for pointers aligned on rows, contained in boxes
  for (int r{0}; r < 9; ++r) {
    for (auto val : rowRequiredSets[r]) {
      std::unordered_set<int> locations;
      int numberFound{0};
      for (int c{0}; c < 9; ++c) {
        if (cellPotentialSets[r][c].count(val)) {
          locations.insert(r - (r % 3) + ((c - (c % 3)) / 3));
          ++numberFound;
        }
      }

      if (locations.size() == 1 && // double or triple in same box
          (numberFound == 2 || numberFound == 3)) {
        int b = *locations.begin();
        for (int ri{0}; ri < 3; ++ri) { // box coordinate
          int row{ri + ((b / 3) * 3)}; // transform to board coord
          for (int c{0}; c < 3; ++c) { // box coordinate
            int col{c + ((b % 3) * 3)}; // transform to board coord
            if (row != r) { // delete possiblities from other rows in box
              if (cellPotentialSets[row][col].count(val)) {
                cellPotentialSets[row][col].erase(val);
                somethingChanged = true;
              }
            }
          }
        }
      }
    }
  }

  // check for pointers aligned on columns, contained in boxes
  for (int c{0}; c < 9; ++c) {
    for (auto val : colRequiredSets[c]) {
      std::unordered_set<int> locations;
      int numberFound{0};
      for (int r{0}; r < 9; ++r) {
        if (cellPotentialSets[r][c].count(val)) {
          locations.insert(r - (r % 3) + ((c - (c % 3)) / 3));
          ++numberFound;
        }
      }

      if (locations.size() == 1 && // double or triple in same box
          (numberFound == 2 || numberFound == 3)) {
        int b = *locations.begin();
        for (int r{0}; r < 3; ++r) { // box coordinate
          int row{r + ((b / 3) * 3)}; // transform to board coord
          for (int ci{0}; ci < 3; ++ci) { // box coordinate
            int col{ci + ((b % 3) * 3)}; // transform to board coord
            if (col != c) { // delete possiblities from other rows in box
              if (cellPotentialSets[row][col].count(val)) {
                cellPotentialSets[row][col].erase(val);
                somethingChanged = true;
              }
            }
          }
        }
      }
    }
  }

  return somethingChanged;
}

// determine whether the value should be interpreted as empty
inline bool Puzzle::isEmpty(int row, int col) const {
  // determine whether the cell should be interpreted as empty
  return board[row][col] == 0;
}

inline bool Puzzle::isEmpty(const int& val) const {
  return val == 0;
}

void Puzzle::print() const {
  for (auto r = board.cbegin(); r != board.cend(); ++r) {
    for (auto c = r->cbegin(); c+1 != r->cend(); ++c) {
      std::cout << *c << ' ';
    }
    std::cout << *(r->end()-1) << "\n";
  }
}


