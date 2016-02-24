#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <sys/param.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#if !defined(strlcpy)
#define USE_BSD_STRLCPY
size_t
strlcpy(
    char *dst,
    const char *src,
    size_t siz);
#endif

#if !defined(strlcat)
#define USE_BSD_STRLCAT
size_t
strlcat(
    char *dst,
    const char *src,
    size_t siz);
#endif

struct kv_split {
    char *key;
    char *value;
};
typedef struct kv_split kv_split_t;

struct parse_cmd {
        char *argv[NCARGS];
        int argc;
};
typedef struct parse_cmd parse_cmd_t;

int string_lstrip_b(
    char **new_str,
    char *str,
    const char *strip_str);

int string_rstrip_b(
    char *str,
    const char *strip_str);

int string_kv_split_b(
    kv_split_t *kv,
    char *str,
    const char *delim_str);

int parse_cmd_b(
    parse_cmd_t *parse_cmd,
    char *cmd);

int string_to_ui8(uint8_t *value, const char *str);

int string_to_ui16(uint16_t *value, const char *str);

int string_to_ui32(uint32_t *value, const char *str);

int string_to_ui64(uint64_t *value, const char *str);

int string_to_i8(int8_t *value, const char *str);

int string_to_i16(int16_t *value, const char *str);

int string_to_i32(int32_t *value, const char *str);

int string_to_i64(int64_t *value, const char *str);

int string_to_f(float *value, const char *str);

int string_to_d(double *value, const char *str);

#endif
