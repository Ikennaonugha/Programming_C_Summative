#include "student.h"

/* The one definition of the shared global registry */
StudentRegistry registry = {NULL, 0, 0};

void print_header(void) {
    printf("\n===================================================================\n");
    printf(" Student Academic Records and Performance Analytics System\n");
    printf("===================================================================\n");
}

/* Allocate the initial pointer array. Returns 1 on success, 0 on failure. */
int init_registry(void) {
    registry.data = malloc(INITIAL_CAPACITY * sizeof(Student *));
    if (registry.data == NULL) {
        perror("Critical: initial memory allocation failed");
        return 0;
    }
    registry.size = 0;
    registry.capacity = INITIAL_CAPACITY;
    return 1;
}

/* Free one student and the course array it owns. */
void free_student(Student *s) {
    if (s == NULL)
        return;
    free(s->courses);
    free(s);
}

/* Free every student, then the registry array itself. */
void free_registry(void) {
    for (Student **p = registry.data; p < registry.data + registry.size; p++)
        free_student(*p);
    free(registry.data);
    registry.data = NULL;
    registry.size = 0;
    registry.capacity = 0;
}

/* Grow the registry with realloc when it is full. 1 on success, 0 on failure. */
static int ensure_capacity(void) {
    if (registry.size < registry.capacity)
        return 1;

    size_t new_capacity = registry.capacity + CAPACITY_STEP;
    Student **tmp = realloc(registry.data, new_capacity * sizeof(Student *));
    if (tmp == NULL) {
        fprintf(stderr, "Error: memory reallocation failed.\n");
        return 0;
    }
    registry.data = tmp;
    registry.capacity = new_capacity;
    return 1;
}

/* Shrink the registry with realloc when there is a lot of spare room. */
static void shrink_capacity_if_needed(void) {
    size_t new_capacity = registry.capacity - CAPACITY_STEP;
    if (new_capacity < INITIAL_CAPACITY || registry.size > new_capacity)
        return;

    Student **tmp = realloc(registry.data, new_capacity * sizeof(Student *));
    if (tmp != NULL) { /* if shrinking fails the old block is still valid */
        registry.data = tmp;
        registry.capacity = new_capacity;
    }
}
/*
 * Find a student by ID using pointer traversal.
 * Returns the student, or NULL if not found. If index_out is not NULL it
 * receives the student's position in the registry.
 */
Student *find_student_by_id(int id, size_t *index_out) {
    for (Student **p = registry.data; p < registry.data + registry.size; p++) {
        if ((*p)->id == id) {
            if (index_out != NULL)
                *index_out = (size_t)(p - registry.data);
            return *p;
        }
    }
    return NULL;
}

/* Print one student's personal details and course list. */
void display_student_details(const Student *s) {
    printf("\n  Student ID : %d\n", s->id);
    printf("  Name       : %s %s\n", s->first_name, s->last_name);
    printf("  Programme  : %s\n", s->programme);
    printf("  Year       : %d\n", s->year);
    printf("  Courses    : %d registered\n", s->course_count);

    if (s->course_count == 0) {
        printf("\n  No courses registered yet.\n");
        return;
    }

    printf("\n  %-10s | %-24s | %7s | %7s | %7s\n",
           "Code", "Course", "Marks", "Max", "%");
    printf("  -----------+--------------------------+---------+---------+--------\n");
    for (const Course *c = s->courses; c < s->courses + s->course_count; c++) {
        float pct = (c->max_marks > 0.0f) ? (c->marks / c->max_marks) * 100.0f : 0.0f;
        printf("  %-10s | %-24.24s | %7.2f | %7.2f | %6.1f%%\n",
               c->code, c->name, c->marks, c->max_marks, pct);
    }
}

void display_students(void) {
    print_header();
    printf("\n--- All Registered Students ---\n");

    if (registry.size == 0) {
        printf("No students registered yet.\n");
        return;
    }

    printf("-------------------------------------------------------------------\n");
    printf(" %-6s | %-22s | %-18s | %-4s | %-7s\n", "ID", "Name", "Programme", "Year", "Courses");
    printf("-------------------------------------------------------------------\n");

    for (Student **p = registry.data; p < registry.data + registry.size; p++) {
        const Student *s = *p;
        char full_name[2 * MAX_STR + 2];
        snprintf(full_name, sizeof full_name, "%s %s", s->first_name, s->last_name);
        printf(" %-6d | %-22.22s | %-18.18s | %-4d | %-7d\n",
               s->id, full_name, s->programme, s->year, s->course_count);
    }
    printf("-------------------------------------------------------------------\n");
    printf("Total students: %zu\n", registry.size);
}

void view_student(void) {
    print_header();
    printf("\n--- View Student Details ---\n");

    int id;
    if (!read_int("Enter Student ID: ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
        return;

    Student *s = find_student_by_id(id, NULL);
    if (s == NULL) {
        printf("Error: no student with ID %d exists.\n", id);
        return;
    }
    display_student_details(s);
}

void register_student(void) {
    print_header();
    printf("\n--- Register New Student ---\n");

    int id;
    if (!read_int("Student ID (1-999999): ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
        return;

    if (find_student_by_id(id, NULL) != NULL) {
        printf("Error: Student ID %d already exists. Duplicate IDs are rejected.\n", id);
        return;
    }

    Student *s = calloc(1, sizeof *s); /* zeroed: courses = NULL, counts = 0 */
    if (s == NULL) {
        perror("Memory allocation failed for new student");
        return;
    }
    s->id = id;

    if (!read_name("First name: ", s->first_name, sizeof s->first_name) ||
        !read_name("Last name: ", s->last_name, sizeof s->last_name) ||
        !read_nonempty("Programme of study: ", s->programme, sizeof s->programme) ||
        !read_int("Year of study (1-6): ", MIN_YEAR, MAX_YEAR, &s->year)) {
        free_student(s);
        return;
    }

    if (!ensure_capacity()) {
        free_student(s);
        return;
    }

    registry.data[registry.size++] = s;
    printf("Success: student %d registered. Total students: %zu\n", s->id, registry.size);
}

void update_student(void) {
    print_header();
    printf("\n--- Update Student Information ---\n");

    int id;
    if (!read_int("Enter Student ID: ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
        return;

    Student *s = find_student_by_id(id, NULL);
    if (s == NULL) {
        printf("Error: no student with ID %d exists.\n", id);
        return;
    }

    for (;;) {
        printf("\nEditing: %s %s (ID %d)\n", s->first_name, s->last_name, s->id);
        printf("[1] First name\n[2] Last name\n[3] Programme\n[4] Year of study\n[0] Done\n");

        int choice;
        if (!read_int("Select field to update: ", 0, 4, &choice) || choice == 0)
            return;

        switch (choice) {
            case 1:
                if (read_name("New first name: ", s->first_name, sizeof s->first_name))
                    printf("First name updated.\n");
                break;
            case 2:
                if (read_name("New last name: ", s->last_name, sizeof s->last_name))
                    printf("Last name updated.\n");
                break;
            case 3:
                if (read_nonempty("New programme: ", s->programme, sizeof s->programme))
                    printf("Programme updated.\n");
                break;
            case 4:
                if (read_int("New year of study (1-6): ", MIN_YEAR, MAX_YEAR, &s->year))
                    printf("Year updated.\n");
                break;
        }
    }
}

void delete_student(void) {
    print_header();
    printf("\n--- Delete Student Record ---\n");

    int id;
    if (!read_int("Enter Student ID to delete: ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
        return;

    size_t index;
    Student *s = find_student_by_id(id, &index);
    if (s == NULL) {
        printf("Error: no student with ID %d exists.\n", id);
        return;
    }

    printf("About to delete %s %s (ID %d) and all %d course record(s).\n",
           s->first_name, s->last_name, s->id, s->course_count);
    if (!confirm("Confirm deletion")) {
        printf("Deletion cancelled.\n");
        return;
    }

    free_student(s);

    /* Close the gap in the pointer array. */
    memmove(&registry.data[index], &registry.data[index + 1],
            (registry.size - index - 1) * sizeof(Student *));
    registry.size--;

    shrink_capacity_if_needed();
    printf("Student %d deleted. Total students: %zu\n", id, registry.size);
}

void exit_program(void) {
    printf("\nReleasing all dynamically allocated memory...\n");
    free_registry();
    printf("Memory freed. Goodbye!\n");
    exit(EXIT_SUCCESS);
}
