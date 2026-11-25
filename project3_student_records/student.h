#ifndef STUDENT_H
#define STUDENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 10   /* starting number of student slots */
#define CAPACITY_STEP     5   /* slots added/removed on grow/shrink */
#define MAX_STR          50   /* max length of names, programme, etc. */
#define MAX_CODE         12   /* max length of a course code */
#define MIN_STUDENT_ID    1
#define MAX_STUDENT_ID    999999
#define MIN_YEAR          1
#define MAX_YEAR          6
#define COURSE_CAPACITY_STEP 4     /* course slots added per realloc */
#define MAX_COURSE_MARKS  1000.0f  /* upper limit for a course's max marks */
#define PASS_PERCENT      50.0f    /* pass = at least 50% of max marks */
#define DEFAULT_DATA_FILE "students.txt"

/* One course result belonging to a student. */
typedef struct {
    char  code[MAX_CODE];
    char  name[MAX_STR];
    float marks;       /* marks obtained */
    float max_marks;   /* maximum marks (> 0) */
} Course;

/* A student owns a dynamically allocated array of Course records. */
typedef struct {
    int    id;                    /* unique identifier */
    char   first_name[MAX_STR];
    char   last_name[MAX_STR];
    char   programme[MAX_STR];
    int    year;                  /* year of study */
    int    course_count;          /* number of courses registered */
    int    course_capacity;       /* allocated slots in courses[] */
    Course *courses;              /* heap array, NULL when empty */
} Student;

/* Growable array of pointers to heap-allocated students. */
typedef struct {
    Student **data;
    size_t    size;
    size_t    capacity;
} StudentRegistry;

/* Shared global registry (defined in operations.c) */
extern StudentRegistry registry;

typedef enum { LOAD_OK, LOAD_MISSING, LOAD_ERROR } LoadStatus;

/* Function pointer type used by the menu jump table */
typedef void (*OperationFunc)(void);

/* validate.c */
void clear_input_buffer(void);
void trim_whitespace(char *s);
int  read_line(const char *prompt, char *buf, size_t size);
int  read_int(const char *prompt, int min, int max, int *out);
int  read_float(const char *prompt, float min, float max, float *out);
int  read_nonempty(const char *prompt, char *buf, size_t size);
int  read_name(const char *prompt, char *buf, size_t size);
int  confirm(const char *prompt);

/* operations.c */
void     print_header(void);
int      init_registry(void);
void     free_student(Student *s);
void     free_registry(void);
Student *find_student_by_id(int id, size_t *index_out);
void     display_student_details(const Student *s);

void register_student(void);
void display_students(void);
void view_student(void);
void update_student(void);
void delete_student(void);
void exit_program(void);

/* courses.c */
float    course_percentage(const Course *c);
int      course_passed(const Course *c);
int      calculate_student_average(const Student *s, float *out_percent);
Course  *find_course(Student *s, const char *code);

void add_course(void);
void update_marks(void);
void display_student_performance(void);

/* search_sort.c */
int  str_casecmp(const char *a, const char *b);
void search_students(void);
void sort_students(void);

/* reports.c */
void generate_report(void);
void export_report(void);

/* file_io.c */
void        set_data_file(const char *path);
const char *get_data_file(void);
LoadStatus  load_records_file(void);
void        save_records(void);
void        reload_records(void);

#endif /* STUDENT_H */
