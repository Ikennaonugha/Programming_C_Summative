#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transaction.h"
#include "utils.h"

void trim(char *s)
{
    size_t len = strlen(s), start = 0;

    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    while (s[start] && isspace((unsigned char)s[start]))
        start++;
    if (start > 0)
        memmove(s, s + start, strlen(s + start) + 1);
}

int read_line(const char *prompt, char *buf, size_t size)
{
    size_t len;

    if (prompt) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        int c;                                   /* input was longer than the buffer: discard rest */
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    if (len > 0 && buf[len - 1] == '\r')
        buf[len - 1] = '\0';
    return 1;
}

int parse_int(const char *s, int min, int max, int *out)
{
    char *end;
    long v;

    errno = 0;
    v = strtol(s, &end, 10);
    if (end == s || errno == ERANGE)
        return 0;
    while (isspace((unsigned char)*end))
        end++;
    if (*end != '\0' || v < min || v > max)
        return 0;
    *out = (int)v;
    return 1;
}

int parse_positive_double(const char *s, double max, double *out)
{
    char *end;
    double v;

    errno = 0;
    v = strtod(s, &end);
    if (end == s || errno == ERANGE || !isfinite(v))
        return 0;
    while (isspace((unsigned char)*end))
        end++;
    if (*end != '\0' || v <= 0.0 || v > max)
        return 0;
    *out = v;
    return 1;
}

int prompt_int(const char *prompt, int min, int max, int *out)
{
    char buf[64];

    for (;;) {
        if (!read_line(prompt, buf, sizeof buf))
            return 0;
        if (parse_int(buf, min, max, out))
            return 1;
        printf("  Invalid input. Enter a whole number from %d to %d.\n", min, max);
    }
}

int prompt_positive_double(const char *prompt, double max, double *out)
{
    char buf[64];

    for (;;) {
        if (!read_line(prompt, buf, sizeof buf))
            return 0;
        if (parse_positive_double(buf, max, out))
            return 1;
        printf("  Invalid amount. Enter a number greater than 0 (max %.0f).\n", max);
    }
}

int prompt_string(const char *prompt, char *buf, size_t size)
{
    for (;;) {
        if (!read_line(prompt, buf, size))
            return 0;
        trim(buf);
        if (buf[0] == '\0')
            printf("  This field cannot be empty.\n");
        else if (strchr(buf, '|'))
            printf("  The '|' character is not allowed.\n");
        else
            return 1;
    }
}

int prompt_date(const char *prompt, char *buf, size_t size)
{
    for (;;) {
        if (!read_line(prompt, buf, size))
            return 0;
        trim(buf);
        if (is_valid_date(buf))
            return 1;
        printf("  Invalid date. Use YYYY-MM-DD (for example 2026-11-25).\n");
    }
}

int prompt_yes_no(const char *prompt, int *out_yes)
{
    char buf[16];

    for (;;) {
        if (!read_line(prompt, buf, sizeof buf))
            return 0;
        trim(buf);
        if (buf[0] == 'y' || buf[0] == 'Y') { *out_yes = 1; return 1; }
        if (buf[0] == 'n' || buf[0] == 'N') { *out_yes = 0; return 1; }
        printf("  Please answer y or n.\n");
    }
}

int str_compare_ci(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb)
            return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int str_equal_ci(const char *a, const char *b)
{
    return str_compare_ci(a, b) == 0;
}

int str_contains_ci(const char *hay, const char *needle)
{
    size_t n = strlen(needle);

    if (n == 0)
        return 1;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < n && hay[i] &&
               tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == n)
            return 1;
    }
    return 0;
}
