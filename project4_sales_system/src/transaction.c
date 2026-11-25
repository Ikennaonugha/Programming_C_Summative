#include <math.h>
#include <stdio.h>
#include <string.h>
#include "transaction.h"

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int is_valid_date(const char *s)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int y, m, d;

    if (s == NULL || strlen(s) != 10)
        return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return 0;
        } else if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
    }
    y = (s[0]-'0')*1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0');
    m = (s[5]-'0')*10 + (s[6]-'0');
    d = (s[8]-'0')*10 + (s[9]-'0');
    if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1)
        return 0;
    return d <= days[m - 1] + (m == 2 && is_leap(y));
}

const char *transaction_validate(const Transaction *t)
{
    if (t == NULL)                         return "No transaction supplied.";
    if (t->id <= 0)                        return "Transaction ID must be a positive number.";
    if (t->product[0] == '\0')             return "Product name cannot be empty.";
    if (strchr(t->product, '|'))           return "Product name cannot contain '|'.";
    if (t->quantity <= 0)                  return "Quantity must be greater than zero.";
    if (t->quantity > MAX_QUANTITY)        return "Quantity is too large.";
    if (!isfinite(t->unit_price) || t->unit_price <= 0.0)
                                           return "Unit price must be greater than zero.";
    if (t->unit_price > MAX_UNIT_PRICE)    return "Unit price is too large.";
    if (!is_valid_date(t->date))           return "Date must be a valid date in YYYY-MM-DD format.";
    return NULL;
}

void transaction_compute_total(Transaction *t)
{
    t->total = (double)t->quantity * t->unit_price;
}

void transaction_print_header(void)
{
    printf("%-6s %-24s %8s %12s %14s  %-10s\n",
           "ID", "Product", "Qty", "Unit Price", "Total", "Date");
    printf("------ ------------------------ -------- ------------ --------------  ----------\n");
}

void transaction_print(const Transaction *t)
{
    printf("%-6d %-24.24s %8d %12.2f %14.2f  %-10s\n",
           t->id, t->product, t->quantity, t->unit_price, t->total, t->date);
}
