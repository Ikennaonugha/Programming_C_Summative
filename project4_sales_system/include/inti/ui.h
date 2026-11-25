#ifndef UI_H
#define UI_H

#include "store.h"

typedef struct {
    TransactionStore store;
    const char *path;   /* data file */
    int dirty;          /* unsaved changes */
} AppState;

void ui_run(AppState *app);

#endif
