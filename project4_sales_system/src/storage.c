#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "utils.h"

StorageStatus storage_save(const TransactionStore *s, const char *path)
{
    FILE *fp = fopen(path, "w");

    if (fp == NULL)
        return STORAGE_ERR_OPEN;
    if (fprintf(fp, "# id|product|quantity|unit_price|date\n") < 0) {
        fclose(fp);
        return STORAGE_ERR_WRITE;
    }
    for (size_t i = 0; i < s->count; i++) {
        const Transaction *t = &s->items[i];
        if (fprintf(fp, "%d|%s|%d|%.4f|%s\n", t->id, t->product, t->quantity, t->unit_price, t->date) < 0) {
            fclose(fp);
            return STORAGE_ERR_WRITE;
        }
    }
    if (fflush(fp) != 0 || ferror(fp)) {
        fclose(fp);
        return STORAGE_ERR_WRITE;
    }
    if (fclose(fp) != 0)
        return STORAGE_ERR_WRITE;
    return STORAGE_OK;
}

StorageStatus storage_load(TransactionStore *dest, const char *path, LoadStats *stats)
{
    FILE *fp = fopen(path, "r");
    TransactionStore tmp;
    char line[256];

    stats->loaded = 0;
    stats->skipped = 0;
    if (fp == NULL)
        return STORAGE_ERR_OPEN;
    if (store_init(&tmp, INITIAL_CAPACITY) != STORE_OK) {
        fclose(fp);
        return STORAGE_ERR_MEMORY;
    }

    while (fgets(line, sizeof line, fp) != NULL) {
        Transaction t;
        StoreStatus st;

        trim(line);
        if (line[0] == '\0' || line[0] == '#')
            continue;
        memset(&t, 0, sizeof t);
        if (sscanf(line, "%d|%49[^|]|%d|%lf|%10s", &t.id, t.product, &t.quantity, &t.unit_price, t.date) != 5) {
            stats->skipped++;
            continue;
        }
        st = store_add(&tmp, &t);
        if (st == STORE_ERR_NOMEM) {
            store_free(&tmp);
            fclose(fp);
            return STORAGE_ERR_MEMORY;
        }
        if (st == STORE_OK)
            stats->loaded++;
        else
            stats->skipped++;
    }
    if (ferror(fp)) {
        store_free(&tmp);
        fclose(fp);
        return STORAGE_ERR_READ;
    }
    fclose(fp);

    store_free(dest);       /* only now is the old data released */
    *dest = tmp;
    return STORAGE_OK;
}

const char *storage_status_message(StorageStatus st)
{
    switch (st) {
    case STORAGE_OK:         return "Done.";
    case STORAGE_ERR_OPEN:   return "Could not open the file (missing or no permission).";
    case STORAGE_ERR_READ:   return "A read error occurred.";
    case STORAGE_ERR_WRITE:  return "A write error occurred.";
    case STORAGE_ERR_MEMORY: return "Out of memory while loading.";
    }
    return "Unknown error.";
}
