#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <math.h>

#include "common_macros.h"
#include "string_util.h"

#ifdef USE_BSD_STRLCPY
// ----------------------------------------------------------------------//
/*	$OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

#endif

#ifdef USE_BSD_STRLCAT
// ----------------------------------------------------------------------//
/*	$OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#endif

int
string_lstrip_b(
    char **new_str,
    char *str,
    const char *strip_str)
{
	int len, last;
	char *find;

	if (new_str == NULL ||
	    str == NULL ||
	    strip_str == NULL) {
		return 1;
	}
	last = strlen(str);
	len = 0;
	while(len < last && str[len] != '\0') {
		find = strchr(strip_str, str[len]);
		if (!find) {
			break;
		}
		str[len] = '\0';
		len++;
	}
	*new_str = &str[len];

	return 0;
}

int
string_rstrip_b(
    char *str,
    const char *strip_str)
{
	int len;
	char *find;

	if (str == NULL || strip_str == NULL) {
		return 1;
	}
	len = strlen(str);
	while(len > 0  && str[len - 1] != '\0') {
		find = strchr(strip_str, str[len - 1]);
		if (!find) {
			break;
		}
		str[len - 1] = '\0';
		len--;
	}

	return 0;
}

int
string_kv_split_b(
    kv_split_t *kv,
    char *str,
    const char *delim_str)
{
	char *key;
	char *value;

	if (kv == NULL ||
	    str == NULL ||
	    delim_str == NULL) {
		return 1;
	}
	if (string_rstrip_b(str, " \t\r")) {
		return 1;
	}
	if ((key = strsep(&str, delim_str)) == NULL) {
		return 1;
	}
	if (string_rstrip_b(key, " \t\r")) {
		return 1;
	}
	if (string_lstrip_b(&value, str, " \t\r")) {
		return 1;
	}
	if (string_rstrip_b(value, " \t\r")) {
		return 1;
	}
        kv->key = key;
        kv->value = value;

        return 0;
}

int
parse_cmd_b(
    parse_cmd_t *parse_cmd,
    char *cmd)
{
	int squote = 0;
	int dquote = 0;
	int cmd_size;
	char *ptr;

        if (parse_cmd == NULL ||
            cmd == NULL) {
                errno = EINVAL;
                return 1;
        }
	ptr = parse_cmd->argv[0] = cmd;
	parse_cmd->argv[1] = NULL;
	parse_cmd->argc = 1;
	cmd_size = strlen(cmd) + 1;
	while (*ptr != '\0') {
		if (!(squote || dquote) && *ptr == ' ') {
			*ptr = '\0';
			if (*(ptr + 1) != '\0') {
				if (parse_cmd->argv[parse_cmd->argc - 1][0] == '\0') {
					parse_cmd->argc--;
				}
				parse_cmd->argv[parse_cmd->argc] = ptr + 1;
				parse_cmd->argv[parse_cmd->argc + 1] = NULL;
				parse_cmd->argc++;
				if (parse_cmd->argc >= NCARGS) {
					errno = ENOBUFS;
					return 1;
				}
			}
		} else if (!squote && *ptr == '"') {
			if (dquote == 1) {
				*ptr = '\0';
				dquote = 0;
			} else {
				parse_cmd->argv[parse_cmd->argc - 1]++;
				dquote = 1;
			}
		} else if (!dquote && *ptr == '\'') {
			if (squote == 1) {
				*ptr = '\0';
				squote = 0;
			} else {
				parse_cmd->argv[parse_cmd->argc - 1]++;
				squote = 1;
			}
		} else if (*ptr == '\\' && (*(ptr + 1) == ' ' || *(ptr + 1) == '"' || *(ptr + 1) == '\'')) {
			memmove(ptr, ptr + 1, (cmd + cmd_size) - (ptr + 1));
		}
		ptr++; 
	}

	return 0;
}


#ifndef LLONG_MAX
#define LLONG_MAX    LONG_MAX
#endif

#ifndef LLONG_MIN
#define LLONG_MIN    LONG_MIN
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX   ULONG_MAX
#endif

int
string_to_ui8(uint8_t *value, const char *str)
{
        unsigned long ul;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        ul = strtoul(str, &ptr, 0);
        if (*ptr !='\0' ||
            (ul == 0 && errno == EINVAL) ||
            (ul == ULONG_MAX && errno == ERANGE)) {
                *value = 0xff;
                return 1;
        }
        if (ul > UINT8_MAX) {
                errno = ERANGE;
                *value = 0xff;
                return 1;
        }
        *value = (uint8_t)ul;

        return 0;
}

int
string_to_ui16(uint16_t *value, const char *str)
{
        unsigned long ul;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        ul = strtoul(str, &ptr, 0);
        if (*ptr !='\0' ||
            (ul == 0 && errno == EINVAL) ||
            (ul == ULONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (ul > UINT16_MAX) {
                errno = ERANGE;
                return 1;
        }
        *value = (uint16_t)ul;

        return 0;
}

int
string_to_ui32(uint32_t *value, const char *str)
{
        unsigned long ul;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        ul = strtoul(str, &ptr, 0);
        if (*ptr !='\0' ||
            (ul == 0 && errno == EINVAL) ||
            (ul == ULONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (ul > UINT32_MAX) {
                errno = ERANGE;
                return 1;
        }
        *value = (uint32_t)ul;

        return 0;
}

int
string_to_ui64(uint64_t *value, const char *str)
{
        unsigned long long ull;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        ull = strtoull(str, &ptr, 0);
        if (*ptr !='\0' ||
            (ull == 0 && errno == EINVAL) ||
            (ull == ULLONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (ull > UINT64_MAX) {
                errno = ERANGE;
                return 1;
        }
        *value = (uint64_t)ull;

        return 0;
}

int
string_to_i8(int8_t *value, const char *str)
{
        long l;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        l = strtol(str, &ptr, 0);
        if (*ptr !='\0' ||
            (l == 0 && errno == EINVAL) ||
            (l == LONG_MIN && errno == ERANGE) ||
            (l == LONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (l > INT8_MAX || l < INT8_MIN) {
                errno = ERANGE;
                return 1;
        }
        *value = (int8_t)l;

        return 0;
}

int
string_to_i16(int16_t *value, const char *str)
{
        long l;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        l = strtol(str, &ptr, 0);
        if (*ptr !='\0' ||
            (l == 0 && errno == EINVAL) ||
            (l == LONG_MIN && errno == ERANGE) ||
            (l == LONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (l > INT16_MAX || l < INT16_MIN) {
                errno = ERANGE;
                return 1;
        }
        *value = (int16_t)l;

        return 0;
}

int
string_to_i32(int32_t *value, const char *str)
{
        long l;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        l = strtol(str, &ptr, 0);
        if (*ptr !='\0' ||
            (l == 0 && errno == EINVAL) ||
            (l == LONG_MIN && errno == ERANGE) ||
            (l == LONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (l > INT32_MAX || l < INT32_MIN) {
                errno = ERANGE;
                return 1;
        }
        *value = (int32_t)l;

        return 0;
}

int
string_to_i64(int64_t *value, const char *str)
{
        long long ll;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        ll = strtoll(str, &ptr, 0);
        if (*ptr !='\0' ||
            (ll == 0 && errno == EINVAL) ||
            (ll == LLONG_MIN && errno == ERANGE) ||
            (ll == LLONG_MAX && errno == ERANGE)) {
                return 1;
        }
        if (ll > INT64_MAX || ll < INT64_MIN) {
                errno = ERANGE;
                return 1;
        }
        *value = (int64_t)ll;

        return 0;
}

int
string_to_f(float *value, const char *str)
{
        double d;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        d = strtod(str, &ptr);
        if (*ptr !='\0' ||
            ((d == HUGE_VAL && errno == ERANGE) ||
             (d == -HUGE_VAL && errno == ERANGE))) {
                return 1;
        }
        *value = (float)d;

        return 0;
}

int
string_to_d(double *value, const char *str)
{
        double d;
        char *ptr;

        if (value == NULL ||
            str == NULL ||
            *str == '\0') {
                errno = EINVAL;
                return 1;
        }
        errno = 0;
        d = strtod(str, &ptr);
        if (*ptr !='\0' ||
            ((d == HUGE_VAL && errno == ERANGE) ||
             (d == -HUGE_VAL && errno == ERANGE))) {
                return 1;
        }
        *value = d;

        return 0;
}






