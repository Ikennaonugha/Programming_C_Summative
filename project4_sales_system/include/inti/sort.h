#ifndef SORT_H
#define SORT_H

#include "store.h"
#include "callbacks.h"

/* Manual (stable) insertion sort. The ordering is supplied by a comparison callback.
   qsort() is deliberately not used. */
void sort_transactions(TransactionStore *s, TxnCompare cmp);

#endif
