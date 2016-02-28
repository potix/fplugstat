#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <time.h>

#if !defined(strptime)
extern char *strptime (const char *__restrict __s,
                       const char *__restrict __fmt, struct tm *__tp)
__THROW;
#endif

#endif
