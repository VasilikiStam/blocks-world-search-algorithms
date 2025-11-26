# Blocks World Search Algorithms

## Περιγραφή
Υλοποίηση αλγορίθμων αναζήτησης (BFS, DFS, Best-First, A*) σε C
για το κλασικό πρόβλημα Blocks World.

## Αλγόριθμοι
- **BFS**: Πλήρης, βέλτιστος για ομοιόμορφες ακμές.
- **DFS**: Μη πλήρης σε άπειρους/πολύ μεγάλους χώρους.
- **Best-First Search**: Χρησιμοποιεί ευρετική h(n).
- **A\***: Χρησιμοποιεί f(n) = g(n) + h(n).

## Τρόπος Μεταγλώττισης & Εκτέλεσης
```bash
gcc src/*.c -o blocks
./blocks <initial_state> <goal_state> <algorithm>
# Παράδειγμα:
./blocks "ABC|" "A|BC" A*
