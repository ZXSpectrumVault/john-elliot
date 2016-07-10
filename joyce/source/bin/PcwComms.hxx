/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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

//
// This class is supposed to define an interface for an abstract comms 
// layer that can control things like serial port speed, parity etc. 
// and transmit/receive data.
//   Under Unix, this would be implemented with <termios.h> calls; under 
// Windows, who knows?
//
class PcwComms
{
protected:
	PcwComms(); 
public:

	string	m_infile, m_outfile;

	virtual void flush(void) = 0;
	virtual void reset(void) = 0;

	// For transmission
	virtual bool getTxBufferEmpty(void) = 0;
	virtual bool getCTS(void) = 0;
	virtual bool getAllSent(void) = 0;

	virtual void sendBreak(void) = 0;

	// For receiving
	virtual bool getRxCharacterAvailable(void) = 0;
	virtual void setDTR(bool b) = 0;
	virtual void setRTS(bool b) = 0;

	// Framing
	virtual void setTXbits(int bits) = 0;
	virtual void setRXbits(int bits) = 0;
	virtual void setStop  (int stops) = 0;
	virtual void setParity(LL_PARITY p) = 0;
	virtual void setTX(int baud) = 0;
	virtual void setRX(int baud) = 0;
	
	virtual void write(char c) = 0;
	virtual char read(void) = 0;

	virtual ~PcwComms();
};

PcwComms *newComms();
