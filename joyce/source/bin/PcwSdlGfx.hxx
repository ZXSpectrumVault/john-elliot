#include <SDL.h>
#include <SDL_audio.h>

#include "uksdl.hxx"


#ifdef SCRMOD
	SDL_Surface *context  = NULL;
	int mustlock = 0;
#else
	extern SDL_Surface *context;
	extern int mustlock;
#endif	/* SCRMOD */


void GrBoxNC(int x1, int y1, int x2, int y2, int colour);
void GrHLineNC(int x1,int x2,int y, int c);
void GrVLineNC(int x,int y1,int y2, int c);
void GrFilledBoxNC(int x1, int y1, int x2, int y2, int colour);
void GrFilledBox(int x1, int y1, int x2, int y2, int colour);
void GrPlotNC(int x, int y, int colour);
void GrClearScreen(int index);
void GrHLine(int x1,int x2,int y, int c);
void GrVLine(int x,int y1,int y2, int c);
void GrClearClipBox(int colour);
void GrSetClipBox(int x1, int y1, int x2, int y2);
void GrPlot(int x, int y, int colour);
void GrPlotRop(int x1, int y1, int colour, int rop);
void GrLineRop(int x1, int y1, int x2, int y2, int colour, int rop);
void GrQueryColor(int n, int *r, int *g, int *b);
int  GrPixel(int x, int y);

void xor_block(int y, int x, int h, int w);
void SdlScrBlit(int dx, int dy, int sx, int sy, int sx2, int sy2);
void reload_palette(void);
int get_keytoken(void);

/* Switch in/out of fullscreen mode */
int  fullscreen(int enable);
