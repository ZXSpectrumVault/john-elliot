
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

/* [JCE] This is the PCW16 version, wherein memory write functions 
 * are different */

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

extern byte *anneMem[4];	
extern byte  anneMemIO[4];

extern void flashStore(byte *addr, byte y);
extern byte flashFetch(byte *addr);

inline byte fetch (word x) 
{ 
	register byte slot = (x >> 14);
	register byte bank = anneMemIO[slot];
	
	if (bank & 0x80) 
	{
		return anneMem[slot][x];
	}
	return flashFetch(anneMem[slot] + x); 
}

inline word fetch2(word x) { return ((fetch((x)+1)<<8)|fetch(x));    }
inline void store(word x, byte y) 
{ 
	register byte slot = (x >> 14);
	register byte bank = anneMemIO[slot];
	
	if (bank & 0x80) 
	{
		(anneMem[slot][x]) = y; 
		return;			
	}
	if (bank < 4) return;	/* Boot ROM */
	flashStore(anneMem[slot] + x, y);
}


inline void store2b(word x, byte hi, byte lo)
{
	store(x,  lo);
	store(x+1,hi);
//	(PCWWR[(word)(x)>>14][x]) = lo;
//	(PCWWR[(word)(x+1)>>14][(x+1)&65535]) = hi;
}
inline void store2(word x, word y) { store2b(x, (y)>>8, y & 0xFF); }


