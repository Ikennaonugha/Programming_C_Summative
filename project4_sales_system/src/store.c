#include <stdlib.h>
#include <string.h>
#include "store.h"

static int find_index(const TransactionStore *s, int id)
{
    for (size_t i = 0; i < s->count; i++)
        if (s->items[i].id == id)
            return (int)i;
    return -1;
}

StoreStatus store_init(TransactionStore *s, size_t capacity)
{
    if (capacity == 0)
        capacity = INITIAL_CAPACITY;
    s->items = calloc(capacity, sizeof *s->items);
    s->count = 0;
    s->capacity = 0;
    if (s->items == NULL)
        return STORE_ERR_NOMEM;
    s->capacity = capacity;
    return STORE_OK;
}

void store_free(TransactionStore *s)
{
    free(s->items);
    s->items = NULL;
    s->count = 0;
    s->capacity = 0;
}

StoreStatus store_add(TransactionStore *s, const Transaction *t)
{
    Transaction rec;
    Transaction *p = NULL;

    if (transaction_validate(t) != NULL)
        return STORE_ERR_INVALID;
    if (find_index(s, t->id) >= 0)
        return STORE_ERR_DUPLICATE;

    if (s->count == s->capacity) {                       /* grow: realloc */
        size_t new_cap = s->capacity ? s->capacity * 2 : INITIAL_CAPACITY;
        Transaction *tmp = realloc(s->items, new_cap * sizeof *tmp);
        if (tmp == NULL)
            return STORE_ERR_NOMEM;                      /* old block is still valid */
        s->items = tmp;
        s->capacity = new_cap;
    }

    rec = *t;
    transaction_compute_total(&rec);
    p = s->items + s->count;                             /* pointer arithmetic */
    *p = rec;
    s->count++;
    return STORE_OK;
}

StoreStatus store_update(TransactionStore *s, int id, const Transaction *updated)
{
    int idx = find_index(s, id);
    Transaction rec;

    if (idx < 0)
        return STORE_ERR_NOT_FOUND;
    rec = *updated;
    rec.id = id;                                         /* the ID never changes */
    if (transaction_validate(&rec) != NULL)
        return STORE_ERR_INVALID;
    transaction_compute_total(&rec);
    s->items[idx] = rec;
    return STORE_OK;
}

StoreStatus store_delete(TransactionStore *s, int id)
{
    int idx = find_index(s, id);

    if (idx < 0)
        return STORE_ERR_NOT_FOUND;
    for (size_t i = (size_t)idx; i + 1 < s->count; i++)  /* close the gap */
        s->items[i] = s->items[i + 1];
    s->count--;
    return STORE_OK;
}

const char *store_status_message(StoreStatus st)
{
    switch (st) {
    case STORE_OK:            return "Done.";
    case STORE_ERR_DUPLICATE: return "A transaction with that ID already exists.";
    case STORE_ERR_NOMEM:     return "Out of memory.";
    case STORE_ERR_NOT_FOUND: return "Transaction not found.";
    case STORE_ERR_INVALID:   return "Invalid transaction data.";
    }
    return "Unknown error.";
}
