/*
 * document_analyzer.c
 * Multi threaded document search using POSIX threads.
 * Usage: ./document_analyzer "keyword" file1.txt file2.txt ...
 *
 * One thread handles one file and works only on its own ThreadArgs,
 * so no locks are needed.
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_LEN    512
#define ERR_MSG_LEN 256

typedef enum {
    STATUS_PENDING = 0,
    STATUS_SUCCESS,
    STATUS_FAILED
} ThreadStatus;

/* Inputs and results for one thread */
typedef struct {
    int         thread_id;               /* starts at 1 */
    const char *input_file;
    const char *keyword;
    char        report_file[PATH_LEN];

    long         total_lines;
    long         total_occurrences;
    long         matching_lines;
    long        *line_numbers;           /* grows as matches are found */
    size_t       line_capacity;
    ThreadStatus status;
    char         error_msg[ERR_MSG_LEN]; /* first error only */

    pthread_t thread;
    int       created;                   /* 1 if pthread_create worked */
} ThreadArgs;

static void  set_error(ThreadArgs *args, const char *fmt, ...);
static int   build_report_name(const char *input, int id, int add_id,
                               char *out, size_t size);
static int   initialize_thread_args(ThreadArgs *args, int id,
                                    const char *file, const char *keyword);
static void  resolve_duplicate_report_names(ThreadArgs *args, int count);
static long  count_occurrences(const char *line, size_t len,
                               const char *keyword, size_t kw_len);
static int   record_match_line(ThreadArgs *args, long line_no);
static int   search_file(ThreadArgs *args);
static int   generate_report(ThreadArgs *args);
static void *process_file(void *arg);
static void  display_results(const ThreadArgs *args, int count);
static void  free_thread_args(ThreadArgs *args, int count);

/* Mark the thread as failed and keep the first error message */
static void set_error(ThreadArgs *args, const char *fmt, ...)
{
    va_list ap;

    args->status = STATUS_FAILED;
    if (args->error_msg[0] != '\0')
        return;

    va_start(ap, fmt);
    vsnprintf(args->error_msg, ERR_MSG_LEN, fmt, ap);
    va_end(ap);
}

/*
 * Turn "docs/document1.txt" into "document1_report.txt".
 * With add_id set, the thread id is added to keep the name unique.
 * Returns 0 on success, or -1 if the name does not fit.
 */
static int build_report_name(const char *input, int id, int add_id,
                             char *out, size_t size)
{
    const char *base = input;
    const char *p;
    size_t      base_len;
    int         written;

    for (p = input; *p != '\0'; p++)
        if (*p == '/' || *p == '\\')
            base = p + 1;

    /* Drop the extension, but keep names that start with a dot */
    base_len = strlen(base);
    for (p = base + base_len; p > base; p--) {
        if (*(p - 1) == '.') {
            if ((p - 1) != base)
                base_len = (size_t)((p - 1) - base);
            break;
        }
    }
    if (base_len == 0)
        base_len = strlen(base);

    if (add_id)
        written = snprintf(out, size, "%.*s_%d_report.txt",
                           (int)base_len, base, id);
    else
        written = snprintf(out, size, "%.*s_report.txt",
                           (int)base_len, base);

    return (written < 0 || (size_t)written >= size) ? -1 : 0;
}

/* Set up one ThreadArgs. Returns 0 on success, or -1 on error */
static int initialize_thread_args(ThreadArgs *args, int id,
                                  const char *file, const char *keyword)
{
    memset(args, 0, sizeof(*args));
    args->thread_id  = id;
    args->input_file = file;
    args->keyword    = keyword;
    args->status     = STATUS_PENDING;

    if (build_report_name(file, id, 0, args->report_file, PATH_LEN) != 0) {
        set_error(args, "File name too long to build a report name");
        return -1;
    }
    return 0;
}

/* Give a later input a unique report name if two inputs would share one */
static void resolve_duplicate_report_names(ThreadArgs *args, int count)
{
    int i, j;

    for (i = 1; i < count; i++) {
        if (args[i].status == STATUS_FAILED)
            continue;
        for (j = 0; j < i; j++) {
            if (args[j].status != STATUS_FAILED &&
                strcmp(args[i].report_file, args[j].report_file) == 0) {
                if (build_report_name(args[i].input_file, args[i].thread_id, 1,
                                      args[i].report_file, PATH_LEN) != 0)
                    set_error(&args[i], "File name too long to build a report name");
                break;
            }
        }
    }
}

/* Count matches in one line, ignoring case. Matches do not overlap */
static long count_occurrences(const char *line, size_t len,
                              const char *keyword, size_t kw_len)
{
    long   count = 0;
    size_t i = 0;
    size_t k;

    if (kw_len == 0 || len < kw_len)
        return 0;

    while (i + kw_len <= len) {
        k = 0;
        while (k < kw_len &&
               tolower((unsigned char)line[i + k]) ==
               tolower((unsigned char)keyword[k]))
            k++;

        if (k == kw_len) {
            count++;
            i += kw_len;
        } else {
            i++;
        }
    }
    return count;
}

/* Store a matching line number. Returns 0 on success, or -1 if out of memory */
static int record_match_line(ThreadArgs *args, long line_no)
{
    if ((size_t)args->matching_lines == args->line_capacity) {
        size_t new_cap = (args->line_capacity == 0) ? 16 : args->line_capacity * 2;
        long  *tmp     = realloc(args->line_numbers, new_cap * sizeof(long));
        if (tmp == NULL)
            return -1;
        args->line_numbers  = tmp;
        args->line_capacity = new_cap;
    }

    args->line_numbers[args->matching_lines] = line_no;
    return 0;
}

/* Read the file line by line and collect the statistics */
static int search_file(ThreadArgs *args)
{
    FILE   *fp;
    char   *line = NULL;
    size_t  cap = 0;
    ssize_t n;
    size_t  kw_len = strlen(args->keyword);
    long    hits;
    int     saved_errno;

    fp = fopen(args->input_file, "r");
    if (fp == NULL) {
        set_error(args, "Cannot open input file: %s", strerror(errno));
        return -1;
    }

    while ((n = getline(&line, &cap, fp)) != -1) {
        args->total_lines++;

        hits = count_occurrences(line, (size_t)n, args->keyword, kw_len);
        if (hits > 0) {
            if (record_match_line(args, args->total_lines) != 0) {
                set_error(args, "Out of memory while recording line numbers");
                free(line);
                fclose(fp);
                return -1;
            }
            args->matching_lines++;
            args->total_occurrences += hits;
        }
    }

    /* getline returns -1 at end of file and on error, so check which */
    saved_errno = errno;
    if (ferror(fp)) {
        set_error(args, "Error reading input file: %s", strerror(saved_errno));
        free(line);
        fclose(fp);
        return -1;
    }

    free(line);
    fclose(fp);
    return 0;
}

/* Write this thread's report, also when the analysis failed */
static int generate_report(ThreadArgs *args)
{
    FILE *fp = fopen(args->report_file, "w");
    long  i;
    int   ok = 1;

    if (fp == NULL) {
        set_error(args, "Cannot create report file '%s': %s",
                  args->report_file, strerror(errno));
        return -1;
    }

    ok &= fprintf(fp, "===== DOCUMENT ANALYSIS REPORT =====\n\n") >= 0;
    ok &= fprintf(fp, "Input File          : %s\n", args->input_file) >= 0;
    ok &= fprintf(fp, "Search Keyword      : %s\n", args->keyword) >= 0;
    ok &= fprintf(fp, "Total Lines         : %ld\n", args->total_lines) >= 0;
    ok &= fprintf(fp, "Total Occurrences   : %ld\n", args->total_occurrences) >= 0;
    ok &= fprintf(fp, "Matching Lines      : %ld\n", args->matching_lines) >= 0;

    ok &= fprintf(fp, "Matching Line Nos.  : ") >= 0;
    if (args->matching_lines == 0) {
        ok &= fprintf(fp, "None (keyword not found)") >= 0;
    } else {
        for (i = 0; i < args->matching_lines; i++)
            ok &= fprintf(fp, "%s%ld", (i == 0) ? "" : ", ",
                          args->line_numbers[i]) >= 0;
    }
    ok &= fprintf(fp, "\n") >= 0;

    if (args->status == STATUS_FAILED)
        ok &= fprintf(fp, "Processing Status   : FAILED: %s\n", args->error_msg) >= 0;
    else
        ok &= fprintf(fp, "Processing Status   : SUCCESS\n") >= 0;

    /* fclose flushes buffered data, so check it too */
    if (fclose(fp) != 0)
        ok = 0;

    if (!ok) {
        set_error(args, "Error writing report file '%s'", args->report_file);
        return -1;
    }
    return 0;
}

/* Thread entry point: analyze one file and write its report */
static void *process_file(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;

    printf("[Thread %d] Processing %s ...\n", args->thread_id, args->input_file);

    if (search_file(args) == 0)
        args->status = STATUS_SUCCESS;

    if (generate_report(args) != 0 && args->status != STATUS_FAILED)
        args->status = STATUS_FAILED;

    if (args->status == STATUS_SUCCESS)
        printf("[Thread %d] Successfully processed %s\n",
               args->thread_id, args->input_file);
    else
        printf("[Thread %d] ERROR processing %s: %s\n",
               args->thread_id, args->input_file, args->error_msg);

    return NULL;
}

/* Print the summary once all threads have finished */
static void display_results(const ThreadArgs *args, int count)
{
    int i;
    int failures = 0;

    printf("\n===== PROCESSING SUMMARY =====\n\n");

    for (i = 0; i < count; i++) {
        if (args[i].status == STATUS_SUCCESS) {
            printf("%s: Completed\n", args[i].input_file);
            printf("    Lines: %ld | Occurrences: %ld | Matching lines: %ld | Report: %s\n",
                   args[i].total_lines, args[i].total_occurrences,
                   args[i].matching_lines, args[i].report_file);
        } else {
            printf("%s: Failed (%s)\n", args[i].input_file, args[i].error_msg);
            failures++;
        }
    }

    printf("\n");
    if (failures == 0) {
        printf("All processing tasks completed.\n");
        printf("Reports generated successfully.\n");
    } else {
        printf("Processing finished with %d error(s) out of %d file(s).\n",
               failures, count);
    }
}

static void free_thread_args(ThreadArgs *args, int count)
{
    int i;
    for (i = 0; i < count; i++)
        free(args[i].line_numbers);
}

int main(int argc, char *argv[])
{
    const char *keyword;
    int         file_count;
    int         i, rc;
    int         failures = 0;
    ThreadArgs *args;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s \"keyword\" file1 [file2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    keyword = argv[1];
    if (keyword[0] == '\0') {
        fprintf(stderr, "Error: the search keyword must not be empty.\n");
        return EXIT_FAILURE;
    }

    file_count = argc - 2;
    args = calloc((size_t)file_count, sizeof(ThreadArgs));
    if (args == NULL) {
        fprintf(stderr, "Error: out of memory.\n");
        return EXIT_FAILURE;
    }

    printf("===== MULTI-THREADED DOCUMENT ANALYZER =====\n\n");
    printf("Search Keyword: %s\n", keyword);
    printf("Number of Input Files: %d\n\n", file_count);

    for (i = 0; i < file_count; i++)
        initialize_thread_args(&args[i], i + 1, argv[i + 2], keyword);
    resolve_duplicate_report_names(args, file_count);

    /* Start one thread per file */
    for (i = 0; i < file_count; i++) {
        if (args[i].status == STATUS_FAILED) {
            printf("[Thread %d] ERROR for %s: %s\n", args[i].thread_id,
                   args[i].input_file, args[i].error_msg);
            continue;
        }

        printf("[Main] Creating thread %d for %s\n",
               args[i].thread_id, args[i].input_file);

        rc = pthread_create(&args[i].thread, NULL, process_file, &args[i]);
        if (rc != 0) {
            set_error(&args[i], "Thread creation failed: %s", strerror(rc));
            printf("[Thread %d] ERROR for %s: %s\n", args[i].thread_id,
                   args[i].input_file, args[i].error_msg);
        } else {
            args[i].created = 1;
        }
    }

    /* Join every thread that was created */
    for (i = 0; i < file_count; i++) {
        if (!args[i].created)
            continue;
        rc = pthread_join(args[i].thread, NULL);
        if (rc != 0) {
            set_error(&args[i], "pthread_join failed: %s", strerror(rc));
            printf("[Thread %d] ERROR joining thread: %s\n",
                   args[i].thread_id, args[i].error_msg);
        }
    }

    display_results(args, file_count);

    for (i = 0; i < file_count; i++)
        if (args[i].status != STATUS_SUCCESS)
            failures++;

    free_thread_args(args, file_count);
    free(args);

    return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
