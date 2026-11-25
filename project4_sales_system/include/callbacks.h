#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <stddef.h>
#include "store.h"

/* ---- Callback types ---- */
/* Decides whether a record matches; ctx carries the condition's parameter. */
typedef int  (*TxnPredicate)(const Transaction *t, const void *ctx);
/* Called for every matching record. */
typedef void (*TxnVisitor)(const Transaction *t);
/* Ordering callback used by the sort routine (<0, 0, >0 like strcmp). */
typedef int  (*TxnCompare)(const Transaction *a, const Transaction *b);

/* ---- Higher-order functions: they receive callbacks and invoke them ---- */
size_t txn_count_if(const TransactionStore *s, TxnPredicate pred, const void *ctx);
size_t txn_for_each_match(const TransactionStore *s, TxnPredicate pred, const void *ctx, TxnVisitor visit);
double txn_sum_if(const TransactionStore *s, TxnPredicate pred, const void *ctx);

/* ---- Predicate callbacks ---- */
int pred_total_above(const Transaction *t, const void *ctx);       /* ctx: const double * */
int pred_product_equals(const Transaction *t, const void *ctx);    /* ctx: const char *   */
int pred_quantity_at_least(const Transaction *t, const void *ctx); /* ctx: const int *    */

/* ---- Comparison callbacks ---- */
int cmp_id_asc(const Transaction *a, const Transaction *b);
int cmp_product_asc(const Transaction *a, const Transaction *b);
int cmp_total_asc(const Transaction *a, const Transaction *b);
int cmp_total_desc(const Transaction *a, const Transaction *b);

/* ---- Visitor callback ---- */
void visit_print_row(const Transaction *t);

#endif
