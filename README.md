<div id="table-of-contents">
<h2>Table of Contents</h2>
<div id="text-table-of-contents">
<ul>
<li><a href="#sec-1">1. How to Use</a></li>
<li><a href="#sec-2">2. Internals</a></li>
<li><a href="#sec-3">3. Verification</a></li>
<li><a href="#sec-4">4. What Worked Well And What Did Not</a></li>
</ul>
</div>
</div>

# How to Use<a id="sec-1" name="sec-1"></a>

-   Run make in the lab6 directory.
-   Run "./sudoku.exe < FilenameOfPuzzle.txt"
    -   sudoku.exe reads from stdin for ease of testing. The command
        above redirects text from a file to stdin
    -   The file must be a correctly formatted sudoku puzzle that is not
        already unsolvable. If any of these conditions are not met then
        an error will occur and the program will exit.
-   The best attempt at a solution the program can make will be
    displayed with formatting equivalent to an input file

# Internals<a id="sec-2" name="sec-2"></a>

-   The solver requires new data structures: sets of values which may
    potentially be held by a cell and sets of values still required by
    the regions (rows, columns, and boxes). These are implemented with
    unordered_sets of T.
-   4 strategies for solving a sudoku are implemented:

      Note: upon insertion of a value, that value is removed from the
    sets of required values for the relevant regions and removed
    from the sets of potential values of all cells in those regions.

    1.  checkSingularCandidates
        -   This iterates over every cell and checks whether it has only
            a single potential value. If so, then that value is inserted
            into the cell.

    2.  checkSingluarPossibilities
        -   This looks at every required value for every region and if
            that value has only one candidate cell in the region then
            that value is inserted into the candidate cell.

    3.  checkPreemptiveSets
        -   This looks for n-tuples of potential values that aren't found
            outside of n cells in a region.
        -   If found, then those n cells must be occupied by some
            permutation of the n-tuple values. This deduction allows us
            to eliminate the complement of that n-tuple from those n
            cells

    4.  checkPointers
        -   This looks at all candidates for a required value in a
            region. If there are only 2 or 3 candidates and they all
            inhabit a second region simultaneously, then that value can
            be eliminated from every cell in the second region not
            intersecting with the first region.

-   The first strategy is employed over and over until it cannot add any
    new information to the puzzle. Then the next strategy has a
    similar turn. If any subsequent strategy discovers new information
    then flow will immediately return to the first strategy, restarting the
    process.

-   If none of the strategies can provide new information, then a
    recursive and multi-threaded backtracking strategy is employed.
    1.  First, the cell with the smallest set of potential values is
        found. If there are no cells left with potential values then
        the recursion stack dies.

    2.  If a set is found in step 1, then all values of that set are
        iterated through. If the whole set has been iterated through
        without a solution, then we can conclude no value for that cell
        will lead to a solvable state and the chain of threads dies.

    3.  For each value in the set of potentials, a temporary Puzzle is
        created where that value has been inserted into the respective
        cell. At first, a new thread is created for the execution of
        that Puzzle's solve() via solve(true). After that first level
        of mutli-threading is done then recursion is employed by each
        thread through solve(false). If a solution is reached, then
        values are moved up the recursive stack to another temporary
        Puzzle shared by all threads. This mix of multi-threading and
        recursion keeps the program from creating too many threads; a
        set of 2, 3, or 4 potential values is usually found for thread
        creation.

    4.  Execution pauses for the child threads to complete their
        execution. If that shared temporary Puzzle is in a solved
        state, then its values are transferred to the parent thread's
        Puzzle and it returns true. Other wise it returns false, the
        Puzzle as it was could not be solved.

# Verification<a id="sec-3" name="sec-3"></a>

-   This was done by checking a fair number of problems generated on
    <http://www.sudokuwiki.org>
-   isValidInsertion() also forces any insertion to obey the rules of
    the puzzle, barring errors from propagating
-   I wrote a testing script which, with the help of repositories at
    <http://www2.warwick.ac.uk/fac/sci/moac/people/students/peter_cock/python/sudoku>
    <http://magictour.free.fr/sudoku.htm> and verifying against qqwing
    <http://qqwing.com/download.html> allows me to verify my solutions
    for thousands of easy, hard, and expert level problems in a few
    seconds! Take a look at runTests.sh and the easy, hard, and expert
    .test files! It should take <=40 seconds to run on the student
    machines depending on which tests you run (see ./runTests.sh -h
    for details). If you get weird system-created error messages and
    stuff then you're creating too many threads, both by the script
    and the program. Try running with the sequential flag (-s), and if
    that is still being weird, change the solve() in sudoku.cpp to
    solve(false) (see the top of the next section for an explanation).

# What Worked Well And What Did Not<a id="sec-4" name="sec-4"></a>

-   If you change the solve() call in sudoku.cpp to solve(false), then
    it will only run one thread utliizing recursion if necessary. This
    turns out to be faster than even the most minimal multi-threading,
    which is disappointing.
-   Even with all four of these basic strategies hard problems cannot
    be solved. They necessitate backtracking.
-   I should have utilised ordered sets to iterate through cells from
    smallest to largest potential value set size. This would
    prioritize those cells most likely to lead to new
    information. However I had done too much work by the time I came
    to this realization to go back and fundamentally alter my
    structure. However, the size of the problem is small enough that
    optimizations of that level would have taken more time than they
    would have saved
-   cellPotentialSets could be treated as the intersection of the
    respective row, col, and box required sets. But the computation is
    excessive when you could just do good bookeeping, despite
    redundancy of data.
-   I do like treating potential values as sets, and not as
    vectors. It's much more intuitive for me.
-   I wish I would have had a class for Cell and classes for the
    regions (Row Col Box). This would have made the architecture very
    pleasant. As it is now my strategy functions are much to large and
    could use the modularity.
