/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
**                  Historical Copyright                                **
**                                                                      **
*************************************************************************/

typedef struct gpoint 
{
	long x;
	long y;
} GPoint;

typedef struct gwsparams
{
	long defaults[9];
	short xymode;
} GWSParams;

typedef struct gwsresult
{
	long data[45];
	GPoint points[6];
} GWSResult;

class VdiDriver;

typedef void (*USERBUT)(VdiDriver *self, unsigned short buttons);
typedef void (*USERMOT)(VdiDriver *self, unsigned long x, unsigned long y);
typedef void (*USERCUR)(VdiDriver *self, unsigned long x, unsigned long y);


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
