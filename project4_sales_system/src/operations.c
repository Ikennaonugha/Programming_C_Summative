#include <stddef.h>
#include "operations.h"

OpResult op_total_revenue(const TransactionStore *s)
{
    OpResult r = {0.0, NULL};
    for (size_t i = 0; i < s->count; i++)
        r.value += s->items[i].total;
    return r;
}

OpResult op_average_value(const TransactionStore *s)
{
    OpResult r = op_total_revenue(s);
    r.value = (s->count == 0) ? 0.0 : r.value / (double)s->count;   /* no division by zero */
    return r;
}

OpResult op_highest_value(const TransactionStore *s)
{
    OpResult r = {0.0, NULL};
    for (size_t i = 0; i < s->count; i++) {
        if (r.txn == NULL || s->items[i].total > r.txn->total)
            r.txn = &s->items[i];
    }
    if (r.txn) r.value = r.txn->total;
    return r;
}

OpResult op_lowest_value(const TransactionStore *s)
{
    OpResult r = {0.0, NULL};
    for (size_t i = 0; i < s->count; i++) {
        if (r.txn == NULL || s->items[i].total < r.txn->total)
            r.txn = &s->items[i];
    }
    if (r.txn) r.value = r.txn->total;
    return r;
}

OpResult op_total_units(const TransactionStore *s)
{
    OpResult r = {0.0, NULL};
    for (size_t i = 0; i < s->count; i++)
        r.value += s->items[i].quantity;
    return r;
}

const OperationEntry OPERATIONS[] = {
    {"Calculate total revenue",           op_total_revenue, 1},
    {"Calculate average transaction value", op_average_value, 1},
    {"Identify highest-value transaction", op_highest_value, 1},
    {"Identify lowest-value transaction",  op_lowest_value,  1},
    {"Calculate total units sold",        op_total_units,   0},
};
const size_t OPERATION_COUNT = sizeof OPERATIONS / sizeof OPERATIONS[0];
