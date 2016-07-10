
#include "Joyce.hxx"
#include "keyboard.hxx"
#include <assert.h>

////////////////////////////////////////////////////////////////////////
// The dummy functions come in two categories...
//
// Stubs for calls that haven't yet been implemented
// Calls that don't do anything in the Unix version

// Stubs...

#define false 0

////////////////////////////////////////////////////////////////////////
// Raw screen access
//
#ifndef WIN32
void cputs(char *s) { fputs(s, stderr); }
#endif
char getch(void) { return getchar(); }

