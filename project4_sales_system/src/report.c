#include <stdio.h>
#include "report.h"
#include "operations.h"
#include "callbacks.h"

void report_print_summary(const TransactionStore *s)
{
    OpResult revenue, average, high, low, units;

    printf("\n========== SALES REPORT ==========\n");
    if (s->count == 0) {
        printf("No transactions recorded. Nothing to report.\n");
        return;
    }
    revenue = op_total_revenue(s);
    average = op_average_value(s);
    high    = op_highest_value(s);
    low     = op_lowest_value(s);
    units   = op_total_units(s);

    printf("Total transactions      : %zu\n", s->count);
    printf("Total revenue           : %.2f\n", revenue.value);
    printf("Average transaction     : %.2f\n", average.value);
    printf("Highest transaction     : %.2f (ID %d, %s)\n", high.value, high.txn->id, high.txn->product);
    printf("Lowest transaction      : %.2f (ID %d, %s)\n", low.value, low.txn->id, low.txn->product);
    printf("Total units sold        : %.0f\n", units.value);
}

double report_product_revenue(const TransactionStore *s, const char *product, size_t *matches)
{
    *matches = txn_count_if(s, pred_product_equals, product);
    return txn_sum_if(s, pred_product_equals, product);
}
