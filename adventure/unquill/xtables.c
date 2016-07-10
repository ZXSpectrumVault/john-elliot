/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996-2000, 2013  John Elliott

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "unquill.h"

char *xfindword(uchr w);

static char xwordch(char c)
{
	if (arch != ARCH_C64) return(0xFF - c);
	else                  return((0xFF - c)  & 0x7F);
}

void xlistcond(char *title, ushrt first)
{
	uchr noun, verb;
	ushrt addr=first, subsaddr;

	fprintf(outfile, "  <%s>\n", title);
	while (zmem(addr))
	{
		verb = zmem(addr++);
		noun = zmem(addr++);
		fprintf(outfile, "    <match");
		if (verb != 0xFF) fprintf(outfile, " verb=\"%s\"", xfindword(verb));
		if (noun != 0xFF) fprintf(outfile, " noun=\"%s\"", xfindword(noun));
		fprintf(outfile,">\n      <conditions>\n");
		subsaddr = zword(addr++);
		while (zmem(subsaddr) != 0xFF)
		{
			xopcond(&subsaddr);
		}
		fprintf(outfile,"      </conditions>\n");
		fprintf(outfile,"      <actions>\n");
		subsaddr++;
		while (zmem(subsaddr) != 0xFF)
		{
			xopact(&subsaddr);
		}
		fprintf(outfile, "      </actions>\n");
		fprintf(outfile, "    </match>\n");
		addr++;
	}
	fprintf(outfile, "  </%s>\n", title);
}
static const char *cname[] = {"at", "notat", "atgt", "atlt", 
			 "present", "absent", "worn", "notworn",
			 "carried", "notcarr", "chance", "zero",
			 "notzero", "eq", "gt", "lt"}; 

void xopcond(ushrt *condad)
{
	uchr id=zmem((*condad)++);
	uchr param1 = zmem((*condad)++);

	if (id < 4) fprintf(outfile, "        <%s loc=\"%d\" />\n", 
					cname[id], param1); 
	else if (id < 10) fprintf(outfile, "        <%s loc=\"%d\" />\n", 
				cname[id], param1);
	else if (id == 10) fprintf(outfile, "        <%s percent=\"%d\" />\n", 
				cname[id], param1);
	else if (id < 13) fprintf(outfile, "        <%s flg=\"%d\" />\n", 
				cname[id], param1);
	else fprintf(outfile, "        <%s flg=\"%d\" value=\"%d\" />\n", 
				cname[id], param1, zmem((*condad)++)); 

}

static char *aname0[] = 
{ 
	"inven", "desc", "quit", "end", "done", "ok", "anykey",
	"save", "load", "turns", "score", "pause", "goto", "message", 
	"remove", "get", "drop", "wear", "destroy", "create", "swap", 
	"set", "clear", "plus", "minus", "let", "beep"
};

static char *aname5[] =
{
	"inven", "desc", "quit", "end", "done", "ok", "anykey",
	"save", "load", "turns", "score", "cls", "dropall", "pause",
	"paper", "ink", "border", "goto", "message", "remove", "get",
	"drop", "wear", "destroy", "create", "swap", "place", "set",
	"clear", "plus", "minus", "let", "beep"
};

static char *aname10[] =
{
	"inven", "desc", "quit", "end", "done", "ok", "anykey",
	"save", "load", "turns", "score", "cls", "dropall", "autog",
	"autod", "autow", "autor", "pause", "paper", "ink", "border", 
	"goto", "message", "remove", "get", "drop", "wear", "destroy", 
	"create", "swap", "place", "set", "clear", "plus", "minus", 
	"let", "beep"
};


void xopact(ushrt *actad)
{
	static char inited=0;
	static char params[40];
	static char ptype[40];
	static char *aname[40];
	int n;
	uchr id=zmem((*actad)++);

	if (!inited)
	{
		if (dbver > 0 && dbver < 10)
		{
			memcpy(aname, aname5, sizeof(aname5));
			for (n = 0; n < 39; n++) 
				{
				if (n < 13)      params[n] = 0;
				else if (n < 29) params[n] = 1;
				else             params[n] = 2;

				if (n < 19)      ptype[n] = NIL;
				else if (n < 27) ptype[n] = OBJ;
				else             ptype[n] = FLG;
			}
			params[25] = params[26] = 2;
			ptype[13] = PSE;
			ptype[14] = ptype[15] = ptype[16] = COL;
			ptype[17] = LOC;
			ptype[18] = MSG;
			ptype[25] = SWAP;
			ptype[26] = PLC;
			ptype[32] = BEEP;
		}
		else if (dbver >= 10)
  		{
			memcpy(aname, aname10, sizeof(aname10));
			for(n=0;n<39;n++)
   			{
				if      (n < 17) params[n] = 0;
				else if (n < 33) params[n] = 1;
				else             params[n] = 2;
	
				if      (n < 21) ptype[n] = NIL;
				else if (n < 29) ptype[n] = OBJ;
				else             ptype[n] = FLG;
			}	
			params[29] = params[30] = 2;
			ptype[17] = PSE;
			ptype[18] = ptype[19] = ptype[20] = COL;
			ptype[21] = LOC;
			ptype[22] = MSG;
			ptype[29] = SWAP;
			ptype[30] = PLC;
			ptype[36] = BEEP;
			if (arch == ARCH_CPC) params[19] = 2;	/* On CPC, INK takes 2 parameters */
		}
		else 
		{
			memcpy(aname, aname0, sizeof(aname0));
			for(n = 0; n < 39; n++)
    			{
				if      (n < 11) params[n] = 0;
				else if (n < 23) params[n] = 1;
				else             params[n] = 2;
				if 	(n < 12) ptype[n]=NIL;
				else if (n < 21) ptype[n]=OBJ;
				else             ptype[n]=FLG;
			}
			params[20] = 2;
			ptype[11] = PSE;
			ptype[12] = LOC;
			ptype[13] = MSG;
			ptype[20] = SWAP;
			ptype[26] = BEEP;
		}
		inited = 1;
	}
  fprintf(outfile, "        <%s", aname[id]);
  for (n = 0; n < params[id]; n++)
  {
    switch (ptype[id])
    {
	case FLG: if (n == 0)
		       fprintf(outfile," flg=\"%d\"", zmem((*actad)++)); 
		  else fprintf(outfile," val=\"%d\"", zmem((*actad)++)); 
		  break;
	case SWAP: if (n == 0)
		       fprintf(outfile," obj=\"%d\"", zmem((*actad)++)); 
		  else fprintf(outfile," with=\"%d\"", zmem((*actad)++)); 
		  break;
	case PLC: if (n == 0)
		       fprintf(outfile," obj=\"%d\"", zmem((*actad)++)); 
		  else fprintf(outfile," loc=\"%d\"", zmem((*actad)++)); 
		  break;
	case BEEP: if (n == 0)
		       fprintf(outfile," dur=\"%d\"", zmem((*actad)++)); 
		  else fprintf(outfile," freq=\"%d\"", zmem((*actad)++)); 
		  break;
	case COL: fprintf(outfile," col=\"%d\"", zmem((*actad)++)); break;
	case PSE: fprintf(outfile," dur=\"%d\"", zmem((*actad)++)); break;
	case OBJ: fprintf(outfile," obj=\"%d\"", zmem((*actad)++)); break;
	case MSG: fprintf(outfile," msg=\"%d\"", zmem((*actad)++)); break;
	case LOC: fprintf(outfile," loc=\"%d\"", zmem((*actad)++)); break;
	default: fprintf(outfile," param%d=\"%d\"", n, zmem((*actad)++)); break;
    }
  }
  fprintf(outfile, " />\n");
}



void xmlcat(char *target, char ch)
{
	char buf[2];

	switch(ch)
	{
		case '%':  strcat(target, "&percnt;"); break;
		case '&':  strcat(target, "&amp;");    break;
		case 96 :  strcat(target, "&pound;");  break;
		case 127:  strcat(target, "&copy;");   break;
		case '<':  strcat(target, "&lt;");     break;
		case '>':  strcat(target, "&gt;");     break;
		case '"':  strcat(target, "&quot;");   break;
		case '\'': strcat(target, "&#x27;");   break;

		default:
			buf[0] = ch;
			buf[1] = 0;
			strcat(target, buf);
			break;
	}
}


char *xword(ushrt addr)
{
	static char buf[50];
	short n;

	buf[0] = 0;
	for (n = 0; n < 4; n++)
	{
		xmlcat(buf, xwordch(zmem(addr++))); 
	}
	buf[n] = 0;
	/* Trim trailing spaces */
	for (n = strlen(buf) - 1; n >= 0; n--)
	{
		if (buf[n] == ' ') buf[n] = 0;
		else break;
	}
	return buf;
}

char *xfindword(uchr w)
{
	int n;
	static char buf[10];

	for (n=vocab; zmem(n); n=n+5)
	{
		if (zmem(n+4) == w) return xword(n);
	}
	sprintf(buf, "&#x%04x;", w);
	return buf;	
}


void xlistlocs(ushrt locs, ushrt conns, uchr count)
{
	uchr item = 0;
	ushrt n = 0;

	fprintf(outfile, "  <rooms>\n");
	for (item = 0; item < count; item++)
	{
		fprintf(outfile, "    <room no=\"%d\">\n", item);
		xoneitem(locs, item);	
		n = zword(conns + 2 * item);
		while (zmem(n) != 0xFF)
		{
			fprintf(outfile, "      <conn dir=\"%s\" to=\"%d\" />\n",
				xfindword(zmem(n)),
				zmem(n+1));
			n += 2;				
		}
		fprintf(outfile, "    </room>\n");
	}
	fprintf(outfile, "  </rooms>\n");
}

void xlistwords(ushrt first)
{
	ushrt n;

	fprintf(outfile, "  <vocab>\n");
	for (n=first; zmem(n); n=n+5)
	{
		fprintf(outfile, "    <word no=\"%d\">%s</word>\n", 
				zmem(n+4), xword(n));
	}
	fprintf(outfile, "  </vocab>\n");
}

void xlistobjs(ushrt otab, ushrt postab, ushrt objmap, uchr num)
{
	ushrt n;
	uchr v;

	fprintf(outfile, "  <objects>\n");
	for(n = 0; n < num; n++)
	{
		fprintf(outfile, "    <object no=\"%d\"", n);
		v = zmem(postab+n);
		switch (v)
		{
			case 252: break;	/* Does not exist */
			case 253: fprintf(outfile, " location=\"worn\""); break;
			case 254: fprintf(outfile, " location=\"carried\""); break;
			default: fprintf(outfile, " location=\"%d\"", v);
		}
		fprintf(outfile, ">\n");
		xoneitem(otab, n);
		if (dbver >= 10)
		{
			v = zmem(objmap + n);
			if (v != 0xFF)
				fprintf(outfile, "      <name>%s</name>\n",
					xfindword(v));	
		}
		fprintf(outfile, "    </object>\n");
	}
	fprintf(outfile, "  </objects>\n");
}

void xlistmsgs(char *section, ushrt mtab, uchr num)
{
	ushrt n;
	uchr cch, term;
	ushrt addr;

	if (arch == ARCH_SPECTRUM) term = 0x1F; else term = 0;
	cch = ~term;

	fprintf(outfile, "  <%ss>\n", section);
	for(n = 0; n < num; n++)
	{
		fprintf(outfile, "    <%s no=\"%d\">", section, n);
		addr = zword(mtab + 2*n);
		xpos=0;

		while (1)
		{
		    cch=(0xFF-(zmem(addr++)));
			if (cch == term) break;
		    xexpch(cch,&addr);
		}
		fprintf(outfile, "</%s>\n", section);
	}
	fprintf(outfile, "  </%ss>\n", section);
}

void xlistsys(char *section, ushrt addr, uchr num)
{
	ushrt n;
	uchr cch, term;

	if (arch == ARCH_SPECTRUM) term = 0x1F; else term = 0;
	cch = ~term;

	fprintf(outfile, "  <%ss>\n", section);
	for(n = 0; n < num;)
	{
		fprintf(outfile, "    <%s no=\"%d\">", section, n);
		xpos=0;

		while (1)
		{
		    cch=(0xFF-(zmem(addr++)));
			if (cch == term) break;
		    xexpch(cch,&addr);
		}
		fprintf(outfile, "</%s>\n", section);
		++n;
	}
	fprintf(outfile, "  </%ss>\n", section);
}




void xoneitem(ushrt table, uchr item)
{
	ushrt n;
	uchr  cch;
	uchr  term;

	if (arch == ARCH_SPECTRUM) term = 0x1F; else term = 0;
	cch = ~term;

	fprintf(outfile, "      <text>");	
	n=zword(table+2*item);
	xpos=0;


	while (1)
	{
	    cch=(0xFF-(zmem(n++)));
		if (cch == term) break;
	    xexpch(cch,&n);
	}
	fprintf(outfile, "</text>\n");	
}


void xexpdict(uchr cch, ushrt *n)
{
	ushrt d=dict;

 	if (dbver > 0) /* Early games aren't compressed & have no dictionary */
	{
		cch -= 164;
		while (cch)
       		if (zmem(d++) > 0x7f) cch--;

      /* d=address of expansion text */

		while (zmem(d) < 0x80) xexpch (zmem(d++) & 0x7F,n);
	        xexpch (zmem(d) & 0x7F,n);
	}
}



void xexpch_c64(uchr cch, ushrt *n)
{
	if (cch >= 'A' && cch <= 'Z') cch = tolower(cch);
	cch &= 0x7F;

	switch(cch)
	{	
		case 0x0D:
		fprintf (outfile,"<br />\n");
		xpos = 0;
		break;

		case '"': fprintf (outfile,"&quot;");    ++xpos; break;
		case '\'':fprintf (outfile,"&#x27;");    ++xpos; break;
		case '%': fprintf (outfile,"&percnt;");   ++xpos; break;
		case '<': fprintf (outfile,"&lt;");    ++xpos; break;
		case '>': fprintf (outfile,"&gt;");    ++xpos; break;
		case '&': fprintf (outfile,"&amp;");   ++xpos; break;
		case 96 : fprintf (outfile,"&pound;"); ++xpos; break;
		case 127: fprintf (outfile,"&copy;");  ++xpos; break;

		default:
		if (cch < 32)
			fprintf (outfile,"<paper col=\"%d\" />", cch);
		else if ((cch > 31) && (cch < 128)) fputc(cch,outfile);
		else fprintf(outfile, "&#x%04x;", cch);
		++xpos;
		if (xpos >= 40)
		{
			fprintf(outfile,"\n");
			xpos = 0;
		}
		break;
	}
}



void xexpch_cpc(uchr cch, ushrt *n)
{
	switch(cch)
	{
		case 0x0d:
		case 0x14: 
		          xpos = 0;
			  fprintf (outfile,"<br />"); break;	
		case '"': fprintf (outfile,"&quot;");    ++xpos; break;
		case '\'':fprintf (outfile,"&#x27;");    ++xpos; break;
		case '%': fprintf (outfile,"&percnt;");   ++xpos; break;
		case '<': fprintf (outfile,"&lt;");    ++xpos; break;
		case '>': fprintf (outfile,"&gt;");    ++xpos; break;
		case '&': fprintf (outfile,"&amp;");   ++xpos; break;
		case 9:   fprintf (outfile,"<invert />"); break;

		default:
			if ((cch > 31) && (cch < 127)) fputc(cch,outfile);
		else	fprintf(outfile, "&#x%04x;", cch);
		++xpos;
		if (xpos >= 40)
		{
			fprintf(outfile,"\n");
			xpos = 0;
		}
		break;}
}


static const ushrt zx_entity[] = 
{
	0x00A0, /*	NO-BREAK SPACE */
	0x259D, /*	QUADRANT UPPER RIGHT */
	0x2598, /*	QUADRANT UPPER LEFT */
	0x2580, /*	UPPER HALF BLOCK */
	0x2597, /*	QUADRANT LOWER RIGHT */
	0x2590, /*	RIGHT HALF BLOCK */
	0x259A, /*	QUADRANT UPPER LEFT AND LOWER RIGHT */
	0x259C, /*	QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT */
	0x2596, /*	QUADRANT LOWER LEFT */
	0x259E, /*	QUADRANT UPPER RIGHT AND LOWER LEFT */
	0x258C, /*	LEFT HALF BLOCK */
	0x259B, /*	QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT */
	0x2584, /*	LOWER HALF BLOCK */
	0x259F, /*	QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT */
	0x2599, /*	QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT */
	0x2588, /*	FULL BLOCK */
};

void xexpch(uchr cch, ushrt *n)
{

	if (arch == ARCH_CPC)
	{
		xexpch_cpc(cch, n);
		return;
	}
	if (arch == ARCH_C64)
	{
		xexpch_c64(cch, n);
		return;
	}
	switch (cch)
	{
		case 0x0D:
		fprintf (outfile,"<br />\n");
		xpos = 0;
		break;

		case 0x17:
		fprintf (outfile,"<tab cols=\"%d\" />",
				((255-zmem((*n)++)) & 0x1F));
		++(*n);
		break;

		case '"': fprintf (outfile,"&quot;");    ++xpos; break;
		case '\'':fprintf (outfile,"&#x27;");    ++xpos; break;
		case '%': fprintf (outfile,"&percnt;");   ++xpos; break;
		case '<': fprintf (outfile,"&lt;");    ++xpos; break;
		case '>': fprintf (outfile,"&gt;");    ++xpos; break;
		case '&': fprintf (outfile,"&amp;");   ++xpos; break;
		case 96 : fprintf (outfile,"&pound;"); ++xpos; break;
		case 127: fprintf (outfile,"&copy;");  ++xpos; break;

		case 16:  
		fprintf (outfile,"<ink col=\"%d\" />",255-zmem((*n)++));
		break;

		case 17:  
		fprintf (outfile,"<paper col=\"%d\" />",255-zmem((*n)++));
		break;

	        case 18:
		fprintf (outfile,"<flash val=\"%d\" />",255-zmem((*n)++));
		break;

		case 19:
		fprintf (outfile,"<bright val=\"%d\" />",255-zmem((*n)++));
		break;

		case 20:
		fprintf (outfile,"<inverse val=\"%d\" />",255-zmem((*n)++));
		break;

		case 21:
		fprintf (outfile,"<over val=\"%d\" />",255-zmem((*n)++));
		break;

		case 6:
		/* Two htabs in succession are a line break */
		if (xpos < 15 && 255-zmem((*n)) == 6)
		{
			fprintf (outfile,"<br />\n");
			xpos = 0;
			++(*n);
			break;
		}
		if (xpos < 15)
		{
			fprintf (outfile,"<htab />");
			xpos = 16;
			break;
		}
		fprintf (outfile,"<br />\n");
		xpos = 0;
		break;

	        default:
		if ((cch > 31) && (cch < 127)) fputc(cch,outfile);
		/* Block graphics */
		else if (cch > 127 && cch < 144) fprintf(outfile, "&#x%04x;",
					zx_entity[cch-128]);
		/* UDGs go to the private use area */
		else if (cch > 143 && cch < 165) fprintf(outfile, "&#x%04x;",
					cch - 144 + 0xE000);
		else if (cch > 164) xexpdict (cch, n);
		else fprintf(outfile, "&#x%04x;", cch);
		++xpos;
		if (xpos >= 32)
		{
			fprintf(outfile,"\n");
			xpos = 0;
		}
		break;
	}
}

#if 0
uchr mirror(uchr c)
{
	ushrt n;	
	uchr o;

	for (n = 0, o = 0; n < 8; n++)
	{
		o = o << 1;
		o |= (c & 1);
		c = c >> 1;
	}
	return o;
}

#endif

void xfontout(ushrt addr, ushrt nchars, char *title, int id)
{
	ushrt n;
	uchr b, m;

	fprintf(outfile,"<%s set=\"%d\">\n", title, id);

	for (n = 0; n < nchars * 8; n++)
	{
		b = zmem(addr + n);
		for (m = 0; m < 8; m++)
		{
			if (b & 0x80) fputc('#', outfile);
			else	      fputc('-', outfile);
			b = b << 1;	
		}
		fputc('\n', outfile);
	}
	fprintf(outfile,"</%s>\n", title);
}


void xlistudgs(ushrt addr)
{
	fprintf(outfile, "  <udgs>\n");
	xfontout(addr, 21, "udg", 0);
	fprintf(outfile, "  </udgs>\n");
}

void xlistfont(ushrt addr)
{
	ushrt a, b, some, index, std_done;

	std_done = 0;
	some = index = 0;
/* [new in v0.9.0] */
/* Before doing a standard font dump, try searching the game code for writes
 * to the "font" system variable. This should bring out all fonts used by the
 * game engine, whether or not they are currently selected. */

/* This byte range is my guess at the area occupied by the Quill engine. */	
	fprintf(outfile, "  <fonts>\n");
	for (a = 0x5D00; a < 0x6C00; a++)
	{
/* This checks for a LD HL, immediately followed by a LD (5C36h),HL */
		if (zmem(a  ) == 0x21 && zmem(a+3) == 0x22 && 
		    zmem(a+4) == 0x36 && zmem(a+5) == 0x5C)
		{
			b = zword(a+1);
			++index;
			if (b < 0x5b00) 
			{
				fprintf(outfile, "  <!-- Font %d is the system font -->\n", index); 						
				continue;
			}
			++some;
			xfontout(b + 256, 96, "font", index);
			if (b == addr) std_done = 1;
		}
	}
/* If the search above didn't find the currently selected font, dump that. */
	if (!std_done)
	{
		if (addr >= 0x5B00) xfontout(addr+256, 96, "font", ++index);
		else fprintf(outfile, "  <!-- Font %d is the system font -->\n",
			 index); 						
	}	
	fprintf(outfile, "  </fonts>\n");
}


static const char *p1_moves[] = {"x=\"1\" y=\"0\"",
				 "x=\"1\" y=\"1\"",
				 "x=\"0\" y=\"1\"",
				 "x=\"-1\" y=\"1\"",
				 "x=\"-1\" y=\"0\"",
				 "x=\"-1\" y=\"-1\"",
				 "x=\"0\" y=\"-1\"",
				 "x=\"1\" y=\"-1\"" };

static const char *anshade[] = {"x", "y", "fill"};
static const char *angosub[] = {"no" };
static const char *anblock[] = {"h", "w", "x", "y" };

/* 2 is a lousy sample size, but all the Spectrum Quill games I've examined 
 * have their graphics tables at 0xFAB8. */

void xlistgfx(void)
{
	ushrt n, m;
	ushrt gfx_base  = 0xFAB8;	/* Arbitrary */
	ushrt gfx_ptrs  = zword(gfx_base);
	ushrt gfx_flags = zword(gfx_base + 2);
	ushrt gfx_count = zmem (gfx_base + 6);
	uchr gflag, nargs = 0, value;
	ushrt gptr;
	char inv, ovr;
	char opcode[30];
	ushrt neg[8];
	const char **argnames;

	fprintf(outfile, "  <graphics>\n");
	for (n = 0; n < gfx_count; n++)
	{
		fprintf(outfile, "    <graphic no=\"%d\"", n);

		gflag = zmem (gfx_flags + n);
		gptr  = zword(gfx_ptrs  + 2*n);

		if (gflag & 0x80) fprintf(outfile, " type=\"location\"");
		else		  fprintf(outfile, " type=\"sub\"");
		fprintf(outfile, " ink=\"%d\" paper=\"%d\" bit6=\"%d\">\n",
			(gflag & 7), (gflag >> 3) & 7, (gflag >> 6) & 1);

		do
		{
			memset(neg, 0, sizeof(neg));
			gflag = zmem(gptr);
			inv = ' '; ovr = ' ';
			if (gflag & 0x08) ovr = 'o';
			if (gflag & 0x10) inv = 'i';
			value = (gflag >> 3);
			opcode[0] = 0;
			argnames = anblock;
			switch(gflag & 7)
			{
				case 0:  nargs = 2;  
					 argnames = anshade;
                                         sprintf(opcode, "<plot");
					 if (ovr == 'o') 
						strcat(opcode, " over=\"1\"");
					 if (inv == 'i') 
						strcat(opcode, " inverse=\"1\"");
					 if (ovr == 'o' && inv == 'i')
						strcpy(opcode, "<amove");
					 break;
				case 1:  nargs = 2; 
					 argnames = anshade;
                                         if (gflag & 0x40) neg[0] = 1;
                                         if (gflag & 0x80) neg[1] = 1;

                                         sprintf(opcode, "<line");
					 if (ovr == 'o') 
						strcat(opcode, " over=\"1\"");
					 if (inv == 'i') 
						strcat(opcode, " inverse=\"1\"");
					 if (ovr == 'o' && inv == 'i')
						strcpy(opcode, "<move");	
					 break;
				case 2:  switch(gflag & 0x30)
					{
						case 0x00:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 2; 
							strcpy(opcode, "<fill");
							argnames = anshade;
							break; 
						case 0x10:
							nargs = 4; 
							strcpy(opcode, "<block");
							argnames = anblock;
							break;
						case 0x20:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 3; 
							argnames = anshade;
							strcpy(opcode, "<shade");
							break;
						case 0x30:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 3; 
							argnames = anshade;
							strcpy(opcode, "<bshade");
							break;
						}
					 break;
				case 3:  nargs = 1; 
					 sprintf(opcode, "<gosub scale=\"%d\"",
						value & 7);
					 argnames = angosub;
					 break;
				case 4:  nargs = 0; 
					 sprintf(opcode, "<rplot");
					 if (ovr == 'o') 
						strcat(opcode, " over=\"1\"");
					 if (inv == 'i') 
						strcat(opcode, " inverse=\"1\"");
					 strcat(opcode, p1_moves[(value / 4)]);
					 break;
				case 5:  nargs = 0; 
					 if (gflag & 0x80) sprintf(opcode,
					  "<bright val=\"%d\"", value & 15);
					 else sprintf(opcode,
					  "<paper val=\"%d\"", value & 15);
					 break;
                                case 6:  nargs = 0; 
					 if (gflag & 0x80) sprintf(opcode,
					  "<flash val=\"%d\"", value & 15);
					 else sprintf(opcode,
					  "<ink val=\"%d\"", value & 15);
					 break;
				case 7:  nargs = 0; 
					 strcpy(opcode, "<end"); break;
			}
			fprintf(outfile, "      %s", opcode); 
			for (m = 0; m < nargs; m++)
			{
				fprintf(outfile, " %s=\"%s%d\"",
					argnames[m], 
					neg[m] ? "-" : "",
					zmem(gptr + 1 + m));
			}
			fprintf(outfile, " />\n");

			gptr += (nargs + 1);

		}
		while((gflag & 7) != 7);
		fprintf(outfile, "    </graphic>\n");
	}
	fprintf(outfile, "  </graphics>\n");
}

