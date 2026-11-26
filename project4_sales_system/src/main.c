#include <stdio.h>
#include "storage.h"
#include "store.h"
#include "ui.h"

#define DEFAULT_DATA_FILE "data/transactions.txt"

int main(int argc, char *argv[])
{
    AppState app;
    LoadStats stats;
    StorageStatus st;

    app.path = (argc > 1) ? argv[1] : DEFAULT_DATA_FILE;
    app.dirty = 0;

    if (store_init(&app.store, INITIAL_CAPACITY) != STORE_OK) {
        fprintf(stderr, "Fatal: could not allocate memory for transactions.\n");
        return 1;
    }

    st = storage_load(&app.store, app.path, &stats);
    if (st == STORAGE_OK) {
        printf("Loaded %zu transaction(s) from %s", stats.loaded, app.path);
        if (stats.skipped)
            printf(" (%zu invalid or duplicate line(s) skipped)", stats.skipped);
        printf(".\n");
    } else if (st == STORAGE_ERR_OPEN) {
        printf("No saved data found at %s. Starting with an empty record.\n", app.path);
    } else {
        fprintf(stderr, "Warning: %s Starting with an empty record.\n", storage_status_message(st));
        if (app.store.items == NULL && store_init(&app.store, INITIAL_CAPACITY) != STORE_OK) {
            fprintf(stderr, "Fatal: out of memory.\n");
            return 1;
        }
    }

    ui_run(&app);
    store_free(&app.store);
    return 0;
}
