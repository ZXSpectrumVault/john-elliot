/* The cpmtools config.h, which doesn't use automake, includes the headers
 * it finds. This replicates that situation. */
#include "../config.h"

#define STR(x) #x
#define DISKDEFS STR(ALIBROOT/diskdefs)

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <libdsk.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX _MAX_PATH
#endif


#include <time.h>

