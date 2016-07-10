
#include <windows.h>
#include <limits.h>
#include "PcwSdlGfx.hxx"

#ifdef __MSVC__
	#undef __inline__		/* Try MSVC's inlines */
	#define __inline__ __inline
	#define strcasecmp stricmp
	#define strncasecmp strnicmp
	__inline void cputs(char *s) {}
	__inline void cprintf(char *s, ...) {}
#else
	inline void cputs(char *s) {}
	inline void cprintf(char *s, ...) {}
#endif

#ifndef PATH_MAX
	#define PATH_MAX _MAX_PATH
#endif

/* Should cputs() and cprintf() open a console window? */

#define LIBROOT "."

#define A_DRIVE "A:"
#define B_DRIVE "B:"

#ifndef WIN32
#define WIN32 1
#endif


