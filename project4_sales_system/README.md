# Sales Transaction Processing and Analysis System (C)

A menu-driven C application for recording and analysing sales transactions for a small business.
It demonstrates **function pointers**, **callback functions**, **dynamic memory allocation**,
**manual searching and sorting** (no `qsort()`), **report generation** and **file persistence**.

## Build and run

Requirements: a C99 compiler (GCC/Clang) and `make`.

```bash
make            # builds ./sales_system
./sales_system  # loads data/transactions.txt automatically
./sales_system my_data.txt   # optional: use a different data file
make clean
```

No `make` (for example on Windows with MinGW)? Compile directly:

```bash
gcc -std=c99 -Wall -Wextra -Iinclude src/*.c -o sales_system -lm
```

## Project structure

```
sales-system/
├── include/            Header files (public interface of each module)
│   ├── transaction.h   Transaction record, validation, date check, printing
│   ├── store.h         Dynamic array + add/update/delete (malloc/calloc/realloc/free)
│   ├── operations.h    Function-pointer type + dispatch table of sales operations
│   ├── callbacks.h     Callback types, higher-order functions, predicates, comparators
│   ├── search.h        Manual searches (ID, product, amount)
│   ├── sort.h          Manual insertion sort driven by a comparison callback
│   ├── report.h        Sales report
│   ├── storage.h       Save/load to a text file
│   ├── utils.h         Safe input helpers and string helpers
│   └── ui.h            Menu handlers
├── src/                Implementation (one .c file per header, plus main.c)
├── data/
│   └── transactions.txt   Sample records (12 transactions), loaded at start-up
├── docs/
│   ├── EVIDENCE.md             Organised evidence: function pointers, callbacks, search, sort, report, files
│   ├── sample_run_input.txt    Keystrokes used for the demonstration run
│   └── sample_run_output.txt   Full program output (evidence)
└── Makefile
```

## How the required C concepts are implemented

| Requirement | Where | How |
|---|---|---|
| Function pointers | `operations.h/.c`, `ui.c` (`handle_process`) | `typedef OpResult (*SalesOperation)(const TransactionStore *)`. A table `OPERATIONS[]` holds the name and function pointer of each operation (total revenue, average, highest, lowest, total units). The user picks one at runtime, it is assigned to a `SalesOperation` variable and called through the pointer. |
| Callback functions | `callbacks.h/.c`, `sort.c`, `ui.c` | Higher-order functions `txn_count_if`, `txn_for_each_match` and `txn_sum_if` take a `TxnPredicate` (and a `TxnVisitor`) and invoke it per record. Predicates: amount above a value, product match, quantity at least. `sort_transactions` receives a `TxnCompare` callback (`cmp_id_asc`, `cmp_product_asc`, `cmp_total_asc`, `cmp_total_desc`). A `void *ctx` argument carries each condition's parameter. |
| Dynamic memory | `store.c`, `search.c`, `storage.c` | `calloc` creates the initial array of 4 records; `realloc` doubles the capacity when full (using a temporary pointer so the old block is not lost on failure); records are reached by pointer arithmetic; `free` releases everything (`store_free`, search result arrays, exit). Every allocation is checked and failures are reported. |
| Searching | `search.c` | Linear search by ID (exact), product name (case-insensitive, partial match) and amount (compared to the cent). |
| Sorting | `sort.c` | Hand-written stable insertion sort by ID, product name, or total (ascending/descending). `qsort()` is not used. |
| Reports | `report.c`, `operations.c` | Transaction count, total revenue, average, highest, lowest, total units, and revenue for a chosen product. |
| File handling | `storage.c` | Pipe-delimited text file. `fopen`, `fprintf`, `fflush` and `fclose` results are checked on save. Load parses line by line, skips malformed/invalid/duplicate lines and reports how many, and replaces the in-memory data only if the whole load succeeds. Data is loaded on start-up and the user is offered a save on exit if there are unsaved changes. |

## Validation and error handling

- Duplicate transaction IDs are rejected (on add and when loading from a file).
- Non-positive or non-numeric quantities and prices, empty product names and impossible dates (such as `2026-02-30`) are rejected; all numeric input is parsed strictly with `strtol`/`strtod`.
- Searching, deleting or updating a missing ID prints a message instead of failing.
- Every menu action handles an empty record set.
- Menu choices are validated; invalid choices re-display the menu.
- Allocation failures are handled in add, search and load without losing existing data.
- Missing or unreadable files are reported; a missing data file at start-up simply starts with an empty record.
- The average is guarded against division by zero.

## Data file format

One transaction per line: `id|product|quantity|unit_price|date`. Lines starting with `#` are comments.
The total amount is not stored; it is recomputed as `quantity * unit_price` on load.

```
1001|Rice (50kg bag)|3|78500.0000|2026-10-02
```

## Evidence

`docs/EVIDENCE.md` is the organised evidence document (annotated code plus real output for every required feature). `docs/sample_run_output.txt` is the complete output of a scripted session (keystrokes in
`docs/sample_run_input.txt`) showing: duplicate-ID and invalid-input rejection, add/update/delete,
function-pointer execution (`[function pointer] executing: ...`), callback processing
(`[callback] ...`), searching, all four sorts, the sales report, saving and loading. Reproduce it with:

```bash
cp -r data /tmp/demo_data
./sales_system /tmp/demo_data/transactions.txt < docs/sample_run_input.txt
```

The program was also built with `-fsanitize=address,undefined` and run on the same session with no errors or leaks reported.
