#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SdlContext.hxx"
#include "SdlTerm.hxx"
#include "SdlVt52.hxx"

SDL_Surface *gl_surface;

static void spfx(SdlVt52 &term)
{
	for (Uint16 n = 0; n < 16; n++)
	{
		if (n & 1) term << "\ep"; else term << "\eq";
                if (n & 2) term << "\er"; else term << "\eu";

		term << n << " ";
	}
	term << "\r\n";
}

static void keyboard(SdlVt52 &term)
{
	unsigned char c;

	term << "\erPress keys to see their ASCII codes, ESC to finish\eu\r\n";
	do
	{
		c = term.read();
		term << (unsigned char)c << " " << (int)c << "\r\n";

	}
	while (c != 27);
}

static void rect(SdlVt52 &term)
{
	term.fillRect(100,100,200,50, term.getForeground());
	term.rect(100,200,200,50, term.getForeground());
}


static void testVt(SdlVt52 &term)
{
	int redraw = 1;
	do
	{
		if (redraw) term << "\erTesting menu\eu\r\n\n"
		                 << "0. Exit\r\n"
		                 << "1. Special effects\r\n"
				 << "2. Keyboard\r\n"
				 << "3. Rectangle\r\n"
				 << "P. Page mode on\r\n"
                                 << "Q. Page mode off\r\n"
		                 << "X. Clear screen\r\n";

		switch(term.read())
		{
			case '0': return;
			case 'x':
			case 'X': term << "\eE\eH"; redraw = 1; break;
			case '1': spfx(term); break;
			case '2': keyboard(term); break;
			case '3': rect(term); break;
			case 'p':
			case 'P': term << "\e41Page mode on \r\n"; break;
			case 'Q':
			case 'q': term << "\e40Page mode off\r\n"; break;

		}
	}	
	while (1);
}


int main(int argc, char **argv)
{
        if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
                fprintf(stderr,
                        "Couldn't initialize SDL: %s\n", SDL_GetError());
                exit(1);
        }
        atexit(SDL_Quit);
	
        gl_surface = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
	SdlVt52 term(gl_surface);

	SDL_Colour clr;
	clr.r = 0x80;
	clr.g = 0xC0;
	clr.b = 0xFF;

	term.setForeground(clr);
	testVt(term);
}


