#include <math.h>
#include <stdlib.h>
#include "search.h"
#include "utils.h"

int search_by_id(const TransactionStore *s, int id)
{
    for (size_t i = 0; i < s->count; i++)
        if (s->items[i].id == id)
            return (int)i;
    return -1;
}

size_t *search_by_product(const TransactionStore *s, const char *name, size_t *n, int *oom)
{
    size_t *hits;
    *n = 0;
    *oom = 0;
    if (s->count == 0)
        return NULL;
    hits = malloc(s->count * sizeof *hits);
    if (hits == NULL) {
        *oom = 1;
        return NULL;
    }
    for (size_t i = 0; i < s->count; i++)
        if (str_contains_ci(s->items[i].product, name))
            hits[(*n)++] = i;
    if (*n == 0) {
        free(hits);
        return NULL;
    }
    return hits;
}

size_t *search_by_amount(const TransactionStore *s, double amount, size_t *n, int *oom)
{
    size_t *hits;
    *n = 0;
    *oom = 0;
    if (s->count == 0)
        return NULL;
    hits = malloc(s->count * sizeof *hits);
    if (hits == NULL) {
        *oom = 1;
        return NULL;
    }
    for (size_t i = 0; i < s->count; i++)
        if (fabs(s->items[i].total - amount) < 0.005)   /* money compared to the cent */
            hits[(*n)++] = i;
    if (*n == 0) {
        free(hits);
        return NULL;
    }
    return hits;
}
