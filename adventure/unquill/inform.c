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

#ifndef TINY
static char *lookup_word(ushrt wid);
static void inform_process_tables(uchr which);
static void add_clause(ushrt *ptr, char *clause);
static void get_action(ushrt *ptr);
static void inform_vocab(void);
static char *xlt_dir(ushrt w, char *suffix);

static uchr verb[256];
static uchr noun[256];
static uchr dummy_obj[256];


static void print_libs(void)
{

/* Map Quill flags to special bits of Inform state */

	fprintf(outfile, "[QuillFlagSet flag var; \n"
                         " QuillFlags->flag = var; \n"
                         " if (flag == 0 && var == 0) give player ~light; \n"
                         " if (flag == 0 && var ~= 0) give player light; \n"
			 " if (flag == 30) score = var;\n"
                         "];\n\n");

/* Handle pause */

	fprintf(outfile, "[QuillDummy ; rfalse; ];\n\n");

	if (dbver > 0)
	{
		fprintf(outfile, "[QuillPause x y; \n"
                                 " switch(QuillFlags->28)\n"
                                 " {\n"
                                 "  7: font on; rtrue;\n"
                                 "  8: font off; rtrue;\n"
                                 " }\n");

/* XXX more functions are needed here! */

	}
	else fprintf(outfile, "[QuillPause x y; \n");
	fprintf(outfile, " x = (x / 5); \n"
                         " @read_char 1 x QuillDummy -> y;\n"
                         "];\n\n" );


/* Decrement a flag if nonzero */

	fprintf(outfile, "[QuillDec flag; \n"
                         " if (QuillFlags->flag > 0)\n"
                         " { QuillFlags->flag = QuillFlags->flag - 1; }\n" 
                         "];\n\n");

/* Things to do when a location is described */

	fprintf(outfile, "[LookRoutine; \n"
                         " QuillDec(2); \n"
                         " if (player hasnt light)  QuillDec(3); \n"
                         " if (location == thedark) QuillDec(4); \n"
                         "];\n\n");

/* Things to do each turn (done as a daemon in the player object) */

        fprintf(outfile, "\n\n"
                         "Object QuillPlayer\n  "
                         "    with daemon [ n i;\n"
                         "         QuillFlags->1 = 0; \n"
                         "         objectloop(i in player) "
                         "{ if (i hasnt worn) "
                         "{ QuillFlags->1 = QuillFlags->1 + 1; } }\n"
			 "         for (n=5:n<9:n++) QuillDec(n); \n"
                         "         if (player hasnt light) QuillDec(9); \n"
			 "         if (location == thedark) QuillDec(10); \n");
	if (!skipc) fprintf(outfile, "         QuillProcess();\n");
	fprintf(outfile, "         ],\n"
                         "    has light transparent concealed animate proper;\n"
                         "! This is where Quill's global light flag ends up \n"
                         "\n\n");
}


/* Given a Quill object number, find a name for it */

static uchr ob_name(uchr n, char *name)
{
	uchr word = 0xFF;

	if (dbver > 0) word = zmem(objmap + n);

	if (word == 0xFF) 
	{
		sprintf(name, "Object_%d", n);
		return word;	/* No name */
	}

	strcpy(name, lookup_word(word));
	return word;		/* Named */
}

/* Find a word from the Quill vocabulary */

static char *lookup_word(ushrt wid)
{
	ushrt n, m;
	static char w[5];

	w[0] = w[4] = 0;

	for (n=vocab; zmem(n); n=n+5) if (zmem(n+4) == wid)
	{
		for (m = 0; m < 4; m++) w[m] = 0xFF - zmem(m+n);
	}
	return w;
}


/* Given a word, find the object to which it refers. */

static char *ob_from_word(ushrt wid)
{
	ushrt n;
	static char buf[20];

	if (wid < 11) return xlt_dir(wid, "obj");	/* Direction */

	if (dbver > 0) for (n = 0; n < nobj; n++)
	{
		if (zmem(objmap + n) == wid)
		{
			ob_name(n, buf);
			return buf;
		}
	}
	dummy_obj[wid] = 1;
	sprintf(buf, "WordObj_%s", lookup_word(wid));
	return buf;
}






static void list_words(ushrt wid)
{
	char w[5];
        ushrt n, m;

        for (n=vocab; zmem(n); n=n+5) if (zmem(n+4) == wid)
        {
                for (m = 0; m < 4; m++) 
		{
			w[m] = 0xFF - zmem(m+n);
			if (w[m] == ' ') w[m] = 0;
		}
		w[m] = 0;
		if (strlen(w) == 1) strcat(w, "//");
 
                fprintf(outfile, "'%s' ", w);
        }
}




static char *xlt_dir(ushrt w, char *suffix)
{
	char *word = lookup_word(w);
	char *strip;
	static char dirbuf[8];

/* Recognise common Quill directional words. ASCE -> ASCEND, CLIM -> CLIMB 
  and DOWN/DESC are overloaded so that 'D' will do either! */

	dirbuf[0] = 0;
	
	if (!strcmp(word, "N   ") || !strcmp(word, "NORT")) strcpy(dirbuf, "n_");
        if (!strcmp(word, "S   ") || !strcmp(word, "SOUT")) strcpy(dirbuf, "s_");
        if (!strcmp(word, "E   ") || !strcmp(word, "EAST")) strcpy(dirbuf, "e_");
        if (!strcmp(word, "W   ") || !strcmp(word, "WEST")) strcpy(dirbuf, "w_");
        if (!strcmp(word, "U   ") || !strcmp(word, "UP  ") ||  
            !strcmp(word, "ASCE") || !strcmp(word, "CLIM")) strcpy(dirbuf, "u_");
        if (!strcmp(word, "D   ") || !strcmp(word, "DOWN") ||
            !strcmp(word, "DESC"))                          strcpy(dirbuf, "d_");
        if (!strcmp(word, "IN  "))                          strcpy(dirbuf, "in_");
        if (!strcmp(word, "OUT "))                          strcpy(dirbuf, "out_");
        if (!strcmp(word, "NE  "))                          strcpy(dirbuf, "ne_");
        if (!strcmp(word, "SE  "))                          strcpy(dirbuf, "se_");
        if (!strcmp(word, "NW  "))                          strcpy(dirbuf, "nw_");
        if (!strcmp(word, "SW  "))                          strcpy(dirbuf, "sw_");

	if (dirbuf[0] == 0)
	{
		strip = strchr(word, ' ');
		if (strip) *strip = 0;		/* Trim spaces off the word */

		sprintf(dirbuf, "%s_%s", word, suffix);
	}
	else strcat(dirbuf, suffix);	
	return dirbuf;	
}

/* Print the objects in a room */

static void inform_objects_in(ushrt room, uchr arrow)
{
	uchr n, named, wearable;
	char obname[20];

	for (n = 0; n < nobj; n++) if (zmem(postab + n) == room)
	{
		wearable = 0;
		named = ob_name(n, obname);
		fprintf(outfile, "Object %s %s \"", 
                                  arrow ? "->" : "",
                                  obname);
		indent = 20;
   		oneitem(objtab, n); 
		fprintf(outfile, "\"");

		if (named != 0xFF) 
		{
			fprintf(outfile, "\n    with name ");
			list_words(zmem(objmap + n));
			if (named >= 200) wearable = 1; 
		}
		if (room == 0xFD) wearable = 1;
		if (wearable || n == 0 || room == 0xFD)
		{
			fprintf(outfile, "\n    has %s%s%s",
				wearable       ? "clothing " : "",
                                (room == 0xFD) ? "worn "     : "",
                                (n == 0)       ? "light"     : ""); 
		}
		fprintf(outfile, ";\n\n");
	}



}


/* Print out the data for a room - ie, its entries in the Locations and 
  Connections tables */

static void inform_room(ushrt room)
{
	ushrt nconn  = zword(conntab + 2*room);

	fprintf(outfile, "Object Room_%d\n", room);
	fprintf(outfile, "    with description \"");

	indent = 22;
	oneitem(loctab, room);
	fprintf(outfile,"\"");

	/* Work out the connections from this room */

	while (zmem(nconn) != 0xFF)
	{
		uchr word, dest;
		char *dir;

		word = zmem(nconn++);
		dest = zmem(nconn++);

		/* Find out if the direction is one we recognise */
		dir = xlt_dir(word, "to");
		fprintf(outfile,",\n         %s Room_%d", dir, dest);
	}
	fprintf(outfile,";\n\n");

	inform_objects_in(room, 1);	
}


void inform_src(ushrt zxptr)
{
	int room;

	fprintf(outfile, "\n\nArray QuillFlags -> 32;\n\n"
                         "Include \"Parser\";\n\n");

	if (!skipl)
	{
		for (room = 0; room < nloc; room++)
		{
			inform_room(room);
		}
	}
	print_libs();

	if (!skipl)
	{
		/* Carried */
	        inform_objects_in(0xFE, 1);
		/* Worn */
		inform_objects_in(0xFD, 1);

		/* Limbo */
		fprintf(outfile, "\n\n");
		inform_objects_in(0xFC, 0);
	}

	inform_process_tables(0);
        inform_process_tables(1);

        if (!skipv) inform_vocab();

	fprintf(outfile, "\n\n"
                        "Include \"VerbLib\";\n"
                        "Include \"Grammar\";\n\n");

	fprintf(outfile,
                        "[ Initialise ; \n\n"
                        "  location = Room_0;    ! Always starts in room 0\n"
                        "  player = QuillPlayer; ! For various reasons\n"
			"  StartDaemon(QuillPlayer); \n");
	if (!skipc) fprintf(outfile, "  QuillProcess(); \n");
	fprintf(outfile, "  lookmode = 2;         ! Quill always prints the description\n"
                        "  notify_mode = 0;      ! Quill doesn't notify\n"
                        "];\n\n");
}


/* The process tables are where the differences really show between 
  Quill's imperative system and Inform's OO system. We simulate the 
  Response table with a GamePreRoutine... */

static void inform_process_tables(uchr process)
{
	char *prefix = skipc ? "! " : "";
	char *ind;
	uchr  w1, w2, ifw, ifi;
	ushrt ptr, ptr2;
	static char clause[50][50];

	if (skipc) comment_out = 1;

	if (process)
	{
		fprintf(outfile, "!\n! Process conditions\n!\n");	
                fprintf(outfile, "%s[QuillProcess loc tmp i;\n%s\n",
                                              prefix, prefix);
                fprintf(outfile, "%s    loc = location;\n"
                                 "%s    if (loc == thedark) "
                                       "loc = real_location;\n",prefix,prefix);

		ptr = proctab;
	}
	else
	{
		fprintf(outfile, "!\n! Response conditions\n!\n");
		fprintf(outfile, "%s[GamePreRoutine loc tmp i;\n%s\n",
                                              prefix, prefix);
		fprintf(outfile, "%s    loc = location;\n"
                                 "%s    if (loc == thedark) "
                                       "loc = real_location;\n",prefix,prefix);
		ptr = resptab;
	}

	while (zmem(ptr))
	{
		ifw  = 0;
		w1   = zmem(ptr++);
		w2   = zmem(ptr++);
		ptr2 = zword(ptr++);
		++ptr;

		if (w1 != 0xFF && !process)
		{
			if (w1 < 11) /* Movement word */
			{
				sprintf(clause[ifw++], "(action == ##Go)");
				sprintf(clause[ifw++], "(noun == %s)", 
                                        ob_from_word(w1));	
			}
			else sprintf(clause[ifw++], "(action == ##%s)", 
                                     lookup_word(w1));
			verb[w1] = 1;
		}
		if (w2 != 0xFF && !process)
		{
			sprintf(clause[ifw++], "(noun == %s)", ob_from_word(w2));
			noun[w2] = 1;
		}
		while (zmem(ptr2) != 0xFF)
		{
			add_clause(&ptr2, &clause[ifw++][0]);
		}
		
		if (ifw) 
		{
			fprintf(outfile, "%s    if (", prefix);

			for (ifi = 0; ifi < ifw; ifi++)
			{
				fprintf(outfile, "%s", clause[ifi]);
				if (ifi == (ifw - 1)) fputc(')', outfile);
				else
				{
					fprintf(outfile, " && \n%s        ", prefix);
				}
			}
			fprintf(outfile, "\n%s    {\n", prefix);
			ind = "        ";
		}
		else    ind = "    ";
	
		++ptr2;	
		while (zmem(ptr2) != 0xFF)
		{
			fprintf(outfile, "%s%s", prefix, ind);
			get_action(&ptr2);
		}
		if (ifw) fprintf(outfile, "%s    }\n", prefix);
	}
	fprintf(outfile,"%s    rfalse;\n%s];\n\n\n", prefix, prefix);
	comment_out = 0;
}




static void add_clause(ushrt *ptr, char *clause)
{
	uchr id;
	uchr param1 = 0, param2 = 0;
	char obname[20];

	id      = zmem((*ptr)++);
	param1  = zmem((*ptr)++);

	if (id > 12) param2  = zmem((*ptr)++);

	switch(id)
	{
		case 0: sprintf(clause, "(loc == Room_%d)", param1); break;
		case 1: sprintf(clause, "(loc ~= Room_%d)", param1); break;
		case 2: sprintf(clause, "(loc >  Room_%d)", param1); break;
		case 3: sprintf(clause, "(loc <  Room_%d)", param1); break;
		case 4: ob_name(param1, obname);
			sprintf(clause, "(IndirectlyContains (loc, %s))",
			        obname);
			break;
		case 5: ob_name(param1, obname);
			sprintf(clause, "(~IndirectlyContains (loc, %s))",
				obname);
			break;
		case 6: ob_name(param1, obname);
			sprintf(clause, "(%s has worn)", obname);
			break;
		case 7: ob_name(param1, obname);
			sprintf(clause, "(%s hasnt worn)", obname);
			break;
		case 8: ob_name(param1, obname);
			sprintf(clause, "(IndirectlyContains (player, %s))",
                                obname);
			break;
		case 9: ob_name(param1, obname);
			sprintf(clause, "(~IndirectlyContains (player, %s))",
				obname);
			break;
		case 10: sprintf(clause, "(random(100) < %d)", param1 + 1);
			 break;
		case 11: if (param1 == 0) 
			 {
				sprintf(clause, "(player hasnt light)");
				break;
			 }
		 	 sprintf(clause, "(QuillFlags->%d == 0)", param1);
			 break;
		case 12: if (param1 == 0)
			 {
				sprintf(clause, "(player has light)");
				break;	
			 }
		 	 sprintf(clause, "(QuillFlags->%d ~= 0)", param1);
			 break;
		case 13: sprintf(clause, "(QuillFlags->%d == %d)", param1,
                                                                   param2);
			 break;
                case 14: sprintf(clause, "(QuillFlags->%d > %d)", param1,
                                                                   param2);
                         break;
                case 15: sprintf(clause, "(QuillFlags->%d < %d)", param1,
                                                                   param2);
                         break;
	}
		
}



static void get_action(ushrt *ptr)
{
	static char inited=0;
	static char params[40];
	uchr id = zmem((*ptr)++);
	uchr param1 = 0, param2 = 0;
	uchr n;
	char obname[20], obname2[20];

	if (!inited)
  	{
		for (n = 0; n < 39; n++)
		{
			if      (n < 17) params[n] = 0;
			else if (n < 33) params[n] = 1;
			else             params[n] = 2;
			params[29] = 2;
			params[30] = 2;
		}
		if (arch == ARCH_CPC) params[19] = 2;
		++inited;
	}
	if (!dbver)
	{
		if (id  > 11) id += 9;
		if (id == 11) id = 17;
		if (id >  29) ++id;
	}
	if (params[id] > 0) param1 = zmem((*ptr)++);
	if (params[id] > 1) param2 = zmem((*ptr)++);

	switch(id)
	{	
		/* INVEN */
		case 0: fprintf(outfile, "InvTallSub();\n");  break;
		/* DESC */
		case 1: fprintf(outfile, "LookSub(0); rtrue;\n"); break;
		/* QUIT */
		case 2: fprintf(outfile, "print \"");
			xpos = 0; sysmess(12);
			fprintf(outfile, "\"; if (YesOrNo() == 0) rtrue;\n");
			break;
		/* END */
		case 3:	fprintf(outfile, "quit;\n"); break;
		/* DONE */
		case 4: fprintf(outfile, "rtrue;\n"); break;
		/* OK */
		case 5: fprintf(outfile, "\"");
			xpos = 0; sysmess(15);
			fprintf(outfile, "\";\n"); 
			break;
		/* ANYKEY */
		case 6: fprintf(outfile, "print \"");
			xpos = 0; sysmess(16);
			fprintf(outfile, "\"; @read_char 1 tmp; print \"^\";\n");
			break;
		/* SAVE */
		case 7: fprintf(outfile, "SaveSub(); LookSub(); rtrue;\n"); break;
		/* LOAD */
		case 8: fprintf(outfile, "RestoreSub(); rtrue;\n");    break;
		/* TURNS */
		case 9: fprintf(outfile, "print \"");
			xpos = 0; sysmess(17);
			fprintf(outfile, "\", turns; ");
			if (dbver > 0) 
			{
				fprintf(outfile, "if (turns > 1) print \"");
				xpos = 0; sysmess(19);
				fprintf(outfile, "\"; print \"");
				xpos = 0; sysmess(20);
				fprintf(outfile, "\";\n");
			}
			else fprintf(outfile, "if (turns > 1) print \"s\"; "
                                              "print \".\";\n");
			break;
		/* SCORE */
		case 10: fprintf(outfile, "print \"");
			 xpos = 0; sysmess(19 + (dbver ? 2 : 0));
			 fprintf(outfile, "\", score; print \"");
			 if (dbver > 0) 
			 {
				xpos = 0; sysmess(22);
				fprintf(outfile, "^\";\n");	
			 }
			 else fprintf(outfile, ".^\";\n");
			 break;
		/* CLS */
		case 11: fprintf(outfile, "@erase_window -1;\n"); break;
		/* DROPALL */
		case 12: fprintf(outfile, "objectloop(i hasnt worn) { "
                                          "if (i in player) { "
                                          "move i to parent(player); }};\n");
			 fprintf(outfile, "! DROPALL (not a true DROP ALL)\n");
			 break;
		/* AUTOG */
		case 13: fprintf(outfile, "TakeSub();\n"); break;
		/* AUTOD */
		case 14: fprintf(outfile, "DropSub();\n"); break;
		/* AUTOW */
		case 15: fprintf(outfile, "WearSub();\n"); break;
		/* AUTOR */
		case 16: fprintf(outfile, "DisrobeSub();\n"); break;
		/* PAUSE (complex) */
		case 17: fprintf(outfile, "QuillPause(%d);\n", param1); break;
		/* PAPER */
		case 18: fprintf(outfile, "! PAPER %d;\n",  param1); break;
		/* INK */
		case 19: if (arch == ARCH_CPC)
			 {
				fprintf(outfile, "! INK %d %d;\n", param1, param2); 
				break;
			 }
			 fprintf(outfile, "! INK %d;\n",    param1); break;
		/* BORDER */
		case 20: fprintf(outfile, "! BORDER %d;\n", param1); break;
		/* GOTO */
		case 21: fprintf(outfile, "PlayerTo(Room_%d);\n", param1);
			 break;
		/* MESSAGE */
		case 22: fprintf(outfile, "print \"");
			 oneitem(msgtab, param1);
			 fprintf(outfile, "\";\n");
			 break;
		/* REMOVE */
		case 23: ob_name(param1, obname);
			 fprintf(outfile, "<Remove %s>;\n", obname); 
		 	 break;
                /* GET */
                case 24: ob_name(param1, obname);
                         fprintf(outfile, "<Take %s>;\n", obname);
                         break;
                /* DROP */
                case 25: ob_name(param1, obname);
                         fprintf(outfile, "<Drop %s>;\n", obname);
                         break;
                /* WEAR */
                case 26: ob_name(param1, obname);
                         fprintf(outfile, "<Wear %s>;\n", obname);
                         break;
		/* DESTROY */
		case 27: ob_name(param1, obname);
			 fprintf(outfile, "remove %s;\n", obname);
			 break;
		/* CREATE */
		case 28: ob_name(param1, obname);
			 fprintf(outfile, "move %s to loc;\n", obname);
			 break;
		/* SWAP */
		case 29: ob_name(param1, obname);
			 ob_name(param2, obname2);
			 fprintf(outfile, "tmp = parent(%s); "
                                          "move %s to parent(%s); "
                                          "move %s to tmp;\n",
                                          obname, obname2, obname, obname2);
			 break;
    
		/* PLACE */
		case 30: ob_name(param1, obname);
                         fprintf(outfile, "move %s to Room_%d;\n", obname, 
                                                                  param2);
                         break;
		/* SET */
		case 31: fprintf(outfile, "QuillFlagSet(%d,255);\n", param1);
			 break;
                /* CLEAR */
                case 32: fprintf(outfile, "QuillFlagSet(%d,0);\n", param1);
                         break;
                /* PLUS */
                case 33: fprintf(outfile, "QuillFlagSet (%d,QuillFlags->%d + %d);\n", param1, param1, param2);
                         break;
		/* MINUS */
               case 34: fprintf(outfile, "QuillFlagSet (%d,QuillFlags->%d - %d);\n", param1, param1, param2);
                         break;
		/* LET */
               case 35: fprintf(outfile, "QuillFlagSet (%d,%d);\n", 
				param1, param2);
			break;
                /* BEEP */
                case 36: fprintf(outfile, "! BEEP %d %d; \n", param1, param2);
			 break;
	}
}

/* Do not generate action routine stubs for routines in the Inform library */

static uchr isinformese(char *s)
{
	if (!strcmp(s, "Quit")) return 1;
	if (!strcmp(s, "Save")) return 1;
	if (!strcmp(s, "Take")) return 1;
	if (!strcmp(s, "Inv"))  return 1;
	if (!strcmp(s, "Wear")) return 1;
	if (!strcmp(s, "Drop")) return 1;
	if (!strcmp(s, "Exit")) return 1;
	if (!strcmp(s, "Go"))   return 1;
        if (!strcmp(s, "Look")) return 1;
        if (!strcmp(s, "Open")) return 1;
	if (!strcmp(s, "Eat"))  return 1;
	if (!strcmp(s, "Jump")) return 1;
        if (!strcmp(s, "Fill")) return 1;
	if (!strcmp(s, "Pull")) return 1;
	if (!strcmp(s, "Push")) return 1;
	if (!strcmp(s, "Kiss")) return 1;
	if (!strcmp(s, "Pray")) return 1;
	if (!strcmp(s, "Turn")) return 1;
	if (!strcmp(s, "Give")) return 1;
	if (!strcmp(s, "Yes"))  return 1;
	if (!strcmp(s, "No"))   return 1;
	if (!strcmp(s, "Cut"))  return 1;
	if (!strcmp(s, "Buy"))  return 1;
	if (!strcmp(s, "Wave")) return 1;
	if (!strcmp(s, "Wait")) return 1;

	if (!strcmp(s, "Get"))
	{
		fprintf(outfile, "[ GetSub; <<Take noun>>; ];\n");
		return 1;
	}

	if (!strcmp(s, "Scor")) 
	{
		fprintf(outfile, "[ ScorSub; ScoreSub(); ];\n");
		return 1;
	}
	if (!strcmp(s, "N") || !strcmp(s, "Nort")) 
	{
		fprintf(outfile, "[ %sSub; <<Go n_obj>>; ];\n", s);
		return 1;	
	}
        if (!strcmp(s, "S") || !strcmp(s, "Sout"))
        {
                fprintf(outfile, "[ %sSub; <<Go s_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "E") || !strcmp(s, "East"))
        {
                fprintf(outfile, "[ %sSub; <<Go e_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "W") || !strcmp(s, "West"))
        {
                fprintf(outfile, "[ %sSub; <<Go w_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "U") || !strcmp(s, "Up") || !strcmp(s, "Clim") ||
            !strcmp(s, "Asce"))
        {
                fprintf(outfile, "[ %sSub; <<Go u_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "D") || !strcmp(s, "Down") || !strcmp(s, "Desc"))
        {
                fprintf(outfile, "[ %sSub; <<Go d_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "NE"))
        {
                fprintf(outfile, "[ %sSub; <<Go ne_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "SE"))
        {
                fprintf(outfile, "[ %sSub; <<Go se_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "NW"))
        {
                fprintf(outfile, "[ %sSub; <<Go nw_obj>>; ];\n", s);
		return 1;
        }
        if (!strcmp(s, "SW"))
        {
                fprintf(outfile, "[ %sSub; <<Go sw_obj>>; ];\n", s);
		return 1;
        }

	return 0;
}


/* Create "actions" and "objects" to hang Quill vocab from */

static void inform_vocab(void)
{
        ushrt syn, n, m;
        static char w[5];

	/* Objects which only exist to have nouns */
	for (n = 0; n < 255; n++) if (dummy_obj[n])
	{
		fprintf(outfile, "Object WordObj_%s\n", lookup_word(n));
		fprintf(outfile, "    with name ");
		list_words(n);
		fprintf(outfile, ";\n");
	}

        w[0] = w[4] = 0;

        for (n=vocab; zmem(n); n=n+5) if (verb[zmem(n+4)])
        {
		for (m = 0; m < 4; m++) 
		{
			w[m] = 0xFF - zmem(m+n);
			if (w[m] == ' ') w[m] = 0;
		}
		for (m = 1; m < 4; m++) if (isupper(w[m])) w[m] = tolower(w[m]);
		if (isinformese(w)) continue;

		fprintf(outfile, "[ %sSub ;  ];\n", w);
	}
	for (syn = 0; syn < 255; syn++) if (verb[syn])
	{ 
		ushrt sc = 0;
	        for (n=vocab; zmem(n); n=n+5) if (syn == zmem(n+4))
		{
                	for (m = 0; m < 4; m++) 
			{
				w[m] = 0xFF - zmem(m+n);
				if (w[m] == ' ') w[m] = 0;
			}
			for (m = 1; m < 4; m++) 
				if (isupper(w[m])) w[m] = tolower(w[m]);

			if (!sc) fprintf(outfile, "Verb ");
			++sc;
			fprintf(outfile, "'%s' ", w);
		}
 		if (sc) fprintf(outfile, "* -> %s;\n", w); 
	}
}






#endif	/* ndef TINY */

