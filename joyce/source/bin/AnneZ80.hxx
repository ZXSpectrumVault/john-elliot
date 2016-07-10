
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

#include "Z80.hxx"
#include "ImcZ80a.hxx"

class AnneSystem;

class AnneZ80 : public ImcZ80
{
public:
	AnneSystem *m_sys;
private:
	Uint32 m_msecs;	// Number of milliseconds that the CPU
			// should take to perform ~80000 instructions.
	Uint32 m_cenCount;	// No. of 900Hz ticks before closing printer
	Uint32 m_ticksStarted;	// SDL tick counter at last startwatch()
	Uint32 m_ticksStopped;	// SDL tick counter at last stopwatch()
	Uint32 m_ticksElapsed;	// SDL ticks elapsed since CPU start
	double m_avgSpeed;	// Average speed of emulation
protected:
        virtual void main_every(void);
	virtual int interrupt(void);
	virtual unsigned int out(unsigned long time,
				 byte hi, byte lo,
				 byte val);
	virtual unsigned int in(unsigned long time,
			        unsigned long hi, unsigned long lo);


	virtual void EDFD(void);
	virtual void EDFE(void);
public:
	AnneZ80(AnneSystem *sys);
	virtual void startwatch(int flag);
	virtual long stopwatch(void);

	virtual void reset(void);

	virtual double getSpeed(bool requested);
	virtual void setSpeed(double percent);

        virtual byte fetchB(word addr);
        virtual word fetchW(word addr);
        virtual void storeB(word addr, byte b);
        virtual void storeW(word addr, word w);
        virtual void setCycles(int cycles);
		
};

