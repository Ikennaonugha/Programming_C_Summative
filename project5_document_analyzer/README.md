# Multi-Threaded Document Analyzer (C, POSIX threads)

Searches several text files at the same time. One thread per file; each thread
counts lines, counts case-insensitive keyword occurrences, records the matching
line numbers and writes its own report.

## Build and run

    make
    ./document_analyzer "IoT" sample_docs/document1.txt sample_docs/document2.txt sample_docs/document3.txt
    # or simply:  make test

Compile command used by the Makefile:

    gcc -std=c11 -Wall -Wextra -pedantic -O2 -pthread -o document_analyzer document_analyzer.c

Reports are written to the current directory as `<input name>_report.txt`
(e.g. `document1_report.txt`). If two inputs share a base name
(`a/doc.txt`, `b/doc.txt`) the later one gets its thread number added
(`doc_2_report.txt`) so threads never write to the same file.

## Program structure

| Function | Role |
|---|---|
| `initialize_thread_args()` | fills one `ThreadArgs` (file, keyword, report name, zeroed results) |
| `resolve_duplicate_report_names()` | avoids two threads sharing a report file |
| `process_file()` | thread entry point: search, report, status messages |
| `search_file()` / `count_occurrences()` | read lines, count lines, occurrences, matching lines |
| `generate_report()` | writes the per-file report, checks every write |
| `display_results()` | terminal summary after all threads are joined |

## Why no synchronization is needed

Each thread gets a pointer to its **own** `ThreadArgs` and writes its **own**
report file. No global variables are shared, so there is no data race.
`pthread_join()` makes each thread's results visible to `main()`.
(`printf` is thread-safe, so terminal lines may appear in a different order
between runs, but a single line is never mixed.)

## Error handling covered

- missing or unreadable input file, or a directory given as input
- empty keyword, missing arguments
- keyword not found (reported as success with 0 matches)
- report file cannot be created or written (including `fclose` failures)
- `pthread_create` failure (thread is skipped and reported)
- all successfully created threads are always joined
- out-of-memory while recording line numbers
- exit code is 0 only when every file succeeded

## Matching rules

- Case-insensitive ("IoT" matches "iot", "IOT").
- Non-overlapping; substrings count (searching "IoT" also matches inside a longer word).
- Lines of any length are supported (`getline`); a last line without `\n` is counted.
