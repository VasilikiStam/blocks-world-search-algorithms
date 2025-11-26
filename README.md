# Blocks World Search Algorithms

## Description

Implementation of classical search algorithms (**BFS**, **DFS**, **Best-First**, **A\***)
in C for the Blocks World problem.  
The program reads a `.pddl` problem file and outputs the solution steps to a text file.

---

## Algorithms

- **Breadth-First Search (BFS)** — Complete, optimal for uniform step cost.
- **Depth-First Search (DFS)** — Not complete in infinite/very large spaces.
- **Best-First Search** — Uses heuristic `h(n)`.
- **A\*** — Uses evaluation `f(n) = g(n) + h(n)`.

---

## Compilation & Execution

### **Compile**
gcc src/AI.c -o blocks_solver
### **Run**
The program requires **three arguments**:
./blocks_solver <algorithm> <input_file> <output_file>

### **Example**
./blocks_solver astar input.pddl solution.txt

### **Supported algorithms**
- `breadth`
- `depth`
- `best`
- `astar`

---

## Input Format

The input must be a `.pddl`-style Blocks World problem containing:

(:objects ...)
(:init ...)
(:goal ...)

Example:
(:objects A B C)
(:init (on A B) (on B C))
(:goal (on C B) (on B A))

---

## Output

The program prints:
- the steps of the solution on the terminal  
- and writes the solution path to the output file  
(e.g., `solution.txt`)

---

## Project Structure

src/
AI.c # Main implementation (parsing + search)
docs/ # Optional documentation folder
README.md

---

## Author
Vasiliki Stam – Blocks World Search Algorithms (C)

