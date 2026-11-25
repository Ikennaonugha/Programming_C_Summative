#include "student.h"
#include <errno.h>
#include <math.h>

/* Discard the rest of the current input line. */
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Remove leading and trailing whitespace in place. */
void trim_whitespace(char *s) {
    size_t start = 0;
    while (s[start] != '\0' && isspace((unsigned char)s[start]))
        start++;
    if (start > 0)
        memmove(s, s + start, strlen(s + start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
}

/*
 * Read one line into buf (at most size-1 characters, newline removed).
 * Over-long input is truncated and the remainder is discarded, so a buffer
 * can never overflow. Returns 1 on success, 0 on end-of-input.
 */
int read_line(const char *prompt, char *buf, size_t size) {
    if (prompt != NULL) {
        printf("%s", prompt);
        fflush(stdout);
    }
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    else if (len == size - 1)
        clear_input_buffer();
    return 1;
}

/* Prompt until the user enters a whole number in [min, max]. 0 on EOF. */
int read_int(const char *prompt, int min, int max, int *out) {
    char buf[64];
    for (;;) {
        if (!read_line(prompt, buf, sizeof buf))
            return 0;
        trim_whitespace(buf);

        char *end;
        errno = 0;
        long v = strtol(buf, &end, 10);
        if (buf[0] == '\0' || *end != '\0' || errno == ERANGE || v < min || v > max) {
            printf("  Invalid input. Enter a whole number between %d and %d.\n", min, max);
            continue;
        }
        *out = (int)v;
        return 1;
    }
}

/* Prompt until the user enters a finite number in [min, max]. 0 on EOF. */
int read_float(const char *prompt, float min, float max, float *out) {
    char buf[64];
    for (;;) {
        if (!read_line(prompt, buf, sizeof buf))
            return 0;
        trim_whitespace(buf);

        char *end;
        errno = 0;
        double v = strtod(buf, &end);
        if (buf[0] == '\0' || *end != '\0' || errno == ERANGE ||
            !isfinite(v) || v < min || v > max) {
            printf("  Invalid input. Enter a number between %.2f and %.2f.\n", min, max);
            continue;
        }
        *out = (float)v;
        return 1;
    }
}

/* Prompt until the user enters a non-blank string. 0 on EOF. */
int read_nonempty(const char *prompt, char *buf, size_t size) {
    for (;;) {
        if (!read_line(prompt, buf, size))
            return 0;
        trim_whitespace(buf);
        if (buf[0] != '\0')
            return 1;
        printf("  Input cannot be empty.\n");
    }
}

/* A valid name has letters, spaces, hyphens or apostrophes only. */
static int is_valid_name(const char *s) {
    int has_letter = 0;
    for (; *s != '\0'; s++) {
        if (isalpha((unsigned char)*s))
            has_letter = 1;
        else if (*s != ' ' && *s != '-' && *s != '\'')
            return 0;
    }
    return has_letter;
}

/* Prompt until the user enters a valid personal name. 0 on EOF. */
int read_name(const char *prompt, char *buf, size_t size) {
    for (;;) {
        if (!read_nonempty(prompt, buf, size))
            return 0;
        if (is_valid_name(buf))
            return 1;
        printf("  Names may only contain letters, spaces, hyphens and apostrophes.\n");
    }
}

/* Ask a yes/no question.*/
int confirm(const char *prompt) {
    char buf[8];
    for (;;) {
        printf("%s (y/n): ", prompt);
        if (!read_line(NULL, buf, sizeof buf))
            return 0;
        trim_whitespace(buf);
        if (buf[0] == 'y' || buf[0] == 'Y')
            return 1;
        if (buf[0] == 'n' || buf[0] == 'N')
            return 0;
        printf("  Please answer y or n.\n");
    }
}
