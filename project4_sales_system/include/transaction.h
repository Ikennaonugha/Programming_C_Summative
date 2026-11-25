#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stddef.h>

#define PRODUCT_NAME_LEN 50
#define DATE_LEN 11            /* "YYYY-MM-DD" + '\0' */
#define MAX_QUANTITY 1000000
#define MAX_UNIT_PRICE 1000000000.0

/* One sales transaction record. */
typedef struct {
    int    id;                         /* unique, > 0 */
    char   product[PRODUCT_NAME_LEN];  /* product name */
    int    quantity;                   /* units sold, > 0 */
    double unit_price;                 /* price per unit, > 0 */
    double total;                      /* quantity * unit_price (derived) */
    char   date[DATE_LEN];             /* YYYY-MM-DD */
} Transaction;

/* Returns 1 if s is a real calendar date in YYYY-MM-DD format. */
int is_valid_date(const char *s);

/* Returns NULL if the record is valid, otherwise a message describing the problem. */
const char *transaction_validate(const Transaction *t);

/* Recomputes t->total from quantity and unit price. */
void transaction_compute_total(Transaction *t);

/* Table output helpers. */
void transaction_print_header(void);
void transaction_print(const Transaction *t);

#endif
