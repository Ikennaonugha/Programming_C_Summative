#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "callbacks.h"
#include "operations.h"
#include "report.h"
#include "search.h"
#include "sort.h"
#include "storage.h"
#include "utils.h"

/* ---------- helpers ---------- */

static int require_records(const AppState *app)
{
    if (app->store.count == 0) {
        printf("No transactions recorded yet.\n");
        return 0;
    }
    return 1;
}

static void print_table(const TransactionStore *s)
{
    transaction_print_header();
    for (size_t i = 0; i < s->count; i++)
        transaction_print(&s->items[i]);
    printf("(%zu record%s)\n", s->count, s->count == 1 ? "" : "s");
}

static void print_indices(const TransactionStore *s, const size_t *idx, size_t n)
{
    transaction_print_header();
    for (size_t i = 0; i < n; i++)
        transaction_print(&s->items[idx[i]]);
    printf("(%zu match%s)\n", n, n == 1 ? "" : "es");
}

static void print_menu(void)
{
    printf("\n===== SALES TRANSACTION MANAGEMENT SYSTEM =====\n");
    printf(" 1. Add New Transaction\n");
    printf(" 2. Display All Transactions\n");
    printf(" 3. Update Transaction\n");
    printf(" 4. Delete Transaction\n");
    printf(" 5. Process Sales Data\n");
    printf(" 6. Filter Transactions (Callbacks)\n");
    printf(" 7. Search Transactions\n");
    printf(" 8. Sort Transactions\n");
    printf(" 9. Generate Sales Report\n");
    printf("10. Save Transactions\n");
    printf("11. Load Transactions\n");
    printf(" 0. Exit\n");
}

/* ---------- 1. add ---------- */

static void handle_add(AppState *app)
{
    Transaction t;
    StoreStatus st;

    memset(&t, 0, sizeof t);
    if (!prompt_int("Transaction ID: ", 1, 2000000000, &t.id)) return;
    if (search_by_id(&app->store, t.id) >= 0) {
        printf("Rejected: transaction ID %d already exists.\n", t.id);
        return;
    }
    if (!prompt_string("Product name: ", t.product, sizeof t.product)) return;
    if (!prompt_int("Quantity sold: ", 1, MAX_QUANTITY, &t.quantity)) return;
    if (!prompt_positive_double("Unit price: ", MAX_UNIT_PRICE, &t.unit_price)) return;
    if (!prompt_date("Date (YYYY-MM-DD): ", t.date, sizeof t.date)) return;

    st = store_add(&app->store, &t);
    if (st == STORE_OK) {
        app->dirty = 1;
        printf("Transaction %d added (total %.2f).\n", t.id, t.quantity * t.unit_price);
    } else {
        printf("Could not add transaction: %s\n", store_status_message(st));
    }
}

/* ---------- 3. update ---------- */

static void handle_update(AppState *app)
{
    int id, idx;
    char buf[128];
    Transaction t;
    StoreStatus st;

    if (!require_records(app)) return;
    if (!prompt_int("ID of transaction to update: ", 1, 2000000000, &id)) return;
    idx = search_by_id(&app->store, id);
    if (idx < 0) {
        printf("Transaction %d not found.\n", id);
        return;
    }
    t = app->store.items[idx];
    printf("Current record:\n");
    transaction_print_header();
    transaction_print(&t);
    printf("Press Enter to keep the current value.\n");

    for (;;) {
        printf("Product [%s]: ", t.product);
        if (!read_line(NULL, buf, sizeof buf)) return;
        trim(buf);
        if (buf[0] == '\0') break;
        if (strchr(buf, '|') || strlen(buf) >= sizeof t.product) {
            printf("  Invalid product name (no '|', max %d characters).\n", PRODUCT_NAME_LEN - 1);
            continue;
        }
        strcpy(t.product, buf);
        break;
    }
    for (;;) {
        printf("Quantity [%d]: ", t.quantity);
        if (!read_line(NULL, buf, sizeof buf)) return;
        trim(buf);
        if (buf[0] == '\0') break;
        if (parse_int(buf, 1, MAX_QUANTITY, &t.quantity)) break;
        printf("  Quantity must be a whole number from 1 to %d.\n", MAX_QUANTITY);
    }
    for (;;) {
        printf("Unit price [%.2f]: ", t.unit_price);
        if (!read_line(NULL, buf, sizeof buf)) return;
        trim(buf);
        if (buf[0] == '\0') break;
        if (parse_positive_double(buf, MAX_UNIT_PRICE, &t.unit_price)) break;
        printf("  Price must be greater than 0.\n");
    }
    for (;;) {
        printf("Date [%s]: ", t.date);
        if (!read_line(NULL, buf, sizeof buf)) return;
        trim(buf);
        if (buf[0] == '\0') break;
        if (is_valid_date(buf)) { strcpy(t.date, buf); break; }
        printf("  Invalid date. Use YYYY-MM-DD.\n");
    }

    st = store_update(&app->store, id, &t);
    if (st == STORE_OK) {
        app->dirty = 1;
        printf("Transaction %d updated.\n", id);
    } else {
        printf("Update failed: %s\n", store_status_message(st));
    }
}

/* ---------- 4. delete ---------- */

static void handle_delete(AppState *app)
{
    int id, idx, yes;

    if (!require_records(app)) return;
    if (!prompt_int("ID of transaction to delete: ", 1, 2000000000, &id)) return;
    idx = search_by_id(&app->store, id);
    if (idx < 0) {
        printf("Transaction %d not found.\n", id);
        return;
    }
    transaction_print_header();
    transaction_print(&app->store.items[idx]);
    if (!prompt_yes_no("Delete this transaction? (y/n): ", &yes)) return;
    if (!yes) {
        printf("Cancelled.\n");
        return;
    }
    store_delete(&app->store, id);
    app->dirty = 1;
    printf("Transaction %d deleted.\n", id);
}

/* ---------- 5. process (function pointers) ---------- */

static void handle_process(AppState *app)
{
    int choice;
    const OperationEntry *entry;
    SalesOperation op;         /* function pointer chosen at runtime */
    OpResult r;

    if (!require_records(app)) return;
    printf("\n--- Sales processing operations ---\n");
    for (size_t i = 0; i < OPERATION_COUNT; i++)
        printf("%zu. %s\n", i + 1, OPERATIONS[i].name);
    if (!prompt_int("Choose an operation: ", 1, (int)OPERATION_COUNT, &choice)) return;

    entry = &OPERATIONS[choice - 1];
    op = entry->run;                       /* assign the function pointer */
    printf("[function pointer] executing: %s\n", entry->name);
    r = op(&app->store);                   /* call through the pointer */

    if (entry->is_money)
        printf("Result: %.2f\n", r.value);
    else
        printf("Result: %.0f\n", r.value);
    if (r.txn) {
        transaction_print_header();
        transaction_print(r.txn);
    }
}

/* ---------- 6. filter (callbacks) ---------- */

static void handle_filter(AppState *app)
{
    int choice;
    size_t n;

    if (!require_records(app)) return;
    printf("\n--- Filter transactions ---\n");
    printf("1. Total amount above a value\n");
    printf("2. Transactions for a product\n");
    printf("3. Quantity at least a value\n");
    printf("4. Count transactions with total above a value\n");
    if (!prompt_int("Choose: ", 1, 4, &choice)) return;

    if (choice == 1 || choice == 4) {
        double limit;
        if (!prompt_positive_double("Amount threshold: ", MAX_UNIT_PRICE * MAX_QUANTITY, &limit)) return;
        n = txn_count_if(&app->store, pred_total_above, &limit);
        if (choice == 4) {
            printf("[callback] pred_total_above matched %zu transaction(s).\n", n);
        } else if (n == 0) {
            printf("No transactions above %.2f.\n", limit);
        } else {
            printf("[callback] pred_total_above + visit_print_row\n");
            transaction_print_header();
            txn_for_each_match(&app->store, pred_total_above, &limit, visit_print_row);
            printf("(%zu match%s)\n", n, n == 1 ? "" : "es");
        }
    } else if (choice == 2) {
        char product[PRODUCT_NAME_LEN];
        if (!prompt_string("Product name: ", product, sizeof product)) return;
        n = txn_count_if(&app->store, pred_product_equals, product);
        if (n == 0) {
            printf("No transactions found for product \"%s\".\n", product);
        } else {
            printf("[callback] pred_product_equals + visit_print_row\n");
            transaction_print_header();
            txn_for_each_match(&app->store, pred_product_equals, product, visit_print_row);
            printf("(%zu match%s)\n", n, n == 1 ? "" : "es");
        }
    } else {
        int min_qty;
        if (!prompt_int("Minimum quantity: ", 1, MAX_QUANTITY, &min_qty)) return;
        n = txn_count_if(&app->store, pred_quantity_at_least, &min_qty);
        if (n == 0) {
            printf("No transactions with quantity of %d or more.\n", min_qty);
        } else {
            printf("[callback] pred_quantity_at_least + visit_print_row\n");
            transaction_print_header();
            txn_for_each_match(&app->store, pred_quantity_at_least, &min_qty, visit_print_row);
            printf("(%zu match%s)\n", n, n == 1 ? "" : "es");
        }
    }
}

/* ---------- 7. search ---------- */

static void handle_search(AppState *app)
{
    int choice;

    if (!require_records(app)) return;
    printf("\n--- Search transactions ---\n");
    printf("1. By transaction ID\n2. By product name\n3. By transaction amount\n");
    if (!prompt_int("Choose: ", 1, 3, &choice)) return;

    if (choice == 1) {
        int id, idx;
        if (!prompt_int("Transaction ID: ", 1, 2000000000, &id)) return;
        idx = search_by_id(&app->store, id);
        if (idx < 0) {
            printf("No transaction with ID %d.\n", id);
        } else {
            transaction_print_header();
            transaction_print(&app->store.items[idx]);
        }
    } else {
        size_t n = 0;
        int oom = 0;
        size_t *hits;

        if (choice == 2) {
            char name[PRODUCT_NAME_LEN];
            if (!prompt_string("Product name (or part of it): ", name, sizeof name)) return;
            hits = search_by_product(&app->store, name, &n, &oom);
        } else {
            double amount;
            if (!prompt_positive_double("Transaction amount: ", MAX_UNIT_PRICE * MAX_QUANTITY, &amount)) return;
            hits = search_by_amount(&app->store, amount, &n, &oom);
        }
        if (oom)
            printf("Search failed: out of memory.\n");
        else if (n == 0)
            printf("No matching transactions found.\n");
        else
            print_indices(&app->store, hits, n);
        free(hits);
    }
}

/* ---------- 8. sort ---------- */

typedef struct {
    const char *label;
    TxnCompare cmp;
} SortOption;

static void handle_sort(AppState *app)
{
    static const SortOption options[] = {
        {"Transaction ID (ascending)",   cmp_id_asc},
        {"Product name (A-Z)",           cmp_product_asc},
        {"Total amount (ascending)",     cmp_total_asc},
        {"Total amount (descending)",    cmp_total_desc},
    };
    int choice;

    if (!require_records(app)) return;
    printf("\n--- Sort transactions ---\n");
    for (size_t i = 0; i < sizeof options / sizeof options[0]; i++)
        printf("%zu. %s\n", i + 1, options[i].label);
    if (!prompt_int("Choose: ", 1, 4, &choice)) return;

    sort_transactions(&app->store, options[choice - 1].cmp);   /* comparison callback passed in */
    app->dirty = 1;
    printf("[callback] sorted using %s\n", options[choice - 1].label);
    print_table(&app->store);
}

/* ---------- 9. report ---------- */

static void handle_report(AppState *app)
{
    char product[PRODUCT_NAME_LEN];
    size_t matches;
    double revenue;

    report_print_summary(&app->store);
    if (app->store.count == 0) return;
    if (!read_line("\nProduct for revenue breakdown (Enter to skip): ", product, sizeof product)) return;
    trim(product);
    if (product[0] == '\0') return;
    revenue = report_product_revenue(&app->store, product, &matches);
    if (matches == 0)
        printf("No transactions found for product \"%s\".\n", product);
    else
        printf("Revenue for \"%s\": %.2f from %zu transaction(s).\n", product, revenue, matches);
}

/* ---------- 10 / 11. files ---------- */

static void handle_save(AppState *app)
{
    StorageStatus st = storage_save(&app->store, app->path);
    if (st == STORAGE_OK) {
        app->dirty = 0;
        printf("Saved %zu transaction(s) to %s.\n", app->store.count, app->path);
    } else {
        printf("Save failed: %s\n", storage_status_message(st));
    }
}

static void handle_load(AppState *app)
{
    LoadStats stats;
    StorageStatus st;
    int yes;

    if (app->dirty) {
        if (!prompt_yes_no("You have unsaved changes that loading will discard. Continue? (y/n): ", &yes)) return;
        if (!yes) {
            printf("Cancelled.\n");
            return;
        }
    }
    st = storage_load(&app->store, app->path, &stats);
    if (st != STORAGE_OK) {
        printf("Load failed: %s\n", storage_status_message(st));
        return;
    }
    app->dirty = 0;
    printf("Loaded %zu transaction(s) from %s", stats.loaded, app->path);
    if (stats.skipped)
        printf(" (%zu invalid or duplicate line(s) skipped)", stats.skipped);
    printf(".\n");
}

/* ---------- main loop ---------- */

void ui_run(AppState *app)
{
    char line[64];
    int choice;

    for (;;) {
        print_menu();
        if (!read_line("Enter your choice: ", line, sizeof line))
            choice = 0;                                    /* end of input: leave cleanly */
        else if (!parse_int(line, 0, 11, &choice)) {
            printf("Invalid choice. Enter a number from 0 to 11.\n");
            continue;
        }

        switch (choice) {
        case 1:  handle_add(app);     break;
        case 2:
            if (require_records(app)) print_table(&app->store);
            break;
        case 3:  handle_update(app);  break;
        case 4:  handle_delete(app);  break;
        case 5:  handle_process(app); break;
        case 6:  handle_filter(app);  break;
        case 7:  handle_search(app);  break;
        case 8:  handle_sort(app);    break;
        case 9:  handle_report(app);  break;
        case 10: handle_save(app);    break;
        case 11: handle_load(app);    break;
        case 0: {
            int yes = 0;
            if (app->dirty) {
                if (prompt_yes_no("You have unsaved changes. Save before exiting? (y/n): ", &yes) && yes)
                    handle_save(app);
            }
            printf("Goodbye!\n");
            return;
        }
        }
    }
}
