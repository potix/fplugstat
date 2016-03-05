#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

#include "string_util.h"

int
mkdirs(
    const char *dir,
    mode_t mode)
{
    char tmp[MAXPATHLEN];

    if (dir == NULL) {
        errno = EINVAL;
        return 1;
    }

    if (strcmp(dir, "/") == 0 ||
        strcmp(dir, ".") == 0 ||
        strcmp(dir, "..") == 0 ) {
        return 0;
    }
    strlcpy(tmp, dir, sizeof(tmp));
    mkdirs(dirname(tmp), mode);
    mkdir(dir, mode);

    return 0;
}
