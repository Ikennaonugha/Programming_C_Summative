#include "student.h"

/* Compare two strings ignoring case. */
int str_casecmp(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* 1 if needle occurs anywhere inside haystack, ignoring case. */
static int contains_ignore_case(const char *haystack, const char *needle) {
    if (*needle == '\0')
        return 1;

    for (; *haystack != '\0'; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*n != '\0' && *h != '\0' &&
               tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (*n == '\0')
            return 1;
    }
    return 0;
}

static void print_result_header(void) {
    printf("-----------------------------------------------------------------------\n");
    printf(" %-6s | %-20s | %-16s | %-4s | %-8s\n", "ID", "Name", "Programme", "Year", "Average");
    printf("-----------------------------------------------------------------------\n");
}

static void print_result_row(const Student *s) {
    char full_name[2 * MAX_STR + 2];
    snprintf(full_name, sizeof full_name, "%s %s", s->first_name, s->last_name);

    char avg_text[16];
    float avg;
    if (calculate_student_average(s, &avg))
        snprintf(avg_text, sizeof avg_text, "%.2f%%", avg);
    else
        snprintf(avg_text, sizeof avg_text, "N/A");

    printf(" %-6d | %-20.20s | %-16.16s | %-4d | %-8s\n",
           s->id, full_name, s->programme, s->year, avg_text);
}

/* Search by exact student ID. Returns the number of matches (0 or 1). */
static int search_by_id(int id) {
    int found = 0;
    for (Student **p = registry.data; p < registry.data + registry.size; p++) {
        if ((*p)->id == id) {
            if (!found)
                print_result_header();
            print_result_row(*p);
            found++;
            break; /* IDs are unique */
        }
    }
    return found;
}

/* Search first or last name (partial, case-insensitive). */
static int search_by_name(const char *query) {
    int found = 0;
    for (Student **p = registry.data; p < registry.data + registry.size; p++) {
        const Student *s = *p;
        if (contains_ignore_case(s->first_name, query) ||
            contains_ignore_case(s->last_name, query)) {
            if (!found)
                print_result_header();
            print_result_row(s);
            found++;
        }
    }
    return found;
}

/* Search by programme of study (partial, case-insensitive). */
static int search_by_programme(const char *query) {
    int found = 0;
    for (Student **p = registry.data; p < registry.data + registry.size; p++) {
        if (contains_ignore_case((*p)->programme, query)) {
            if (!found)
                print_result_header();
            print_result_row(*p);
            found++;
        }
    }
    return found;
}

void search_students(void) {
    print_header();
    printf("\n--- Search Students ---\n");

    if (registry.size == 0) {
        printf("No students registered yet.\n");
        return;
    }

    printf("[1] Student ID\n[2] First or last name\n[3] Programme of study\n[0] Cancel\n");
    int choice;
    if (!read_int("Search by: ", 0, 3, &choice) || choice == 0)
        return;

    int found = 0;
    if (choice == 1) {
        int id;
        if (!read_int("Student ID: ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
            return;
        printf("\n");
        found = search_by_id(id);
    } else {
        char query[MAX_STR];
        if (!read_nonempty(choice == 2 ? "Name (or part of it): " : "Programme (or part of it): ",
                           query, sizeof query))
            return;
        printf("\n");
        found = (choice == 2) ? search_by_name(query) : search_by_programme(query);
    }

    if (found == 0)
        printf("No matching students found.\n");
    else {
        printf("-----------------------------------------------------------------------\n");
        printf("%d match(es) found.\n", found);
    }
}

typedef int (*StudentComparator)(const Student *a, const Student *b);

static int compare_by_id(const Student *a, const Student *b) {
    return (a->id > b->id) - (a->id < b->id);
}

/* Alphabetical by last name, then first name, ignoring case. */
static int compare_by_name(const Student *a, const Student *b) {
    int r = str_casecmp(a->last_name, b->last_name);
    if (r != 0)
        return r;
    return str_casecmp(a->first_name, b->first_name);
}

/* Students with no courses have no average; they rank below everyone. */
static float average_or_floor(const Student *s) {
    float avg;
    return calculate_student_average(s, &avg) ? avg : -1.0f;
}

static int compare_by_average(const Student *a, const Student *b) {
    float x = average_or_floor(a);
    float y = average_or_floor(b);
    return (x > y) - (x < y);
}

/* By programme, then by name so equal programmes are listed neatly. */
static int compare_by_programme(const Student *a, const Student *b) {
    int r = str_casecmp(a->programme, b->programme);
    if (r != 0)
        return r;
    return compare_by_name(a, b);
}

/* Sort an array of student pointers in place with insertion sort. */
static void insertion_sort(Student **arr, size_t n, StudentComparator cmp, int descending) {
    for (size_t i = 1; i < n; i++) {
        Student *key = arr[i];
        size_t j = i;

        while (j > 0) {
            int r = cmp(arr[j - 1], key);
            if (descending)
                r = -r;
            if (r <= 0) /* already in the right order */
                break;
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

typedef struct {
    const char       *label;
    StudentComparator compare;
} SortStrategy;

void sort_students(void) {
    print_header();
    printf("\n--- Sort Student Records ---\n");

    if (registry.size < 2) {
        printf("Need at least two students to sort.\n");
        return;
    }

    const SortStrategy strategies[] = {
        {"Student ID",                  compare_by_id},
        {"Name (alphabetical)",         compare_by_name},
        {"Overall academic average",    compare_by_average},
        {"Programme of study",          compare_by_programme}
    };
    const int count = (int)(sizeof strategies / sizeof strategies[0]);

    for (int i = 0; i < count; i++)
        printf("[%d] %s\n", i + 1, strategies[i].label);
    printf("[0] Cancel\n");

    int choice;
    if (!read_int("Sort by: ", 0, count, &choice) || choice == 0)
        return;

    int direction;
    printf("[1] Ascending (A-Z, low to high)\n[2] Descending (Z-A, high to low)\n");
    if (!read_int("Order: ", 1, 2, &direction))
        return;

    insertion_sort(registry.data, registry.size, strategies[choice - 1].compare, direction == 2);

    printf("Students sorted by %s (%s).\n", strategies[choice - 1].label,
           direction == 1 ? "ascending" : "descending");
    display_students();
}
