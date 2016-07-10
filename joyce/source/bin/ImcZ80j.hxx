
/* C++ Z80 emulation, based on Ian Collier's xz80 v0.1d
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


class ImcZ80 : public Z80
{
public:
   byte *memptr;

   int frames;
   int int_line, nmi_line;	// INT and NMI lines
   int m_cycles;		// Cycles per frame 

/*#ifdef DEBUG
	word breakpoint=0;
	unsigned int breaks=0;

	inline void log(FILE *fp, char *name, word val)
	{
		int i;
		fprintf(fp,"%s=%04X ",name,val);
		for(i=0;i<8;i++,val++)fprintf(fp," %02X",fetch(val));
		fputc('\n',fp);
	}
#endif*/
	virtual void mainloop(word initpc = 0, word initsp = 0);
	virtual void reset(void);
	virtual void Int(int status);
	virtual void Nmi(int status);

        virtual void main_every(void);
	virtual int interrupt(void) = 0;
/* Output "val" to the I/O port whose high address byte is "hi" and
 * whose low address byte is "lo".  The output occurred "time" clock
 * cycles after the start of the current frame.  The result from the
 * function is the number of wait states introduced by the I/O device.*/

	virtual unsigned int out(unsigned long time,
				 byte hi, byte lo,
				 byte val) = 0;
/* Input from the I/O port whose high address byte is "hi" and whose
 * low address byte is "lo".  The input occurred "time" clock cycles
 * after the start of the current frame.  The result from the function
 * is the 8-bit unsigned input from the port, plus (t<<8) where t is
 * the number of wait states introduced by the I/O device. */
	virtual unsigned int in(unsigned long time,
			        unsigned long hi, unsigned long lo) = 0;


	virtual void EDFE(void) = 0;
	virtual void EDFD(void) = 0;
	virtual void startwatch(int flag) = 0;
	virtual long stopwatch(void) = 0;
};

extern byte *PCWRD[], *PCWWR[];	
/*
 * extern byte snapa,snapf,snapb,snapc,snapd,snape,snaph,snapl,
             snapa1,snapf1,snapb1,snapc1,snapd1,snape1,snaph1,snapl1,
             snapi,snapr,snapiff1,snapiff2,snapim;
extern word snapix,snapiy,snapsp,snappc;
extern int load_snap;
extern char snapname[];
extern char snapsavename[];
extern int hsize,vsize;
extern byte keyports[9];
extern int bordercr;
extern int borderchange;
extern int soundfd;
*/

inline byte fetch (word x) { return PCWRD[(word)(x)>>14][x]; }
inline word fetch2(word x) { return ((fetch((x)+1)<<8)|fetch(x));    }
inline void store(word x, byte y) { (PCWWR[(word)(x)>>14][x]) = y; }
inline void store2b(word x, byte hi, byte lo)
{
	(PCWWR[(word)(x)>>14][x]) = lo;
	(PCWWR[(word)(x+1)>>14][(x+1)&65535]) = hi;
}
inline void store2(word x, word y) { store2b(x, (y)>>8, y & 0xFF); }


