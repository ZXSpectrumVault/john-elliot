
#ifndef RSTYPES_H_INCLUDED
#define RSTYPES_H_INCLUDED 1

/* Resource type names */
#ifndef RT_CURSOR		/* eg, if you #included <windows.h> from wine */
#define RT_CURSOR        1
#define RT_BITMAP        2
#define RT_ICON          3
#define RT_MENU          4
#define RT_DIALOG        5
#define RT_STRING        6
#define RT_FONTDIR       7
#define RT_FONT          8
#define RT_ACCELERATOR   9
#define RT_RCDATA       10
#define RT_MESSAGELIST  11
#define RT_GROUP_CURSOR 12
/* Were Microsoft superstitious? :-) */
#define RT_GROUP_ICON   14
#endif	/* ndef RT_CURSOR */

/* Defined types */
typedef unsigned char byte;	/* 8 bits */
typedef unsigned short wyde;	/* 16 bits */
typedef unsigned long dwyde;	/* 32 bits */

#endif /* ndef RSTYPES_H_INCLUDED */
