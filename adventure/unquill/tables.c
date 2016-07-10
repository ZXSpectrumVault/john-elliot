/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996-2000  John Elliott

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

void wordch(char c);

void def_colours(void)
{
	if (arch == ARCH_SPECTRUM)
	{
		(console->ink)    (zmem(ucptr - 12)); 
		(console->paper)  (zmem(ucptr - 10));
		(console->flash)  (zmem(ucptr -  8));
		(console->bright) (zmem(ucptr -  6));
		(console->inverse)(zmem(ucptr -  4));
		(console->over)   (zmem(ucptr -  2));
	}
	if (arch == ARCH_CPC)
	{
		(console->inverse)(0);
	}
}

void listcond(ushrt first)
{
	ushrt addr=first, subsaddr;

	while (zmem(addr))
	{
		fprintf(outfile,"%4x: ", addr);
		opword(zmem(addr++));
		fputc(' ', outfile);
		opword(zmem(addr++));
		fprintf(outfile,"  Conditions:\n");
		subsaddr = zword(addr++);
		while (zmem(subsaddr) != 0xFF)
		{
			fprintf(outfile,"%4x:               ",subsaddr);
			opcond(&subsaddr);
			fputc('\n',outfile);
		}
		fprintf(outfile,"                 Actions:\n");
		subsaddr++;
		while (zmem(subsaddr) != 0xFF)
		{
			fprintf(outfile,"%4x:               ",subsaddr);
			opact(&subsaddr);
			fputc('\n',outfile);
		}
		addr++;
	}
	fputc('\n',outfile);
}

void opcond(ushrt *condad)
{
  uchr id=zmem((*condad)++);
  uchr param1;

  opcact(id, "AT     NOTAT  ATGT   ATLT   PRESENTABSENT WORN   NOTWORN"
             "CARRIEDNOTCARRCHANCE ZERO   NOTZEROEQ     GT     LT     ");

  param1=zmem((*condad)++);
  fprintf(outfile,"%4d ",param1);
  if (id > 12) fprintf(outfile,"%4d ",zmem((*condad)++));

  if (verbose)
      {
      indent=44;
      if (id < 4)   /* Location */
          {
          fprintf (outfile,"            ;");
          oneitem (loctab,param1);
          }
      else if (id < 10) /* Object */
          {
          fprintf (outfile,"            ;");
          oneitem (objtab,param1);
          }
      }
}

void opact(ushrt *actad)
{
	static char inited=0;
	static char params[40];
	static char ptype[40];
	int n;
	uchr id=zmem((*actad)++);
	uchr actpar[2];

	if (!inited)
	{
		if (dbver > 0 && dbver < 10)
		{
			for (n = 0; n < 39; n++) 
				{
				if (n < 13)      params[n] = 0;
				else if (n < 29) params[n] = 1;
				else             params[n] = 2;

				if (n < 19)      ptype[n] = NIL;
				else if (n < 27) ptype[n] = OBJ;
				else             ptype[n] = NIL;
			}
			params[25] = params[26] = 2;
			ptype[17] = LOC;
			ptype[18] = MSG;
			ptype[25] = SWAP;
			ptype[26] = PLC;
		}
	else if (dbver >= 10)
  	{
		 for(n=0;n<39;n++)
   		 {
			if      (n < 17) params[n] = 0;
			else if (n < 33) params[n] = 1;
			else             params[n] = 2;

			if      (n < 21) ptype[n] = NIL;
			else if (n < 29) ptype[n] = OBJ;
			else             ptype[n] = NIL;
		}	
		params[29] = params[30] = 2;
		ptype[21]=LOC;
		ptype[22]=MSG;
		ptype[29]=SWAP;
		ptype[30]=PLC;

		if (arch == ARCH_CPC) params[19] = 2;	/* On CPC, INK takes 2 parameters */
	}
	else 
	{
		for(n = 0; n < 39; n++)
    		{
			if      (n < 11) params[n] = 0;
			else if (n < 23) params[n] = 1;
			else             params[n] = 2;
			if 	(n < 12) ptype[n]=NIL;
			else if (n < 21) ptype[n]=OBJ;
			else             ptype[n]=NIL;
		}
		params[20] = 2;
		ptype[12] = LOC;
		ptype[13] = MSG;
		ptype[20] = SWAP;
	}
	inited = 1;
  }

if (dbver > 0 && dbver < 10)
	   opcact(id,"INVEN  DESC   QUIT   END    DONE   OK     ANYKEY "
                     "SAVE   LOAD   TURNS  SCORE  CLS    DROPALLPAUSE  "
                     "PAPER  INK    BORDER GOTO   MESSAGEREMOVE GET    "
                     "DROP   WEAR   DESTROYCREATE SWAP   PLACE  SET    "
                     "CLEAR  PLUS   MINUS  LET    BEEP   ");


else if (dbver >= 10)
	   opcact(id,"INVEN  DESC   QUIT   END    DONE   OK     ANYKEY "
                     "SAVE   LOAD   TURNS  SCORE  CLS    DROPALLAUTOG  "
                     "AUTOD  AUTOW  AUTOR  PAUSE  PAPER  INK    BORDER "
                     "GOTO   MESSAGEREMOVE GET    DROP   WEAR   DESTROY"
                     "CREATE SWAP   PLACE  SET    CLEAR  PLUS   MINUS  "
                     "LET    BEEP   ");

else       opcact(id,"INVEN  DESC   QUIT   END    DONE   OK     ANYKEY "
                     "SAVE   LOAD   TURNS  SCORE  PAUSE  GOTO   MESSAGE"
                     "REMOVE GET    DROP   WEAR   DESTROYCREATE SWAP   "
                     "SET    CLEAR  PLUS   MINUS  LET    BEEP   ");

for (n=params[id]; n>0;)
  {
  actpar[n-1]=(zmem((*actad)++));
  fprintf(outfile,"%4d ",actpar[--n]);
  }

  if (verbose)
     {
     indent=44;
     switch(ptype[id])
        {
        case PLC:
          fprintf (outfile,"       ;");
          oneitem (objtab,actpar[1]);
          fprintf (outfile,"\n\n                                ");
        /* Fall through to... */
        case LOC:
          fprintf (outfile,"            ;");
          oneitem (loctab,actpar[0]);
          break;
        case SWAP:
          fprintf (outfile,"       ;");
          oneitem (objtab,actpar[0]);
          fprintf (outfile,"\n\n                                ");
          actpar[0]=actpar[1];
        /* Fall through to... */
        case OBJ:
          fprintf (outfile,"            ;");
          oneitem (objtab,actpar[0]);
          break;
        case MSG:
          fprintf (outfile,"            ;");
          oneitem (msgtab,actpar[0]);
          break;
        default:
          ;
        }
     }
}

void opcact(char n, char *str)
{
	fprintf(outfile,"%.7s",str+7*n);
}

void listconn(ushrt first,uchr nrms)
{
	uchr item = 0;
	ushrt n = 0, dest;

	fprintf(outfile,"\n%4x: Connections from   0:",first);
	if (verbose)
	{
		indent = 23;
		fprintf(outfile," ;");
		oneitem(loctab,0);
	}
	fprintf(outfile,"\n      ");
	xpos = 0;
	n = first;
	for(item = 0; item <= (nrms-1);) if (zmem(n)==0xFF)
	{
		fprintf(outfile,"\n%4x: Connections from %3d:",n++,++item);
		if (verbose && item <= (nrms - 1))
		{
			indent=28;
			fprintf(outfile," ;");
			oneitem(loctab,item);
		}
		fprintf(outfile,"\n      ");
	}
	else
	{
		opword(zmem(n++));
		fprintf(outfile," to %3d",dest=zmem(n++));
		if (verbose)
		{
			fprintf(outfile,"           ;");
			oneitem(loctab,dest);
		}
		fprintf(outfile,"\n      ");
	}
}

void listwords(ushrt first)
{
	ushrt n,m;

	for (n=first; zmem(n); n=n+5)
	{
		fprintf (outfile,"\n%4x: Word %3d: ",n,zmem(n+4));
		for (m=0; m<4; m++) wordch(zmem(m+n));
	}
	fputc('\n',outfile);
}

void listpos(ushrt first,uchr n)
{
	uchr m,loc;

	for (m=0; m<n; m++)
  	{
		loc=zmem(first+m);
		fprintf(outfile,"\n%4x: Object %3d is initially ",(first+m),m);
		switch (loc)
    		{
			case 0xFF:
			fprintf(outfile,"Invalid location. ");
			fprintf(stderr,"Warning: Invalid location in "
                                       "object positions table");
			break;

			case 0xFE:
 			fprintf(outfile,"carried.");
 			break;

			case 0xFD:
			fprintf(outfile,"worn.");
			break;

			case 0xFC:
			fprintf(outfile,"not created.");
			break;

			default:
			fprintf(outfile,"in room %3d.",loc);
		} 
		if (verbose)
		{
			opch32('\n');
			indent=44;
			oneitem(objtab,m);
			if (loc<252)
			{
				opch32('\n');
				oneitem(loctab,loc);
			}
		}
	}
	fputc('\n',outfile);
}


void listmap(ushrt first,uchr n)
{
	uchr m,wrd;

	for (m=0; m<n; m++)
	{
		wrd=zmem(first+m);
		fprintf(outfile,"\n%4x: Object %3d named ",(first+m),m);
		opword(wrd);
		if(verbose)
		{	      
			fputs("    ;",outfile);
			indent=31;
			oneitem(objtab,m);
		}
	}
	fputc('\n',outfile);
}


void wordch(char c)
{
	if (arch != ARCH_C64) fputc(0xFF - c, outfile);
	else                  fputc((0xFF - c)  & 0x7F, outfile);
}


void opword(ushrt wid)
{
	ushrt n,m;

	for (n=vocab; zmem(n); n=n+5) if (zmem(n+4) == wid)
	{
		for (m=0; m<4; m++) wordch(zmem(m+n));
		break;
	}	
}




void oneitem(ushrt table, uchr item)
{
	ushrt n;
	uchr  cch;
	uchr  term;

	if (arch == ARCH_SPECTRUM) term = 0x1F; else term = 0;
	cch = ~term;
	
	n=zword(table+2*item);
	xpos=0;

	while (1)
	{
		cch=(0xFF-(zmem(n++)));
		if (cch == term) break;
		expch(cch,&n);
	}
	def_colours();
}



void listitems(char *itmid, ushrt first, ushrt howmany)
{
    uchr item=0,cch;
    ushrt n;
	ushrt term;

	if (arch == ARCH_SPECTRUM) term = 0x1F; else term = 0;
    
    indent=0;
    howmany=howmany & 0xFF;
    fprintf(outfile,"\nThere are %d %ss.\n",howmany,itmid);
    fprintf(outfile,"\n%4x: %s 0:\n      ",first,itmid);
    xpos=0;
    n=first;
    for(item=0;item<howmany;)
      {
      cch=(0xFF-(zmem(n++)));
      if (cch == term)  /* End of text */
          {
          if (++item==howmany) fprintf(outfile,"\n");
          else fprintf (outfile,"\n%4x: %s %d:\n      ",n,itmid,item);
          xpos=0;
          }
      else
        expch(cch, &n);
      }
}





void expdict(uchr cch, ushrt *n)
{
	ushrt d=dict;

 	if (dbver > 0) /* Early games aren't compressed & have no dictionary */
	{
		cch -= 164;
		while (cch)
       		if (zmem(d++) > 0x7f) cch--;

      /* d=address of expansion text */

		while (zmem(d) < 0x80) expch (zmem(d++) & 0x7F,n);
	        expch (zmem(d) & 0x7F,n);
	}
}



void expch_c64(uchr cch, ushrt *n)
{
	if (cch >= 'A' && cch <= 'Z') cch = tolower(cch);
	cch &= 0x7F;
	
	switch (oopt)
	{
		case 'I':		/* Inform */
		switch(cch)
		{
			case '^':  fprintf(outfile, "@@9");  cch = '4'; break;
			case '@':  fprintf(outfile, "@@6");  cch = '4'; break;
			case '~':  fprintf(outfile, "@@12"); cch = '6'; break;
			case '\\': fprintf(outfile, "@@9");  cch = '2'; break;
			case '"':  cch = '~'; break;
			case '`':  fprintf(outfile, "@L");   cch = 'L'; break;
			case 0x0D: cch = '^'; break;

		}
		/* FALL THROUGH */

		case 'T':                 /* Plain text */
		if      ((cch > 31) && (cch <= 127)) opch32(cch);
		else if (cch > 128)  opch32('?');
		else if (cch == 8)   opch32(8);
		else if (cch == 0x0D) opch32('\n');
		break;

		case 'Z':    /* ZXML */
		switch (cch)
		{
			case 0x0D:
			fprintf (outfile,"<BR>");
			xpos = 0;
			break;

			case '%': fprintf (outfile,"%%25;");   ++xpos; break;
			case '<': fprintf (outfile,"&lt;");    ++xpos; break;
			case '>': fprintf (outfile,"&gt;");    ++xpos; break;
			case '&': fprintf (outfile,"&amp;");   ++xpos; break;
			case 96 : fprintf (outfile,"&pound;"); ++xpos; break;
			case 127: fprintf (outfile,"&copy;");  ++xpos; break;
			default:
			if (cch < 32) fprintf(outfile,"<PAPER=%d>", cch);
			else if ((cch > 31) && (cch < 127)) 
			{
				fputc(cch,outfile);
				++xpos;
			}
			else
			{
				fprintf(outfile,"%%%x;",cch);
				++xpos;
			}
		}
		if (oopt == 'z' && xpos > 39) { fprintf(outfile, "<BR>"); xpos = 0; }
		break;
	}
}



void expch_cpc(uchr cch, ushrt *n)
{
	static char reverse = 0;
	
	switch (oopt)
	{
		case 'I':		/* Inform */
		switch(cch)
		{
			case '^':  fprintf(outfile, "@@9");  cch = '4'; break;
			case '@':  fprintf(outfile, "@@6");  cch = '4'; break;
			case '~':  fprintf(outfile, "@@12"); cch = '6'; break;
			case '\\': fprintf(outfile, "@@9");  cch = '2'; break;
			case '"':  cch = '~'; break;
			case '`':  fprintf(outfile, "@L");   cch = 'L'; break;
			case 0x0D: cch = '^'; break;

		}
		/* FALL THROUGH */

		case 'T':                 /* Plain text */
		if      ((cch > 31) && (cch < 127)) opch32(cch);
	/*	else if (cch > 164)  expdict(cch, n); */
		else if (cch > 126)  opch32('?');
		else if (cch == 9)   break;
		else if (cch == 0x0D) opch32('\n');
		else if (cch == 0x14) opch32('\n');
		else if (cch < 31)    opch32('?');
		break;

		case 'Z':	/* ZXML */
		case 'z':	/* formatted ZXML */
		switch (cch)
		{
			case 0x0D:
			case 0x14:
			fprintf (outfile,"<BR>");
			xpos = 0;
			break;

			/* CPC colour codes live in this range, but I don't know 
			 * what they mean */

			case 9:	reverse ^= 1;
					if (reverse) fprintf(outfile, "<INVERSE>");
					else		 fprintf(outfile, "</INVERSE>");
					break;
					
			case '%': fprintf (outfile,"%%25;");   ++xpos; break;
			case '<': fprintf (outfile,"&lt;");    ++xpos; break;
			case '>': fprintf (outfile,"&gt;");    ++xpos; break;
			case '&': fprintf (outfile,"&amp;");   ++xpos; break;
			case 96 : fprintf (outfile,"&pound;"); ++xpos; break;
			case 127: fprintf (outfile,"&copy;");  ++xpos; break;

			default:
			if ((cch > 31) && (cch < 127)) fputc(cch,outfile);
/*			else if (cch > 164) expdict (cch, n); */
			else fprintf(outfile,"%%%x;",cch);
			++xpos;
		}
		if (oopt == 'z' && xpos > 39) { fprintf(outfile, "<BR>"); xpos = 0; }
		break;
	}
}



void expch(uchr cch, ushrt *n)
{
	ushrt tbsp;

	if (arch == ARCH_CPC)
	{
		expch_cpc(cch, n);
		return;
	}
	if (arch == ARCH_C64)
	{
		expch_c64(cch, n);
		return;
	}
	
	switch (oopt)
	{
		case 'I':		/* Inform */
		switch(cch)
		{
			case '^':  fprintf(outfile, "@@9");  cch = '4'; break;
			case '@':  fprintf(outfile, "@@6");  cch = '4'; break;
			case '~':  fprintf(outfile, "@@12"); cch = '6'; break;
			case '\\': fprintf(outfile, "@@9");  cch = '2'; break;
			case '"':  cch = '~'; break;
			case '`':  fprintf(outfile, "@L");   cch = 'L'; break;
			case 0x0D: cch = '^'; break;

		}
		/* FALL THROUGH */

		case 'T':                 /* Plain text */
		if      ((cch > 31) && (cch <= 164)) opch32(cch);
		else if (cch > 164)  expdict(cch, n);
		else if (cch == 6)   for (opch32(' ');(xpos%16);opch32(' '));
		else if (cch == 8)   opch32(8);
		else if (cch ==0x0D) opch32('\n');
		else if (cch == 0x17)
		{
			tbsp=(255-zmem((*n)++)) & 0x1F;
			++(*n);
			if (xpos > tbsp) opch32('\n');
			for (;tbsp>0;tbsp--) opch32(' ');
		}
		else if ((cch > 0x0F) && (cch<0x16))
		{
			switch (cch)
			{
				case 0x10: (console->ink)(255 - zmem(*n)); break;
				case 0x11: (console->paper)(255 - zmem(*n)); break;
				case 0x12: (console->flash)(255 - zmem(*n)); break;
				case 0x13: (console->bright)(255 - zmem(*n)); break;
				case 0x14: (console->inverse)(255 - zmem(*n)); break;
				case 0x15: (console->over)(255 - zmem(*n)); break;
			}
 			(*n)++;
		}
		break;

		case 'Z':	/* ZXML */
		case 'z':	/* Formatted ZXML */
		switch (cch)
		{
			case 0x0D:
			fprintf (outfile,"<BR>");
			xpos = 0;
			break;

			case 0x17:
			xpos += 255 - zmem(*n);
			fprintf (outfile,"<TAB%d>",((255-zmem((*n)++)) & 0x1F));
			++(*n);
			break;

			case '%': fprintf (outfile,"%%25;");   ++xpos; break;
			case '<': fprintf (outfile,"&lt;");    ++xpos; break;
			case '>': fprintf (outfile,"&gt;");    ++xpos; break;
			case '&': fprintf (outfile,"&amp;");   ++xpos; break;
			case 96 : fprintf (outfile,"&pound;"); ++xpos; break;
			case 127: fprintf (outfile,"&copy;");  ++xpos; break;

			case 16:  
			fprintf (outfile,"<INK=%d>",255-zmem((*n)++));
			break;

			case 17:  
			fprintf (outfile,"<PAPER=%d>",255-zmem((*n)++));
			break;

		        case 18:
			if (zmem((*n)++)==0xFF) fprintf (outfile,"</BLINK>");
			else fprintf (outfile,"<BLINK>");
			break;

			case 19:
			if (zmem((*n)++)==0xFF) fprintf (outfile,"</BRIGHT>");
			else fprintf (outfile,"<BRIGHT>");
			break;

			case 20:
			if (zmem((*n)++)==0xFF) fprintf (outfile,"</INVERSE>");
			else fprintf (outfile,"<INVERSE>");
			break;

			case 21:
			if (zmem((*n)++)==0xFF) fprintf (outfile,"</OVER>");
			else fprintf (outfile,"<OVER>");
			break;

			case 6: 
			if (oopt == 'Z')
			{
				fprintf(outfile,"<,>"); 
				break;
			}
			xpos = (xpos + 16) & (~15);
			break;	

		        default:
			if ((cch > 31) && (cch < 127)) fputc(cch,outfile);
			else if (cch > 164) expdict (cch, n);
			else fprintf(outfile,"%%%x;",cch);
			++xpos;
			break;
		}
		if (oopt == 'z' && xpos > 31) 
		{
			fprintf(outfile, "<BR>");
			xpos = 0;
		}
		break;
	}
}


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


void fontout(ushrt addr, ushrt nchars, char *title, char *desc)
{
	char n, m, xbpos = 0;

	fprintf(outfile,"%04x: %s\n"
                        "#define %s_width %d\n"
                        "#define %s_height 8\n"
                        "\n"
	                "static char %s_bits[] = {\n ", addr, desc,
			title, nchars * 8, title, title);

	for (m = 0; m < 8; m++) for (n = 0; n < nchars; n++)
	{
		fprintf(outfile,"0x%2.2x", mirror(zmem(addr + (8 * n) + m)));
		if (++xbpos == 12)
		{
			if ((n==(nchars-1)) && (m==7)) fprintf(outfile, "};\n");
			else fprintf(outfile, ",\n ");
			xbpos=0;
        	}
	        else    fprintf(outfile,", ");
	}
}

void listudgs(ushrt addr)
{
	fontout(addr, 21, "udg", "User-defined graphics");
}

void listfont(ushrt addr)
{
	ushrt a, b, some, index, std_done;
	char fnt_name[10];

	std_done = 0;
	some = index = 0;
/* [new in v0.9.0] */
/* Before doing a standard font dump, try searching the game code for writes
 * to the "font" system variable. This should bring out all fonts used by the
 * game engine, whether or not they are currently selected. */

/* This byte range is my guess at the area occupied by the Quill engine. */	
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
				fprintf(outfile, "      ;Font %d is the system font\n", index); 						
				continue;
			}
			++some;
			sprintf(fnt_name,"font%d", index);
			fontout(b + 256, 96, fnt_name, "User-defined font");
			if (b == addr) std_done = 1;
		}
	}
/* If the search above didn't find the currently selected font, dump that. */
	if (addr >= 0x5B00 && !std_done)
		fontout(addr+256, 96, "font", "User-defined font");
}


static const char *p1_moves[] = {" 001  000",
                                 " 001  001",
				 " 000  001",
				 "-001  001",
				 "-001  000",
				 "-001 -001",
				 " 000 -001",
				 " 001 -001" };

/* 2 is a lousy sample size, but all the Spectrum Quill games I've examined 
 * have their graphics tables at 0xFAB8. */

void listgfx(void)
{
	ushrt n, m;
	ushrt gfx_base  = 0xFAB8;	/* Arbitrary */
	ushrt gfx_ptrs  = zword(gfx_base);
	ushrt gfx_flags = zword(gfx_base + 2);
	ushrt gfx_count = zmem (gfx_base + 6);
	uchr gflag, nargs = 0, value;
	ushrt gptr;
	char inv, ovr;
	char *opcode = NULL;
	ushrt neg[8];
	char tempop[30];

	fprintf(outfile, "%04x: There are %d graphics record%s.\n",
			gfx_base + 6, gfx_count, (gfx_count == 1) ? "" : "s");
	for (n = 0; n < gfx_count; n++)
	{
		fprintf(outfile, "%04x: Location %d graphics flag: ", 	
				gfx_flags + n, n);
		gflag = zmem (gfx_flags + n);
		gptr  = zword(gfx_ptrs  + 2*n);

		fprintf(outfile, "      ");
		if (gflag & 0x80) fprintf(outfile, "Location graphic.\n");
		else		  fprintf(outfile, "Not location graphic.\n");
		fprintf(outfile, "      Ink=%d Paper=%d Bit6=%d\n", 
			(gflag & 7), (gflag >> 3) & 7, (gflag >> 6) & 1);

		do
		{
			memset(neg, 0, sizeof(neg));
			gflag = zmem(gptr);
			fprintf(outfile, "%04x: ", gptr);
			inv = ' '; ovr = ' ';
			if (gflag & 0x08) ovr = 'o';
			if (gflag & 0x10) inv = 'i';
			value = (gflag >> 3);
			switch(gflag & 7)
			{
				case 0:  nargs = 2;  
                                         opcode = tempop;
                                         sprintf(opcode, "PLOT  %c%c",
							ovr, inv);
					 if (ovr == 'o' && inv == 'i')
						strcpy(opcode, "AMOVE");
					 break;
				case 1:  nargs = 2; 
                                         if (gflag & 0x40) neg[0] = 1;
                                         if (gflag & 0x80) neg[1] = 1;

					 if (ovr == 'o' && inv == 'i')
						opcode = "MOVE";	
					 else
					 {
						opcode = tempop;
           	                                sprintf(opcode, "LINE  %c%c",
                                                        ovr, inv);
					 }
					 break;
				case 2:  switch(gflag & 0x30)
					{
						case 0x00:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 2; 
							opcode = "FILL";
							break; 
						case 0x10:
							nargs = 4; 
							opcode = "BLOCK";
							break;
						case 0x20:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 3; 
							opcode = "SHADE";
							break;
						case 0x30:
                                                	if (gflag & 0x40) neg[0] = 1;
                                                	if (gflag & 0x80) neg[1] = 1;
							nargs = 3; 
							opcode = "BSHADE";
							break;
						}
					 break;
				case 3:  nargs = 1; 
					 sprintf(tempop, "GOSUB  sc=%03d",
					 		value & 7);
					 opcode = tempop; 
					break;
				case 4:  nargs = 0; 
					 sprintf(tempop, "RPLOT %c%c %s",
							ovr, inv, p1_moves[(value / 4)]);
					 opcode = tempop;
					 break;
				case 5:  nargs = 0; 
					 opcode = tempop;
					 if (gflag & 0x80) sprintf(tempop,
					  "BRIGHT    %03d ", value & 15);
					 else sprintf(tempop,
					  "PAPER     %03d ", value & 15);
					 break;
                                case 6:  nargs = 0; 
					 opcode = tempop;
                                         if (gflag & 0x80) sprintf(tempop,
                                          "FLASH     %03d ", value & 15);
                                         else sprintf(tempop,
                                          "INK       %03d ", value & 15);
					 break;
				case 7:  nargs = 0; opcode = "END";   break;
			}
			fprintf(outfile, "%-8s ", opcode); 
			for (m = 0; m < nargs; m++)
			{
				fprintf(outfile, "%c%03d ", 
					neg[m] ? '-' : ' ',
					zmem(gptr + 1 + m));
			}
			fprintf(outfile, "\n");

			gptr += (nargs + 1);

		}
		while((gflag & 7) != 7);

	}
}
