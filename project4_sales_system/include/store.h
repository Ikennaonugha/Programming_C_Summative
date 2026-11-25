#ifndef STORE_H
#define STORE_H

#include <stddef.h>
#include "transaction.h"

#define INITIAL_CAPACITY 4

typedef enum {
    STORE_OK = 0,
    STORE_ERR_DUPLICATE,
    STORE_ERR_NOMEM,
    STORE_ERR_NOT_FOUND,
    STORE_ERR_INVALID
} StoreStatus;

/* Dynamic array of transactions. */
typedef struct {
    Transaction *items;
    size_t count;
    size_t capacity;
} TransactionStore;

StoreStatus store_init(TransactionStore *s, size_t capacity);   /* calloc */
void        store_free(TransactionStore *s);                    /* free */
StoreStatus store_add(TransactionStore *s, const Transaction *t);  /* realloc when full */
StoreStatus store_update(TransactionStore *s, int id, const Transaction *updated);
StoreStatus store_delete(TransactionStore *s, int id);
const char *store_status_message(StoreStatus st);

#endif
