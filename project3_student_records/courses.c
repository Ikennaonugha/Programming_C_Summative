#include "student.h"

/* Percentage score of one course (0 if max_marks is invalid). */
float course_percentage(const Course *c) {
    if (c->max_marks <= 0.0f)
        return 0.0f;
    return (c->marks / c->max_marks) * 100.0f;
}

/* A course is passed when marks are at least PASS_PERCENT of max marks. */
int course_passed(const Course *c) {
    return c->marks * 100.0f >= PASS_PERCENT * c->max_marks;
}

/* Student average % = sum(marks) / sum(max marks) * 100. Returns 0 if there are no courses. */
int calculate_student_average(const Student *s, float *out_percent) {
    if (s->course_count == 0)
        return 0;

    double total_marks = 0.0, total_max = 0.0;
    for (const Course *c = s->courses; c < s->courses + s->course_count; c++) {
        total_marks += c->marks;
        total_max += c->max_marks;
    }
    if (total_max <= 0.0)
        return 0;

    *out_percent = (float)(total_marks / total_max * 100.0);
    return 1;
}

/* Find a course by (upper-case) code. Returns NULL if absent. */
Course *find_course(Student *s, const char *code) {
    for (Course *c = s->courses; c < s->courses + s->course_count; c++) {
        if (strcmp(c->code, code) == 0)
            return c;
    }
    return NULL;
}

/* Ask for a student ID and look the student up. NULL if invalid/missing. */
static Student *prompt_for_student(void) {
    int id;
    if (!read_int("Enter Student ID: ", MIN_STUDENT_ID, MAX_STUDENT_ID, &id))
        return NULL;

    Student *s = find_student_by_id(id, NULL);
    if (s == NULL)
        printf("Error: no student with ID %d exists.\n", id);
    return s;
}

/* Read a course code: letters and digits only, shorter than MAX_CODE. */
static int read_course_code(const char *prompt, char *out) {
    char buf[64];
    for (;;) {
        if (!read_nonempty(prompt, buf, sizeof buf))
            return 0;

        int ok = strlen(buf) < MAX_CODE;
        for (const char *p = buf; ok && *p != '\0'; p++) {
            if (!isalnum((unsigned char)*p))
                ok = 0;
        }
        if (!ok) {
            printf("  Course codes use letters and digits only (max %d characters).\n",
                   MAX_CODE - 1);
            continue;
        }

        for (char *p = buf; *p != '\0'; p++)
            *p = (char)toupper((unsigned char)*p);
        strcpy(out, buf); /* safe: length checked above */
        return 1;
    }
}

/* Make room for one more course, growing the array with realloc. */
static int ensure_course_capacity(Student *s) {
    if (s->course_count < s->course_capacity)
        return 1;

    int new_capacity = s->course_capacity + COURSE_CAPACITY_STEP;
    Course *tmp = realloc(s->courses, (size_t)new_capacity * sizeof(Course));
    if (tmp == NULL) {
        fprintf(stderr, "Error: memory reallocation failed for courses.\n");
        return 0;
    }
    s->courses = tmp;
    s->course_capacity = new_capacity;
    return 1;
}

void add_course(void) {
    print_header();
    printf("\n--- Add Course to Student ---\n");

    Student *s = prompt_for_student();
    if (s == NULL)
        return;

    Course c;
    memset(&c, 0, sizeof c);

    if (!read_course_code("Course code (e.g. CS101): ", c.code))
        return;
    if (find_course(s, c.code) != NULL) {
        printf("Error: %s %s is already registered for course %s.\n",
               s->first_name, s->last_name, c.code);
        return;
    }

    if (!read_nonempty("Course name: ", c.name, sizeof c.name) ||
        !read_float("Maximum marks: ", 0.01f, MAX_COURSE_MARKS, &c.max_marks) ||
        !read_float("Marks obtained: ", 0.0f, c.max_marks, &c.marks))
        return;

    if (!ensure_course_capacity(s))
        return;

    s->courses[s->course_count++] = c;
    printf("Success: course %s added for %s %s (%d course(s) registered).\n",
           c.code, s->first_name, s->last_name, s->course_count);
}

void update_marks(void) {
    print_header();
    printf("\n--- Update Course Marks ---\n");

    Student *s = prompt_for_student();
    if (s == NULL)
        return;

    if (s->course_count == 0) {
        printf("%s %s has no registered courses.\n", s->first_name, s->last_name);
        return;
    }

    printf("\nCourses for %s %s:\n", s->first_name, s->last_name);
    for (const Course *p = s->courses; p < s->courses + s->course_count; p++)
        printf("  %-10s %-24.24s %7.2f / %.2f\n", p->code, p->name, p->marks, p->max_marks);
    printf("\n");

    char code[MAX_CODE];
    if (!read_course_code("Course code to update: ", code))
        return;

    Course *c = find_course(s, code);
    if (c == NULL) {
        printf("Error: %s is not registered for course %s.\n", s->last_name, code);
        return;
    }

    float new_marks;
    printf("Current marks: %.2f / %.2f\n", c->marks, c->max_marks);
    if (!read_float("New marks: ", 0.0f, c->max_marks, &new_marks))
        return;

    printf("Marks for %s updated: %.2f -> %.2f\n", c->code, c->marks, new_marks);
    c->marks = new_marks;
}

void display_student_performance(void) {
    print_header();
    printf("\n--- Student Academic Performance ---\n");

    Student *s = prompt_for_student();
    if (s == NULL)
        return;

    display_student_details(s);

    float average;
    if (!calculate_student_average(s, &average)) {
        printf("\n  Overall average: not available (no courses registered).\n");
        return;
    }

    int passed = 0;
    for (const Course *c = s->courses; c < s->courses + s->course_count; c++)
        passed += course_passed(c);

    printf("\n  Overall average : %.2f%%\n", average);
    printf("  Courses passed  : %d of %d\n", passed, s->course_count);
    printf("  Courses failed  : %d of %d\n", s->course_count - passed, s->course_count);
}
