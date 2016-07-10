

#ifndef LIBROOT
# ifdef ALIBROOT
#  define Q2(x)   Q1(x)
#  define Q1(x)    #x
#  define LIBROOT   Q2(ALIBROOT)
# else 
#  define LIBROOT "/usr/local/lib/joyce"
# endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define cprintf printf
void cputs(char *s);
char getch(void);

#include "PcwSdlGfx.hxx"

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

// Filenames for floppy drives
#define A_DRIVE "/dev/fd0"
#define B_DRIVE "/dev/fd1"

#define UNIX 1
