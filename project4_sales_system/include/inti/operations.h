#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stddef.h>
#include "store.h"

/* Result of a sales-processing operation. txn is set when the result points at one record. */
typedef struct {
    double value;
    const Transaction *txn;
} OpResult;

/* Function pointer type: every operation has this signature. */
typedef OpResult (*SalesOperation)(const TransactionStore *s);

typedef struct {
    const char *name;
    SalesOperation run;     /* chosen and executed at runtime */
    int is_money;           /* 1: print as currency, 0: print as a count */
} OperationEntry;

OpResult op_total_revenue(const TransactionStore *s);
OpResult op_average_value(const TransactionStore *s);
OpResult op_highest_value(const TransactionStore *s);
OpResult op_lowest_value(const TransactionStore *s);
OpResult op_total_units(const TransactionStore *s);

/* Dispatch table of function pointers. */
extern const OperationEntry OPERATIONS[];
extern const size_t OPERATION_COUNT;

#endif
