! syntax=inform
! 
! IQRun: A port of the Quill-running parts of UnQuill to Inform.
!       Handle process and response tables
! Copyright John Elliott: 24 November 1999
!
!    This library is free software; you can redistribute it and/or
!    modify it under the terms of the GNU Library General Public
!    License as published by the Free Software Foundation; either
!    version 2 of the License, or (at your option) any later version.
!
!    This library is distributed in the hope that it will be useful,
!    but WITHOUT ANY WARRANTY; without even the implied warranty of
!    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
!    Library General Public License for more details.
!
!    You should have received a copy of the GNU Library General Public
!    License along with this library; if not, write to the Free
!    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
!

[ RamLoad n;
	if (RamSaveBuf->256 == 0) rfalse;

	for (n = 0: n < 37: n++)
	{
		QuillFlags->n = RamSaveBuf->n;
	}
	for (n = 0: n < 219: n++)
	{
		ObjectPos->n = RamSaveBuf->(n+37);
	}

];


[ RamSave n;
	RamSaveBuf->256 = 255;

        for (n = 0: n < 37: n++)
        {
                RamSaveBuf->n = QuillFlags->n;
        }
        for (n = 0: n < 219: n++)
        {
                RamSaveBuf->(n + 37) = ObjectPos->n;
        }
];



[ AttemptMove cbase;

	cbase = peekw(2 * QuillFlags->35 + Connections);
	
	while (Zmem(cbase) ~= 255)
	{
		if (QuillVerb == Zmem(cbase))
		{
			++cbase;
			QuillFlags->35 = Zmem(cbase);
			rtrue;	
		}
		else cbase = cbase + 2;
	}
	rfalse;
];


[ffeq a b;
	if (a == b) rtrue;
	if (a == 255) rtrue;
	if (b == 255) rtrue;
	rfalse;
];

[ Present ob pos;

	pos = ObjectPos->ob;

	if (pos == 254) rtrue;
	if (pos == 253) rtrue;
	if (pos == QuillFlags->35) rtrue;

	rfalse;
];

[SkipCondition ccond cond;

	cond  = Zmem(ccond); ! Condition byte

	while (cond ~= 255)
	{
		if (cond <= 12) ++ccond;
		else if (cond <= 15) ccond = ccond + 2;

		++ccond;
		cond = Zmem(ccond);
	} 
	return ccond;
];



[EvalCondition ccond cond ctrue location param param2;

	ctrue = 1;
	cond  = Zmem(ccond); ! Condition byte
	location = QuillFlags->35;

	while ((cond ~= 255) && (ctrue ~= 0))
	{
		++ccond;
		param = Zmem(ccond);
		
		switch (cond)
		{
			0: ! AT
				ctrue = (param == location);
			1: ! NOTAT
				ctrue = (param ~= location);
			2: ! ATGT
				ctrue = (param < location);
			3: ! ATLT
				ctrue = (param > location);
			4: ! PRESENT
				ctrue = Present(param);		
			5: ! ABSENT
				ctrue = (Present(param) == 0);
			6: ! WORN
				ctrue = (ObjectPos->param == 253);
			7: ! NOTWORN
				ctrue = (ObjectPos->param ~= 253);
			8: ! CARRIED
				ctrue = (ObjectPos->param == 254);
			9: ! NOTCARR
				ctrue = (ObjectPos->param ~= 254);
			10: ! CHANCE
				ctrue = (random(100) - 1) < param;
			11: ! ZERO
				ctrue = (QuillFlags->param == 0);
			12: ! NOTZERO
				ctrue = (QuillFlags->param ~= 0);
			13: ! EQ
				++ccond;
				param2 = Zmem(ccond);
				ctrue = (QuillFlags->param == param2);
			14: ! GT
				++ccond;
				param2 = Zmem(ccond);
				ctrue = (QuillFlags->param > param2);
			15: ! LT
				++ccond;
				param2 = Zmem(ccond);
				ctrue = (QuillFlags->param < param2);
			default:
				print "Unknown condition number ", cond, "!^";
		}
		++ccond;
		cond = Zmem(ccond);
	} 
	return ctrue;
];

[ AutoObj n m;

	if (Options->1 == 0) return 255;
	if (n > 253)         return 255;
	for (m = 0: m < ObjectCount: m++) if (n == Zmem(ObjectWords + m)) return m;	
	return 255;
];

[GetObj ob d;
	if (options->0 == 0) d = 0;
	else                 d = 3;

	if (ob            == 255) { SysMess(8);    PutChar(13); rtrue; }	
	if (ObjectPos->ob == 254) { SysMess(22+d); PutChar(13); rtrue; }
	if (ObjectPos->ob == 253) { SysMess(26+d); PutChar(13); rtrue; }
	if (ObjectPos->ob ~= QuillFlags->35)
				  { SysMess(23+d); PutChar(13); rtrue; }
	if (QuillFlags->1 == GameObjectLimit)
				  { SysMess(21+d); PutChar(13); rtrue; }

	ObjectPos->ob = 254;
	QuillFlags->1 = QuillFlags->1 + 1;
	PutChar(13);
	rfalse; 
];

[DropObj ob d;
	if (options->0 == 0) d = 0;
	else                 d = 3;
	if (ob            == 255) { SysMess(8);   PutChar(13); rtrue; }
	if (ObjectPos->ob ~= 254 &&
	    ObjectPos->ob ~= 253) { SysMess(25+d);PutChar(13); rtrue; }
	ObjectPos->ob = QuillFlags->35;
	QuillFlags->1 = QuillFlags->1 - 1;
	PutChar(13);
	rfalse; 	
];


[WearObj ob d;
        if (options->0 == 0) d = 0;
        else                 d = 3;
!        if (ob            == 255) { SysMess(8);   PutChar(13); rtrue; }
        if (ObjectPos->ob ~= 254) { SysMess(25+d);PutChar(13); rtrue; }
        ObjectPos->ob = 253;
        QuillFlags->1 = QuillFlags->1 - 1;
        PutChar(13);
        rfalse;
];




[RemoveObj ob d;
        if (options->0 == 0) d = 0;
        else                 d = 3;
!        if (ob            == 255) { SysMess(8);   PutChar(13); rtrue; }
        if (ObjectPos->ob ~= 253) { SysMess(20+d); PutChar(13); rtrue; }
        if (QuillFlags->1 == GameObjectLimit)
                                  { SysMess(21+d); PutChar(13); rtrue; }
	
        ObjectPos->ob = 254;
        QuillFlags->1 = QuillFlags->1 + 1;
        PutChar(13);
        rfalse;
];

[Tick;			! Pause timeout
	rtrue;
];

[Pause x y z;

	z = 0;

	Flush();

	if (Options->1 ~= 0 && QuillFlags->28 > 0 && QuillFlags->28 < 23)
	{
		z = 1;
		switch(QuillFlags->28)
		{
!
! These effects require Spectrum hardware
!
			1,2,3,5,6:	! Sound effects
			16,17,18:	! Keyboard click
			4:		! Screen flicker
			7,8:		! Font 1,2
			19:		! Graphics on/off
			20:		! No-op

			9: 		! Clear screen impressively
				Cls();
			10:		! Set AlsoSee
				if (x < SysCount) AlsoSee = x;
			11: 		! Set carrying limit
				GameObjectLimit = x;
			12: 		! Restart game
				AlsoSee = 1;
				FileID  = 255;
				GameObjectLimit = ObjectLimit;
				return 4;
			13: 		! Reboot Spectrum
				Flush();
				print "Z-machine terminated!^";
				@quit;
			14:		! Increase object limit
				if ((255 - x) >= GameObjectLimit)
					GameObjectLimit = GameObjectLimit + x;
				else    GameObjectLimit = 255;
			15: 		! Decrease object limit
				if (GameObjectLimit < x) GameObjectLimit = 0;
				else	GameObjectLimit = GameObjectLimit - x;
			21:		! RAM save/load	
				if (x == 50) RamLoad();
				else	     RamSave();
			22:		! Change identity byte
				FileID = x;
			default:
				z = 0;
		}
		QuillFlags->28 = 0;
	}
	if (z == 0)
	{
		if (x ~= 0)
		{
			x = x / 5;	! Timeout now in 10ths of a second
			if (x == 0) x = 1;
			@read_char 1 x Tick -> y;
				! If terp doesn't do timed events, all 
				! pauses are PAUSE 0
		}
		else	@read_char 1 -> y;
	}


];

	

[RunAction ccond cact cdone cobj nout n param param2;

	cdone = 0;
	cact = Zmem(ccond); ! Action byte

	while (cact ~= 255 && cdone == 0)
	{
! Translate numbers for old-style games
		if (Options->1 == 0)
		{
			if (cact >  11) cact = cact + 9;
			if (cact == 11) cact = 17;
			if (cact  > 29) cact = cact + 1;
		}
		if (Options->1 == 5)
		{
			if (cact >= 13) cact = cact + 4;
		}
		if (cact >= 17)
		{
			++ccond;
			param = Zmem(ccond);
		}
		if (cact >= 33 || cact == 29 || cact == 30)
		{
			++ccond;
			param2 = Zmem(ccond);
		}
		if (Options->5 == CPC && cact == 19) ! INK 
		{
			++ccond;
			param2 = Zmem(ccond);
		}
		switch(cact)
		{
			0: ! INVEN
				nout = 0;
				PutChar(13);
				SysMess(9);
				PutChar(13);
				for (n = 0: n < ObjectCount: n++)
				{
				    if (ObjectPos->n == 254)
				    {
					TablePrint(Objecttab, n);
					PutChar(13);
					++nout;
				    }
	                            if (ObjectPos->n == 253)
                                    {
                                        TablePrint(Objecttab, n);
					SysMess(10);	! "Worn" 
                                        PutChar(13);
                                        ++nout;
                                    }
				}
				if (nout == 0) SysMess(11);
				PutChar(13);
			1: ! DESC
				cdone = 2;
			2: ! QUIT
				SysMess(12);
				if (YesNo() == 'N') cdone = 1;
			3: ! END
				cdone = 3;		
			4: ! DONE
				cdone = 1;
			5: ! OK
				cdone = 1;
				SysMess(15);
				PutChar(13);
			6: ! ANYKEY
			        SysMess(16);
				Flush();
				@read_char 1 n;
				PutChar(13);
			7: ! SAVE
!
! XXX This does a Z-machine standard save. While this is quicker to program
!    and more likely to work, it does mean that multipart games in which
!    you do a save-and-restore to get from one part to the next, won't work.
!
				@save -> nout;
				cdone = 2;

			8: ! LOAD
				@restore -> nout;
				cdone = 2;	
			9: ! TURNS
				SysMess(17);
				nout = QuillFlags->31 + 256 * QuillFlags->32;
				Flush();
				print (number) nout;
				SysMess(18);
				if (nout ~= 1)
				{
					if (Options->1 ~= 0) SysMess(19);
					else PutChar('s');
				}
				if (Options->1 ~= 0) SysMess(20);
				else PutChar('.');
				PutChar(13);
			10: ! SCORE
				if (Options->1 ~= 0) 
				{
					SysMess(21);
					Flush();
					print (number) QuillFlags->30;
					SysMess(22);
				}	
				else
				{
					SysMess(19);
					Flush();
					print (number) QuillFlags->30;
					PutChar('.');
				}
				PutChar(13);
			11: ! CLS
				Cls();
			12: ! DROPALL
				for (cobj = 0: cobj < ObjectCount: cobj++)
				{
				    if (ObjectPos->cobj == 254 || ObjectPos->cobj == 253)
				    {
					ObjectPos->cobj = QuillFlags->35;
				    }
				}
				QuillFlags->1 = 0;
				PutChar(13);
			13: ! AUTOG
				cobj  = AutoObj(QuillNoun);
				if (cobj == 255) { SysMess(8); cdone = 1; }
				else cdone = GetObj(cobj);
			14: ! AUTO
				cobj  = AutoObj(QuillNoun);
				if (cobj == 255) { SysMess(8); cdone = 1; }
				else cdone = DropObj(cobj);
			15: ! AUTOW 
				if (QuillNoun < 200)
				{
					SysMess(8); 
					cdone = 1;
				}
				else
				{
					cobj  = AutoObj(QuillNoun);
					if (cobj == 255) 
					{
						SysMess(8); cdone = 1;	
					}
					else cdone = WearObj(cobj);
				}
			16: ! AUTOR
				if (QuillNoun < 200)
				{
					SysMess(8);
					cdone = 1;
				}
				else
				{
					cobj  = AutoObj(QuillNoun);
					if (cobj == 255) 
					{
						SysMess(8);
						cdone = 1;
					}
					else cdone = RemoveObj(cobj);
				}
			17: ! PAUSE
				Pause(param);
			18: ! PAPER
				Paper(param);
			19: ! INK
				Ink(param);
			20: ! BORDER, ignored

			21: ! GOTO
				QuillFlags->35 = param;
			22: ! MESSAGE
				TablePrint(Messages, param);
				PutChar(13);
			23: ! REMOVE
				cdone = RemoveObj(param);
			24: ! GET
				cdone = GetObj(param);
			25: ! DROP
				cdone = DropObj(param);
			26: ! WEAR
				cdone = WearObj(param);
			27: ! DESTROY
				if (ObjectPos->param == 254) 
					{ QuillFlags->1 = QuillFlags->1 - 1; }
				ObjectPos->param = 252;
			28: ! CREATE
				ObjectPos->param = QuillFlags->35;
			29: ! SWAP
				cobj = ObjectPos->param;
				ObjectPos->param = ObjectPos->param2;
				ObjectPos->param2 = cobj;
			30: ! PLACE
				if (ObjectPos->param == 254)
                                        { QuillFlags->1 = QuillFlags->1 - 1; }
				ObjectPos->param = param2;
			31: ! SET
				QuillFlags->param = 255;
			32: ! CLEAR
				QuillFlags->param = 0;
			33: ! PLUS
				if (QuillFlags->param + param2 <= 255)
					QuillFlags->param = QuillFlags->param + param2;
				else	QuillFlags->param = 255; ! [9-10-2015] Clamp at 255
			34: ! MINUS
				if (QuillFlags->param < param2)	! [9-10-2015] Clamp at 0
					QuillFlags->param = 0;
				else	QuillFlags->param = QuillFlags->param - param2;
			35: ! LET
				QuillFlags->param = param2;	
			36: ! BEEP
				if (Options->4 == 0)
				{
					@sound_effect 1;
				}
			default: print "Unknown action ", cact, "!^";
		}
                ++ccond;
                cact = Zmem(ccond);
	}
	return cdone;
];




[RunTable table done ccond tverb tnoun t td1;

	done = 0;
	td1 = 0;

	while (Zmem(table) ~= 0 && done == 0)
	{
		tverb = Zmem(table);  ++table;
		tnoun = Zmem(table);  ++table;
		ccond = PeekW(table); table = table + 2;

		if (ffeq(tverb, QuillVerb) && ffeq(tnoun, QuillNoun))
		{
			t = EvalCondition(ccond);
!
! Skip over the conditions that EvalCondition just looked at.
! [0.9.0] This does not work because 255 can appear in the argument of 
! a function.	
! 			while (Zmem(ccond) ~= 255) { ++ccond; }
			ccond = SkipCondition(ccond);

			++ccond;
			if (t ~= 0)
			{
				done = RunAction(ccond);
				td1 = 1;
!
! td1 is set if an action part was run.
!
			}
		}

	}	
	if ((done == 0) && (td1 ~= 0)) done = 1;
	return done;
];

