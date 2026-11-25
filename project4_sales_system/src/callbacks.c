#include <stdio.h>
#include "callbacks.h"
#include "utils.h"

/* ---------- Higher-order functions (they invoke the callbacks they receive) ---------- */

size_t txn_count_if(const TransactionStore *s, TxnPredicate pred, const void *ctx)
{
    size_t n = 0;
    for (size_t i = 0; i < s->count; i++)
        if (pred(&s->items[i], ctx))          /* callback invoked here */
            n++;
    return n;
}

size_t txn_for_each_match(const TransactionStore *s, TxnPredicate pred, const void *ctx, TxnVisitor visit)
{
    size_t n = 0;
    for (size_t i = 0; i < s->count; i++) {
        if (pred(&s->items[i], ctx)) {        /* predicate callback */
            visit(&s->items[i]);              /* visitor callback */
            n++;
        }
    }
    return n;
}

double txn_sum_if(const TransactionStore *s, TxnPredicate pred, const void *ctx)
{
    double sum = 0.0;
    for (size_t i = 0; i < s->count; i++)
        if (pred(&s->items[i], ctx))
            sum += s->items[i].total;
    return sum;
}

/* ---------- Predicates ---------- */

int pred_total_above(const Transaction *t, const void *ctx)
{
    return t->total > *(const double *)ctx;
}

int pred_product_equals(const Transaction *t, const void *ctx)
{
    return str_equal_ci(t->product, (const char *)ctx);
}

int pred_quantity_at_least(const Transaction *t, const void *ctx)
{
    return t->quantity >= *(const int *)ctx;
}

/* ---------- Comparators ---------- */

int cmp_id_asc(const Transaction *a, const Transaction *b)
{
    return (a->id > b->id) - (a->id < b->id);
}

int cmp_product_asc(const Transaction *a, const Transaction *b)
{
    return str_compare_ci(a->product, b->product);
}

int cmp_total_asc(const Transaction *a, const Transaction *b)
{
    return (a->total > b->total) - (a->total < b->total);
}

int cmp_total_desc(const Transaction *a, const Transaction *b)
{
    return cmp_total_asc(b, a);
}

/* ---------- Visitor ---------- */

void visit_print_row(const Transaction *t)
{
    transaction_print(t);
}
