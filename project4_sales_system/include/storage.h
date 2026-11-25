#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include "store.h"

typedef enum {
    STORAGE_OK = 0,
    STORAGE_ERR_OPEN,
    STORAGE_ERR_READ,
    STORAGE_ERR_WRITE,
    STORAGE_ERR_MEMORY
} StorageStatus;

typedef struct {
    size_t loaded;
    size_t skipped;   /* malformed, invalid or duplicate lines */
} LoadStats;

StorageStatus storage_save(const TransactionStore *s, const char *path);
/* Replaces the contents of dest only if the whole load succeeds. */
StorageStatus storage_load(TransactionStore *dest, const char *path, LoadStats *stats);
const char *storage_status_message(StorageStatus st);

#endif
