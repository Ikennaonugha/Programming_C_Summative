#include "sort.h"

void sort_transactions(TransactionStore *s, TxnCompare cmp)
{
    for (size_t i = 1; i < s->count; i++) {
        Transaction key = s->items[i];
        size_t j = i;
        while (j > 0 && cmp(&s->items[j - 1], &key) > 0) {   /* comparison callback */
            s->items[j] = s->items[j - 1];
            j--;
        }
        s->items[j] = key;
    }
}
