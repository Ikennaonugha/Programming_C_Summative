#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "store.h"

/* Linear search by ID. Returns the index, or -1 when not found. */
int search_by_id(const TransactionStore *s, int id);

/* The next two return a malloc'ed array of matching indices (caller frees) and set *n.
   They return NULL with *n == 0 when nothing matches or memory runs out;
   *oom is set to 1 only for an allocation failure. */
size_t *search_by_product(const TransactionStore *s, const char *name, size_t *n, int *oom);
size_t *search_by_amount(const TransactionStore *s, double amount, size_t *n, int *oom);

#endif
