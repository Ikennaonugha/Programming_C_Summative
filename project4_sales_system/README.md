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
│   ├── sample_run_input.txt    Keystrokes used for the demonstration run
│   └── sample_run_output.txt   Full program output (evidence)
└── Makefile
```

