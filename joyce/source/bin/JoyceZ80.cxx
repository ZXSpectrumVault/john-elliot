/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

*************************************************************************/

#include "Joyce.hxx"
#include "PcwFid.hxx"
#include "PcwBdos.hxx"
#include "inline.hxx"

void ResetPcw(void);
void LoadBoot(void);

#define GRANULARITY   100.0	// Check timing every 100ms
#define G_TICKS	         90	// == GRANULARITY * 90%

/* These functions are used to display all daisywheel I/O */

extern int daisy_int;

/* Hacked-together implementation of the pcwlink interface, just as a proof
 * of concept

static signed char pcwlink_channel = -1;

static void pcwlink_log(const char *s, ...)
{
	char buf[200];
	va_list ap;
	FILE *fp;

	va_start(ap, s);
	vsprintf(buf, s, ap);
	fp = fopen("pcwlink.log", "a");
	fprintf(fp, "%s\n", buf);
	fclose(fp);
	va_end(ap);

}

static void open_pcwlink()
{
	FILE *fp = fopen("pcwlink.ch", "r+b");
	if (!fp)
	{
		pcwlink_channel = 0;
		fp = fopen("pcwlink.ch", "wb");
		fwrite(&pcwlink_channel, 1, 1, fp);
		fclose(fp);
	}
	else
	{
		fread(&pcwlink_channel, 1, 1, fp);
		pcwlink_channel ^= 1;	
		fseek(fp, 0, SEEK_SET);
		fwrite(&pcwlink_channel, 1, 1, fp);
		fclose(fp);
	}
	pcwlink_log("[%d] PCWLink channel: %d", getpid(), pcwlink_channel);
}

static void write_pcwlink(Z80 *r, byte v)
{
	FILE *fp;
	if (pcwlink_channel == -1) open_pcwlink();

	if (pcwlink_channel == 0) fp = fopen("pcwlink.0", "wb");
	else			  fp = fopen("pcwlink.1", "wb");
	fwrite(&v, 1, 1, fp);
	fclose(fp);
	if (r->pc == 0x4167) 
		pcwlink_log("[%d] PCWLink write @ %04x %04x:   %d", getpid(), r->pc, r->iy, v & 1);
	else if (r->pc == 0x4151) 
		pcwlink_log("[%d] PCWLink write @ %04x %04x: %d  ", getpid(), r->pc, r->iy, v & 2);
	else	pcwlink_log("[%d] PCWLink write @ %04x %04x: %d %d", getpid(), r->pc, r->iy, v & 2, v & 1);
}


static byte read_pcwlink(Z80 *r)
{
	byte v;
	FILE *fp;

	if (pcwlink_channel == -1) open_pcwlink();

	if (pcwlink_channel == 0) fp = fopen("pcwlink.1", "rb");
	else			  fp = fopen("pcwlink.0", "rb");

	if (fp == NULL) v = 0xFF;
	else
	{
		fread(&v, 1, 1, fp);
		fclose(fp);
	}
	if (r->pc == 0x4170) 
		pcwlink_log("[%d] PCWLink read  @ %04x %04x: %d  ", getpid(), r->pc, r->iy, (~v) & 2);
	else if (r->pc == 0x4185) 
		pcwlink_log("[%d] PCWLink read  @ %04x %04x:   %d", getpid(), r->pc, r->iy, (~v) & 1);
	else
		pcwlink_log("[%d] PCWLink read  @ %04x %04x: %d %d", getpid(), r->pc, r->iy, (~v) & 2, (~v) & 1);
	
//	return ~(((v << 1) & 0xFE) | ((v >> 1) & 1));
	return ~v;
}
*/

// Variables controlling JOYCE trace code 
int tr = 0;
static int id = 0;
/*
static void daisyIn(byte hi, byte lo, word pc, byte a, byte l, word sp)
{
	int n;

	fprintf(stderr, "DAISY: [%04x] IN %02x%02x a=%02x l=%02x ", pc, hi, lo, a, l);
	for (n = 0; n < 5; n++)		
		fprintf(stderr, "%04x ", fetch2(sp + 2*n));	
	fprintf(stderr,"\n");
}


static void daisyOut(byte hi, byte lo, byte v, word pc)
{
	fprintf(stderr, "DAISY: [%04x] OUT %02x%02x, %02x\n", pc, hi, lo, v);
}
*/

JoyceZ80::JoyceZ80(JoyceSystem *s)
{
	XLOG("JoyceZ80::Z80");
	m_msecs = (int)GRANULARITY;
	m_sys = s;	
}

void JoyceZ80::main_every(void)
{
	if (m_sys->getResetFlag())
	{
		/* set Program Counter to 0000H */
		reset();
		m_sys->reset();
	}
/*
 // Uncomment & modify this code to monitor a memory location
 //
	{
		static word w = 0;
		word f2 = PCWRAM[0x1024] + 256 * PCWRAM[0x1025];
	
		if (w != f2)
		{
			fprintf(stderr, "1024: %04x to %04x\n", w, f2);
			w = f2;
		}
	
*/

/*
	if (pc >= 0x3e88 && pc <= 0x4130 && pcwlink_channel == 0)
		pcwlink_log("[%d] PC=%04x %02x AF=%02x:%02x BC=%04x DE=%04x HL=%04x IX=%04x IY=%04x\n",
			getpid(), pc, fetch(pc), a,f, getBC(), getDE(), getHL(), 
			ix, iy);
*/

 // Uncomment & modify this code for excruciating traces of what the Z80 is
 // doing.
	{

//		if (pc == 0x11c9)	// Expander
//		{
//			if (m_sys->getDebug()) tr = 1;
//		}
//		if (id == 0 && tr == 1) tr = 2;
//		if (id == 500 && tr == 2) tr = 0;
//
//		if (tr == 1 && pc == 0x173) { printf("READ BYTE "); tr = 2; }
//		if (pc == 0x11c9) tr = 2;

//		if (pc == 0xbcee && c == 2) tr = 1;
//		if (pc == 0xbcf1 && tr == 1) tr = 2;

/*
		if (tr == 2)
		{
			++id;
			printf("%d: PC=%04x %02x AF=%02x:%02x BC=%04x DE=%04x HL=%04x IX=%04x IY=%04x\n",
			id, pc, fetch(pc), a,f, getBC(), getDE(), getHL(), 
			ix, iy);
			fflush(stdout);
		}
*/
//
//		if (tr == 2 && pc == 0x5b6d) { printf("=%02x", a); tr = 1; }
		

	}

	m_sys->m_fdc->tick();	// Check for FDC interrupt
	m_sys->m_cps.tick();	// Check for CPS8256 interrupt
	m_sys->m_daisy.tick();	// Check for daisywheel interrupt
	m_sys->m_activeTerminal->tick();	
}

void JoyceZ80::reset(void)
{
	m_avgSpeed = 0;
        m_cenCount = 0;
	m_ticksStarted = 0;
	m_ticksStopped = 0;
	m_ticksElapsed = 0;
	ImcZ80::reset();
}


//
// Support the 0xED 0xFD pseudo-op
//
void JoyceZ80::EDFD()
{
	if (pc < 0xFC12 && pc >= 0xFC0F)     m_sys->m_activePrinter->writeChar(c); // LIST
	if (pc < 0xFC30 && pc >= 0xFC2D) a = m_sys->m_activePrinter->isBusy() ? 0 : 1; // LISTST

	if (pc < 0xFC51 && pc >= 0xFC4E) /* TIME */
	{
		word temp = de;

		setDE(0xFBF4);
		jdos_time(this, 1);
		setDE(temp);
	}
}




/****************************************************************/
/*** Extended JOYCE instructions. This is called on the ED FE ***/
/*** instruction to emulate anything that distinguishes JOYCE ***/
/*** from a real PCW					      ***/
/****************************************************************/

void JoyceZ80::EDFE(void)
{
	switch(a)
	{
		case 0x00:	/* Get JOYCE version */
			a = 0xFF;	/* Utility compatibility number. 
					 * 0  for a real Z80
					 * -1 for Joyce 1.00+ */
			setHL(BCDVERS);	/* JOYCE version */
			setDE('J');	/* Emulator ID */
			break;

		case 0x01:
/*
       A=1:  Boot. If B=0, read sector 1 from floppy drive & enter it;
		   If B>0, read sector 1 from image .\BOOT\BOOTn.DSK and
			   enter it.
*/
			if (m_sys->fdcBoot(b))
			{
				pc = 0xF010;
				sp = 0xF000;
				iff1 = iff2 = 0;
			}
			break;

		case 0x02: 
/*
	A=2: Set colour. BCD = 'white' RGB
			 EHL = 'black' RGB
*/
			m_sys->m_activeTerminal->setForeground(b,c,d);
			m_sys->m_activeTerminal->setBackground(e,h,l);	
			break;

/* 
	A=3 - A=7: Service a call from a disc driver FID

*/
		case 0x03:
			m_sys->m_hdc.fidLogin(this);
			break;
		case 0x04:
			m_sys->m_hdc.fidRead(this);
			break;
		case 0x05:
			m_sys->m_hdc.fidWrite(this);
			break;
		case 0x06:
			m_sys->m_hdc.fidFlush(this);
			break;
		case 0x07:
			m_sys->m_hdc.fidEms(this);
			break;
		case 0x08:		/* A=8: Save settings */
			m_sys->storeHardware();
			m_sys->saveHardware();
			break;
		case 0x09:		/* A=9: Switch which LPT we're using */
			setHL(0); 
			break;
		case 0x0A:		/* A=10: Access DOS filesystem */
			if (jdos(this, m_sys) == -1) goodbye(99);
			break;
		case 0x0B:
			fid_char(this);	/* A=11: Screen control FID */
			break;
		case 0x0C:
			m_sys->m_activeKeyboard->edfe(this);
					/* A=12: low-level keyboard mapping */
			break;
		case 0x0D:
			fid_com(this);	/* (v1.21) A=13: COMPORT.FID */
			break;
		case 0x0E:
			m_sys->m_hdc.fidMessage(this); /* (v1.22) FID disc messages */
			break;
		case 0x0F:		/* (v1.22) LOGO */
			m_sys->m_termLogo.drLogo(this);
			break;
		case 0x10:		/* (v1.22) parse BOOT/BOOT.CFG */
			m_sys->getBootID(this);
			break;
//		case 0x80:		/* Trace on */
//			tr = 2;
//			break;
//		case 0x81:		/* Trace off */
//			tr = 0;	
//			break;
		case 0xFC:		/* (v1.30) CPMREDIR interface */
			cpmredir_intercept(this);
			break;
		case 0xFD:		/* Mouse intercept patch */
			m_sys->m_activeMouse->edfe(this);
			break;
		case 0xFE:		/* Set JOYCE timing parameters */
			setHL(m_cycles);
 			a = ((JoycePcwTerm *)m_sys->m_defaultTerminal)->getRefresh();
			if (c) ((JoycePcwTerm *)m_sys->m_defaultTerminal)->setRefresh(c);
			if (de) m_cycles = de;
			break;
		case 0xFF:		/* Halt & catch fire */
			goodbye(hl);
			break;

		default:		/* Invalid ED xx */
			joyce_dprintf("Unknown operation: ED FE with A=%02x at PC=%04x",
				a, pc);
			break;
	}
	return;
}


/****************************************************************/
/*** Write byte to given IO port.			      ***/
/****************************************************************/
unsigned JoyceZ80::out(unsigned long time,
		  byte hi, byte lo, byte val)
{
	switch(lo & 0xFF)
	{
		case 0: break; /* Floppy disc controller */
		case 1: m_sys->m_fdc->writeData(val); break;

		case 0x84: /* CEN: */
		case 0x85:
		case 0x86:
		case 0x87: m_sys->m_cen.out(lo, val);  break;

		case 0xA2: /* Mouse */
		case 0xA3: m_sys->m_activeMouse->out(lo, val); break;

/*
		case 0xA0: // AY-8910 
		case 0xA1:
		case 0xA2:
		case 0xA3: AYPort(lo, val);
*/

		case 0xE0: case 0xE1: // Z80-DART channel A
		case 0xE2: case 0xE3: // Z80-DART channel B
		case 0xE4: case 0xE5: // CPS8256 timer
		case 0xE6: case 0xE7: // CPS8256 timer
		case 0xE8:	      // CPS8256 parallel
		m_sys->m_cps.out(lo, val);
		break;

		case 0xF0: /* Memory paging */
		case 0xF1:
		case 0xF2:
		case 0xF3: m_sys->m_mem->out(lo, val); break;

		case 0xF4: ((JoyceMemory *)m_sys->m_mem)->setLock(val); break; 

		case 0xF5: /* Video controller */
		case 0xF6:
		case 0xF7: m_sys->m_activeTerminal->out(lo, val); break;

		case 0xF8: // System control
			   ((JoyceAsic *)m_sys->m_asic)->outF8(val); break;

		case 0xFC: 
		case 0xFD: 
			   switch(m_sys->getModel())
			   {
				case PCW8000: case PCW10:
					m_sys->m_matrix.out(lo, val);
					break;
			  	case PCW9000: case PCW9000P: 
			   		m_sys->m_daisy.out(hi*256+lo, val);
			   		//daisyOut(hi, lo, val, pc);
			   		break;
			   }
			   break;
		case 0xFE: m_sys->m_locoLink.out(val);
			   break;
		case 0xFF: //write_pcwlink(this, val);
			   //break;			
		default:
		joyce_dprintf("OUT 0x%02x, 0x%02x at PC=%04x\n", lo, val, pc);
		break;
	}
	return 0;
}

/****************************************************************/
/*** Read byte from given IO port.			      ***/
/****************************************************************/
unsigned JoyceZ80::in(unsigned long time, unsigned long hi, unsigned long lo)
{
	byte value;

/*	if (tr)
	{
		printf("in(%02x)\n", lo);
		fflush(stdout);
	} */
	switch (lo)
	{
		case 0: /* Floppy disc controller */
		return m_sys->m_fdc->readControl();
		case 1:
		return m_sys->m_fdc->readData();

		case 0x84: /* CEN: */
		case 0x85:
		case 0x86:
		case 0x87: return m_sys->m_cen.in(lo & 0xFF); 

		case 0x9F: /* Kempston joystick */
			   if (m_sys->m_joystick.readPort(0x9F, value)) 
				return value;
			   return 0xFF;
// XXX This "DK'Tronics" joystick driver is simplistic and wrong. The 
// DK'Tronics joystick is actually a single register on the DK'Tronics
// sound board (probably an AY-3-8912). It should only be available when
// register 0x0E has been selected (by writing it to port 0xAA). 
//
// However, until JOYCE supports the DK'Tronics sound board, there's
// no harm in leaving this code in for testing purposes. 
		case 0xA9: /* DK'Tronics joystick */
			   if (m_sys->m_joystick.readPort(0xA9, value)) 
				return value;
			   return 0xFF;

		/* CPS8256 */
                case 0xE0:
//
// Z80-DART channel A, or joystick.
// If both are connected, joystick gets priority.
// 
			   if (m_sys->m_joystick.readPort(0xE0, value)) 
				return value;
//
// Not joystick. Fall through to CPS8256.
//
		case 0xE1: 		// Z80-DART channel A
                case 0xE2: case 0xE3:	// Z80-DART channel B
                case 0xE4: case 0xE5:
                case 0xE6: case 0xE7:
                case 0xE8: return m_sys->m_cps.in(lo & 0xFF);

		case 0xF4: return ((JoyceAsic *)m_sys->m_asic)->inF4(); /* Interrupt counter */
		case 0xF8: return ((JoyceAsic *)m_sys->m_asic)->inF8();	/* Odds & ends */

		case 0xFC:	/* Built-in printer */
		case 0xFD:
	        	switch(m_sys->getModel())
			{
				case PCW8000: case PCW10:
					return m_sys->m_matrix.in(lo);
				case PCW9000: case PCW9000P:
			 		//daisyIn(hi, lo, pc, a, l, sp);
			 		return m_sys->m_daisy.in(hi*256 + lo);	
			}
			break;	
		case 0xFE: return m_sys->m_locoLink.in();
			   break;
		case 0xA7: case 0xA6:			/* Lightpen */
		case 0xA0: case 0xA1: case 0xA2:	/* AMX mouse */
		case 0xD0: case 0xD1: case 0xD2:	/* Kempston mouse */
		case 0xD3: case 0xD4: return m_sys->m_activeMouse->in(lo);
		case 0xFF: break;//return read_pcwlink(this);	
	}
	joyce_dprintf("IN(%02x) at PC=%04x\n", lo, pc);
	return 0xFF;
}


void JoyceZ80::startwatch(int flag) 
{ 
	/* If restarted, add up running total */
	if (flag && (m_ticksStopped > m_ticksStarted))
	{
		m_ticksElapsed += (m_ticksStopped - m_ticksStarted);
	} 
	m_ticksStarted = SDL_GetTicks();
	m_ticksStopped = 0;	// Running
}


long JoyceZ80::stopwatch(void) 
{
	m_ticksStopped = SDL_GetTicks(); 
	return 0; 
}

double JoyceZ80::getSpeed(bool requested)
{
	if (requested)
	{
		if (!m_msecs) return 0;
		return ((GRANULARITY * 100.0) / m_msecs);
	} 
	return m_avgSpeed * 100.0;
}


void JoyceZ80::setSpeed(double percent)
{
	if (!percent) 
	{
		m_msecs = 0;	// 0% speed means maximum.
		return;
	}
	double msecs = (GRANULARITY * 100.0) / percent;
	m_msecs = (int)msecs;
}



/****************************************************************/
/*** This function is called at 900Hz. It has to implement    ***/
/*** all PCW features which rely on time.		      ***/
/****************************************************************/
int JoyceZ80::interrupt(void)
{
	if (extern_stop) return Z80_quit;

	++m_cenCount;

	// Handle any waiting SDL events.
	m_sys->eventLoop();

	// Note that these events are all included in startwatch() / 
	// stopwatch() checks. If they are going to do anything time-
	// consuming it is up to them to do their own stopwatch() or 
	// startwatch().
	//
	m_sys->m_activeTerminal->onScreenTimer();
	if (m_sys->getDebug()) m_sys->m_termDebug.onScreenTimer();

	// Poll the keyboard
	m_sys->m_activeKeyboard->poll(this);

	// Do ASICy things
	((JoyceAsic *)m_sys->m_asic)->onAsicTimer();

	if (m_cenCount > 27000) 
	{
		m_cenCount = 0;
		m_sys->m_cen.printerTick();
		m_sys->m_cps.printerTick();
	}
	if ((m_cenCount % G_TICKS) == G_TICKS-1)	// Every second 
	{
		// We should have taken (GRANULARITY)ms since last check. 
		// Calculate if that is so.
		m_ticksStopped = SDL_GetTicks(); 
		if (m_ticksStopped > m_ticksStarted)
		{
			m_ticksElapsed += (m_ticksStopped - m_ticksStarted);
		}	 
		m_ticksStarted = m_ticksStopped;
		m_ticksStopped = 0;	// Running
		if (m_ticksElapsed < m_msecs)
		{
			SDL_Delay(m_msecs - m_ticksElapsed);
		}
		if (m_ticksElapsed)
		{
			m_avgSpeed = (GRANULARITY / m_ticksElapsed);
		}
		else	m_avgSpeed = 9999.99;
		m_ticksElapsed = 0;

	}
	return 0;	/* No interrupt */
}





byte JoyceZ80::fetchB(word addr) { return fetch(addr); }
word JoyceZ80::fetchW(word addr) { return fetch2(addr); }
void JoyceZ80::storeB(word addr, byte v) { store(addr, v); }
void JoyceZ80::storeW(word addr, word v) { store2(addr, v); }

void JoyceZ80::setCycles(int c)
{
	m_cycles = c;
}
