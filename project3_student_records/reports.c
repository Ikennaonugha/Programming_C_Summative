#include "student.h"

/* Statistics for one course code across all students. */
typedef struct {
    char   code[MAX_CODE];
    char   name[MAX_STR];   /* name as first seen */
    int    students;        /* students taking the course */
    int    passed;          /* of those, how many passed it */
    double marks;           /* sum of marks obtained */
    double max_marks;       /* sum of maximum marks */
} CourseStat;

/* Head-count for one programme of study. */
typedef struct {
    char name[MAX_STR];     /* spelling as first seen */
    int  students;
} ProgrammeStat;

/* Find the entry for `code`, appending a new one (growing the array with realloc) if it is not there yet. */
static CourseStat *get_course_stat(CourseStat **arr, size_t *count, size_t *cap,
                                   const Course *c) {
    for (CourseStat *p = *arr; p < *arr + *count; p++) {
        if (strcmp(p->code, c->code) == 0)
            return p;
    }

    if (*count == *cap) {
        size_t new_cap = (*cap == 0) ? 8 : *cap * 2;
        CourseStat *tmp = realloc(*arr, new_cap * sizeof(CourseStat));
        if (tmp == NULL)
            return NULL;
        *arr = tmp;
        *cap = new_cap;
    }

    CourseStat *entry = *arr + *count;
    memset(entry, 0, sizeof *entry);
    snprintf(entry->code, sizeof entry->code, "%s", c->code);
    snprintf(entry->name, sizeof entry->name, "%s", c->name);
    (*count)++;
    return entry;
}

/* Same idea for programmes; names are compared ignoring case. */
static ProgrammeStat *get_programme_stat(ProgrammeStat **arr, size_t *count, size_t *cap,
                                         const char *name) {
    for (ProgrammeStat *p = *arr; p < *arr + *count; p++) {
        if (str_casecmp(p->name, name) == 0)
            return p;
    }

    if (*count == *cap) {
        size_t new_cap = (*cap == 0) ? 4 : *cap * 2;
        ProgrammeStat *tmp = realloc(*arr, new_cap * sizeof(ProgrammeStat));
        if (tmp == NULL)
            return NULL;
        *arr = tmp;
        *cap = new_cap;
    }

    ProgrammeStat *entry = *arr + *count;
    memset(entry, 0, sizeof *entry);
    snprintf(entry->name, sizeof entry->name, "%s", name);
    (*count)++;
    return entry;
}

/* Write the full academic performance report to `out`. */
static int write_report(FILE *out) {
    CourseStat    *course_stats = NULL;
    ProgrammeStat *programme_stats = NULL;
    size_t course_count = 0, course_cap = 0;
    size_t programme_count = 0, programme_cap = 0;

    size_t total_courses = 0;
    double all_marks = 0.0, all_max = 0.0;
    int courses_passed = 0;
    int assessed = 0, passing = 0, failing = 0;

    const Student *best = NULL, *worst = NULL;
    float best_avg = 0.0f, worst_avg = 0.0f;

    for (Student **sp = registry.data; sp < registry.data + registry.size; sp++) {
        const Student *s = *sp;
        total_courses += (size_t)s->course_count;

        ProgrammeStat *ps = get_programme_stat(&programme_stats, &programme_count,
                                               &programme_cap, s->programme);
        if (ps == NULL)
            goto out_of_memory;
        ps->students++;

        for (const Course *c = s->courses; c < s->courses + s->course_count; c++) {
            all_marks += c->marks;
            all_max += c->max_marks;

            int passed = course_passed(c);
            courses_passed += passed;

            CourseStat *cs = get_course_stat(&course_stats, &course_count, &course_cap, c);
            if (cs == NULL)
                goto out_of_memory;
            cs->students++;
            cs->passed += passed;
            cs->marks += c->marks;
            cs->max_marks += c->max_marks;
        }

        float avg;
        if (calculate_student_average(s, &avg)) {
            assessed++;
            if (avg >= PASS_PERCENT)
                passing++;
            else
                failing++;

            if (best == NULL || avg > best_avg) {
                best = s;
                best_avg = avg;
            }
            if (worst == NULL || avg < worst_avg) {
                worst = s;
                worst_avg = avg;
            }
        }
    }

    fprintf(out, "===================================================================\n");
    fprintf(out, " ACADEMIC PERFORMANCE REPORT\n");
    fprintf(out, "===================================================================\n");
    fprintf(out, " Total registered students        : %zu\n", registry.size);
    fprintf(out, " Total courses registered         : %zu\n", total_courses);
    fprintf(out, " Students with course records     : %d\n", assessed);
    fprintf(out, " Students not yet assessed        : %zu (no courses)\n",
            registry.size - (size_t)assessed);

    if (all_max > 0.0)
        fprintf(out, " Overall average performance      : %.2f%%\n", all_marks / all_max * 100.0);
    else
        fprintf(out, " Overall average performance      : N/A (no course records)\n");

    if (best != NULL) {
        fprintf(out, " Highest academic average         : %.2f%%  (%s %s, ID %d)\n",
                best_avg, best->first_name, best->last_name, best->id);
        fprintf(out, " Lowest academic average          : %.2f%%  (%s %s, ID %d)\n",
                worst_avg, worst->first_name, worst->last_name, worst->id);
    } else {
        fprintf(out, " Highest / lowest average         : N/A\n");
    }

    fprintf(out, " Students passing (avg >= %.0f%%)   : %d\n", PASS_PERCENT, passing);
    fprintf(out, " Students failing (avg <  %.0f%%)   : %d\n", PASS_PERCENT, failing);
    fprintf(out, " Course results passed / failed   : %d / %zu\n",
            courses_passed, total_courses - (size_t)courses_passed);

    fprintf(out, "\n-------------------------------------------------------------------\n");
    fprintf(out, " AVERAGE PERFORMANCE PER COURSE\n");
    fprintf(out, "-------------------------------------------------------------------\n");
    if (course_count == 0) {
        fprintf(out, " No course records yet.\n");
    } else {
        fprintf(out, " %-10s | %-24s | %8s | %8s | %6s\n",
                "Code", "Course", "Students", "Average", "Passed");
        fprintf(out, " -----------+--------------------------+----------+----------+-------\n");
        for (const CourseStat *cs = course_stats; cs < course_stats + course_count; cs++) {
            double avg = (cs->max_marks > 0.0) ? cs->marks / cs->max_marks * 100.0 : 0.0;
            fprintf(out, " %-10s | %-24.24s | %8d | %7.2f%% | %d/%d\n",
                    cs->code, cs->name, cs->students, avg, cs->passed, cs->students);
        }
    }

    fprintf(out, "\n-------------------------------------------------------------------\n");
    fprintf(out, " STUDENTS PER PROGRAMME OF STUDY\n");
    fprintf(out, "-------------------------------------------------------------------\n");
    for (const ProgrammeStat *ps = programme_stats; ps < programme_stats + programme_count; ps++)
        fprintf(out, " %-30.30s : %d\n", ps->name, ps->students);
    fprintf(out, "===================================================================\n");

    free(course_stats);
    free(programme_stats);
    return 1;

out_of_memory:
    fprintf(stderr, "Error: out of memory while building the report.\n");
    free(course_stats);
    free(programme_stats);
    return 0;
}

void generate_report(void) {
    print_header();
    if (registry.size == 0) {
        printf("\nNo students registered yet. Nothing to report.\n");
        return;
    }
    printf("\n");
    write_report(stdout);
}

void export_report(void) {
    print_header();
    printf("\n--- Export Report to File ---\n");
    if (registry.size == 0) {
        printf("No students registered yet. Nothing to export.\n");
        return;
    }

    char filename[128];
    if (!read_nonempty("File name (e.g. report.txt): ", filename, sizeof filename))
        return;

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error: cannot open file for writing");
        return;
    }

    int ok = write_report(fp);
    if (fclose(fp) != 0) {
        perror("Error: failed to finish writing the file");
        ok = 0;
    }

    if (ok)
        printf("Report saved to '%s'.\n", filename);
    else
        printf("Report could not be saved.\n");
}
