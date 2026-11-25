#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Prints prompt, reads one line (newline stripped, overlong input discarded).
   Returns 1 on success, 0 on end-of-input. */
int read_line(const char *prompt, char *buf, size_t size);

/* Strict parsers: whole string must be a valid number inside the range. Return 1 on success. */
int parse_int(const char *s, int min, int max, int *out);
int parse_positive_double(const char *s, double max, double *out);

/* Looping prompts: repeat until valid input. Return 0 only on end-of-input. */
int prompt_int(const char *prompt, int min, int max, int *out);
int prompt_positive_double(const char *prompt, double max, double *out);
int prompt_string(const char *prompt, char *buf, size_t size);   /* non-empty, no '|' */
int prompt_date(const char *prompt, char *buf, size_t size);
int prompt_yes_no(const char *prompt, int *out_yes);

void trim(char *s);
int  str_equal_ci(const char *a, const char *b);     /* case-insensitive equality */
int  str_contains_ci(const char *hay, const char *needle);  /* case-insensitive substring */
int  str_compare_ci(const char *a, const char *b);   /* case-insensitive ordering */

#endif
