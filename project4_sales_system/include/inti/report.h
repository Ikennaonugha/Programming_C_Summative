#ifndef REPORT_H
#define REPORT_H

#include "store.h"

void   report_print_summary(const TransactionStore *s);
/* Revenue for one product; *matches receives the number of matching transactions. */
double report_product_revenue(const TransactionStore *s, const char *product, size_t *matches);

#endif
