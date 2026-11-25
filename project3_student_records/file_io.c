#include "student.h"
#include <errno.h>
#include <limits.h>
#include <math.h>

#define FILE_MAGIC "STUDENT_RECORDS_V1"
#define LINE_BUF 512
#define MAX_FIELDS 8
#define MAX_COURSES_PER_STUDENT 1000

static char data_path[256] = DEFAULT_DATA_FILE;
static int load_failed = 0;

void set_data_file(const char *path) {
    snprintf(data_path, sizeof data_path, "%s", path);
}

const char *get_data_file(void) {
    return data_path;
}

/* ---------- saving ---------- */

static int write_all(FILE *fp) {
    if (fprintf(fp, "%s|%zu\n", FILE_MAGIC, registry.size) < 0)
        return 0;

    for (Student **sp = registry.data; sp < registry.data + registry.size; sp++) {
        const Student *s = *sp;
        if (fprintf(fp, "%d|%s|%s|%s|%d|%d\n", s->id, s->first_name, s->last_name,
                    s->programme, s->year, s->course_count) < 0)
            return 0;

        for (const Course *c = s->courses; c < s->courses + s->course_count; c++) {
            if (fprintf(fp, "%s|%s|%.6g|%.6g\n", c->code, c->name, c->marks, c->max_marks) < 0)
                return 0;
        }
    }
    return 1;
}

/* Writes to a temporary file first so a failed save never destroys the old data. */
void save_records(void) {
    if (load_failed &&
        !confirm("The existing data file could not be read and will be overwritten. Continue")) {
        printf("Save cancelled.\n");
        return;
    }

    char tmp_path[sizeof data_path + 8];
    snprintf(tmp_path, sizeof tmp_path, "%s.tmp", data_path);

    FILE *fp = fopen(tmp_path, "w");
    if (fp == NULL) {
        perror("Error: cannot open file for writing");
        return;
    }

    int ok = write_all(fp);
    if (fclose(fp) != 0)
        ok = 0;
    if (!ok) {
        fprintf(stderr, "Error: failed while writing '%s'.\n", tmp_path);
        remove(tmp_path);
        return;
    }

    if (rename(tmp_path, data_path) != 0) {
        remove(data_path); /* some platforms refuse to rename over an existing file */
        if (rename(tmp_path, data_path) != 0) {
            perror("Error: cannot replace data file");
            remove(tmp_path);
            return;
        }
    }

    load_failed = 0;
    printf("Saved %zu student(s) to '%s'.\n", registry.size, data_path);
}

/* ---------- loading ---------- */

static int split_fields(char *line, char **fields, int max) {
    int n = 0;
    fields[n++] = line;
    for (char *p = line; *p != '\0'; p++) {
        if (*p == '|') {
            if (n == max)
                return -1;
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    return n;
}

static int parse_int(const char *s, int *out) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return 0;
    *out = (int)v;
    return 1;
}

static int parse_float(const char *s, float *out) {
    char *end;
    errno = 0;
    double v = strtod(s, &end);
    if (*s == '\0' || *end != '\0' || errno == ERANGE || !isfinite(v))
        return 0;
    *out = (float)v;
    return 1;
}

static int copy_field(char *dst, size_t size, const char *src) {
    size_t len = strlen(src);
    if (len == 0 || len >= size)
        return 0;
    memcpy(dst, src, len + 1);
    return 1;
}

/* Next non-blank line. Returns 1 if read, 0 at end of file, -1 if line too long. */
static int next_line(FILE *fp, char *buf, size_t size, int *lineno) {
    while (fgets(buf, (int)size, fp) != NULL) {
        (*lineno)++;
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[--len] = '\0';
        else if (len == size - 1)
            return -1;
        if (len > 0 && buf[len - 1] == '\r')
            buf[--len] = '\0';
        if (len > 0)
            return 1;
    }
    return 0;
}

static void discard(StudentRegistry *r) {
    for (Student **p = r->data; p < r->data + r->size; p++)
        free_student(*p);
    free(r->data);
    r->data = NULL;
    r->size = r->capacity = 0;
}

static int append(StudentRegistry *r, Student *s) {
    if (r->size == r->capacity) {
        size_t new_cap = r->capacity + CAPACITY_STEP;
        Student **tmp = realloc(r->data, new_cap * sizeof(Student *));
        if (tmp == NULL)
            return 0;
        r->data = tmp;
        r->capacity = new_cap;
    }
    r->data[r->size++] = s;
    return 1;
}

static int id_in(const StudentRegistry *r, int id) {
    for (Student **p = r->data; p < r->data + r->size; p++) {
        if ((*p)->id == id)
            return 1;
    }
    return 0;
}

static int valid_code(const char *s) {
    for (; *s != '\0'; s++) {
        if (!isalnum((unsigned char)*s))
            return 0;
    }
    return 1;
}

/* Reads a course line into c. Returns NULL on success or an error message. */
static const char *parse_course(char *line, Course *c, const Student *s) {
    char *f[MAX_FIELDS];
    if (split_fields(line, f, MAX_FIELDS) != 4)
        return "course line must have 4 fields";
    if (!copy_field(c->code, sizeof c->code, f[0]) || !valid_code(c->code))
        return "invalid course code";
    for (char *p = c->code; *p != '\0'; p++)
        *p = (char)toupper((unsigned char)*p);
    if (!copy_field(c->name, sizeof c->name, f[1]))
        return "invalid course name";
    if (!parse_float(f[2], &c->marks) || !parse_float(f[3], &c->max_marks))
        return "marks are not numbers";
    if (c->max_marks <= 0.0f || c->max_marks > MAX_COURSE_MARKS)
        return "maximum marks out of range";
    if (c->marks < 0.0f || c->marks > c->max_marks)
        return "marks outside 0..maximum";
    for (const Course *q = s->courses; q < s->courses + s->course_count; q++) {
        if (strcmp(q->code, c->code) == 0)
            return "duplicate course code for a student";
    }
    return NULL;
}

/* Reads a student line into s (courses array allocated, count still 0). */
static const char *parse_student(char *line, Student *s, const StudentRegistry *r, int *declared) {
    char *f[MAX_FIELDS];
    if (split_fields(line, f, MAX_FIELDS) != 6)
        return "student line must have 6 fields";
    if (!parse_int(f[0], &s->id) || s->id < MIN_STUDENT_ID || s->id > MAX_STUDENT_ID)
        return "invalid student ID";
    if (id_in(r, s->id))
        return "duplicate student ID";
    if (!copy_field(s->first_name, sizeof s->first_name, f[1]) ||
        !copy_field(s->last_name, sizeof s->last_name, f[2]) ||
        !copy_field(s->programme, sizeof s->programme, f[3]))
        return "invalid name or programme";
    if (!parse_int(f[4], &s->year) || s->year < MIN_YEAR || s->year > MAX_YEAR)
        return "invalid year of study";
    if (!parse_int(f[5], declared) || *declared < 0 || *declared > MAX_COURSES_PER_STUDENT)
        return "invalid course count";

    if (*declared > 0) {
        s->courses = malloc((size_t)*declared * sizeof(Course));
        if (s->courses == NULL)
            return "out of memory";
        s->course_capacity = *declared;
    }
    s->course_count = 0;
    return NULL;
}

LoadStatus load_records_file(void) {
    FILE *fp = fopen(data_path, "r");
    if (fp == NULL) {
        if (errno == ENOENT)
            return LOAD_MISSING;
        perror("Error: cannot open data file");
        load_failed = 1;
        return LOAD_ERROR;
    }

    StudentRegistry loaded = {NULL, 0, 0};
    Student *current = NULL;
    const char *error = NULL;
    char line[LINE_BUF];
    char *f[MAX_FIELDS];
    int lineno = 0, declared_courses = 0, status;
    int expected = 0;

    loaded.data = malloc(INITIAL_CAPACITY * sizeof(Student *));
    if (loaded.data == NULL) {
        fclose(fp);
        perror("Error: memory allocation failed");
        return LOAD_ERROR;
    }
    loaded.capacity = INITIAL_CAPACITY;

    status = next_line(fp, line, sizeof line, &lineno);
    if (status != 1 || split_fields(line, f, MAX_FIELDS) != 2 ||
        strcmp(f[0], FILE_MAGIC) != 0 || !parse_int(f[1], &expected) || expected < 0) {
        error = "missing or invalid file header";
        goto fail;
    }

    for (int i = 0; i < expected; i++) {
        status = next_line(fp, line, sizeof line, &lineno);
        if (status != 1) {
            error = (status == 0) ? "unexpected end of file" : "line too long";
            goto fail;
        }

        current = calloc(1, sizeof *current);
        if (current == NULL) {
            error = "out of memory";
            goto fail;
        }

        if ((error = parse_student(line, current, &loaded, &declared_courses)) != NULL)
            goto fail;

        for (int j = 0; j < declared_courses; j++) {
            status = next_line(fp, line, sizeof line, &lineno);
            if (status != 1) {
                error = (status == 0) ? "unexpected end of file" : "line too long";
                goto fail;
            }
            if ((error = parse_course(line, &current->courses[current->course_count], current)) != NULL)
                goto fail;
            current->course_count++;
        }

        if (!append(&loaded, current)) {
            error = "out of memory";
            goto fail;
        }
        current = NULL;
    }

    status = next_line(fp, line, sizeof line, &lineno);
    if (status != 0) {
        error = "unexpected extra data after the last record";
        goto fail;
    }

    fclose(fp);
    free_registry();
    registry = loaded;
    load_failed = 0;
    return LOAD_OK;

fail:
    fprintf(stderr, "Error: '%s' is not a valid data file (line %d): %s.\n",
            data_path, lineno, error);
    free_student(current);
    discard(&loaded);
    fclose(fp);
    load_failed = 1;
    return LOAD_ERROR;
}

void reload_records(void) {
    print_header();
    printf("\n--- Load Records from File ---\n");

    if (!confirm("This replaces everything currently in memory. Continue")) {
        printf("Load cancelled.\n");
        return;
    }

    switch (load_records_file()) {
        case LOAD_OK:
            printf("Loaded %zu student(s) from '%s'.\n", registry.size, data_path);
            break;
        case LOAD_MISSING:
            printf("File '%s' does not exist. Memory left unchanged.\n", data_path);
            break;
        case LOAD_ERROR:
            printf("Load failed. Memory left unchanged.\n");
            break;
    }
}
