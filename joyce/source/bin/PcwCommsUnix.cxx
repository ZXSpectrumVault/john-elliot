/************************************************************************

    JOYCE v2.1.6 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2005  John Elliott <seasip.webmaster@gmail.com>

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

#include "Pcw.hxx"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#include <sys/time.h>
#include <errno.h>

//
// UNIX implementation of the abstract PcwComms class
//
class PcwCommsUnix: public PcwComms
{
public:
	PcwCommsUnix();
	virtual ~PcwCommsUnix();
        virtual void flush(void);
        virtual void reset(void);

        // For transmission
	virtual bool getTxBufferEmpty(void);
	virtual bool getCTS(void);
	virtual bool getAllSent(void);
        virtual void setDTR(bool b);
        virtual void setRTS(bool b);
        virtual void sendBreak(void);

	// For reception
        virtual bool getRxCharacterAvailable(void);

	// Framing
	virtual void setTX(int baud);
	virtual void setRX(int baud);
	virtual void setTXbits(int n);
	virtual void setRXbits(int n);
	virtual void setStop(int n);
	virtual void setParity(LL_PARITY p);

	virtual void write(char c);
	virtual char read(void);

private:
	void openUp(void);
	void setParams(struct termios *termios_p, int nbits);
	void setAllParams(void);

	int m_fdin, m_fdout;
	int m_txbits, m_rxbits, m_stopBits;
	LL_PARITY m_parity;
	int m_txbaud, m_rxbaud;
	int m_rdpend, m_wrpend;
	char m_wrpch, m_rdpch;
};

PcwComms *newComms() { return new PcwCommsUnix(); } 


PcwComms::PcwComms() { m_infile = ""; m_outfile = ""; }
PcwComms::~PcwComms() {} 

PcwCommsUnix::PcwCommsUnix()
{
	m_rdpend = m_wrpend = 0;
	m_rdpch  = m_wrpch  = 0;
	m_fdin = m_fdout = -1;
	m_txbaud = m_rxbaud = 9600;
	m_txbits = m_rxbits = 8;
	m_stopBits = 2;
	m_parity = PE_NONE;
	m_outfile = "/dev/ttyS0";
	m_infile = "/dev/ttyS0";
}


PcwCommsUnix::~PcwCommsUnix()
{
	flush();
}



void PcwCommsUnix::reset(void)
{
	flush();
}


void PcwCommsUnix::flush(void)
{
	if (m_fdin >= 0)
	{
		close(m_fdin);
		m_fdin = -1;
	}

	if (m_fdout >= 0)
	{
		close(m_fdout);
		m_fdout = -1;
	}
}


void PcwCommsUnix::openUp(void)
{
        struct termios t;
	if (m_fdin < 0)
	{
		m_fdin = open(m_infile.c_str(), O_RDONLY | O_NONBLOCK);
		if (m_fdin < 0) joyce_dprintf("Can't open %s: %s\n", m_infile.c_str(), strerror(errno));
		if (m_fdin < 0) m_fdin = open("/dev/zero", O_RDONLY);
		setRX(m_rxbaud);	
        	tcgetattr(m_fdin, &t);
	        setParams(&t, m_rxbits);
       		tcsetattr(m_fdout, TCSADRAIN, &t); /* XXX Why is this using 
						      m_fd*out*? Still, it 
						      seems to work so I'm not
						      changing it */
	}

	if (m_fdout < 0)
	{
		m_fdout =open(m_outfile.c_str(), O_CREAT | O_WRONLY | O_APPEND | O_NONBLOCK, S_IRUSR | S_IWUSR);
		if (m_fdout < 0) joyce_dprintf("Can't open %s: %s\n", m_outfile.c_str(), strerror(errno));
		if (m_fdout < 0) m_fdout = open("/dev/null", O_WRONLY | O_APPEND | O_NDELAY);
		setTX(m_txbaud);
        	tcgetattr(m_fdout, &t);
	        setParams(&t, m_txbits);
       		tcsetattr(m_fdout, TCSADRAIN, &t);
	}
}


void PcwCommsUnix::setParams(struct termios *termios_p, int nbits)
{                                                              
	termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR| 
                                        IGNCR|ICRNL|IXON);      
	termios_p->c_oflag &= ~OPOST;                              
	termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);   
	termios_p->c_cflag &= ~(CSIZE|PARENB);                     

	if (m_parity == PE_EVEN) termios_p->c_cflag |= PARENB;                     
	if (m_parity == PE_ODD)  termios_p->c_cflag |= PARODD;                     

	if (m_stopBits == 2) termios_p->c_cflag &= ~CSTOPB;	// 1 stop bit
	else		     termios_p->c_cflag |= CSTOPB;	// 1.5 or 2

	switch(nbits)
	{
   		case 5:  (termios_p)->c_cflag |=  CS5; break;
   		case 6:  (termios_p)->c_cflag |=  CS6; break;
   		case 7:  (termios_p)->c_cflag |=  CS7; break;
   		default: (termios_p)->c_cflag |=  CS8; break;
	}
}



// Convert a baud rate to a TERMIOS baud flag. We allow some variance
// because the calculation is not exact.
static int baud2termios(int baud)
{
	if (baud <=    55)  return B50;
	if (baud <=    80)  return B75;
	if (baud <=   115)  return B110;
	if (baud <=   140)  return B134;
	if (baud <=   170)  return B150;
	if (baud <=   325)  return B300;
	if (baud <=   625)  return B600;
	if (baud <=  1225)  return B1200; 
	if (baud <=  1825)  return B1800; 
	if (baud <=  2425)  return B2400; 
	if (baud <=  3625)  return B2400;	// TERMIOS doesn't support 3600
	if (baud <=  4825)  return B4800; 
	if (baud <=  9625)  return B9600; 
	if (baud <= 19250)  return B19200;
	if (baud <= 38500)  return B38400;
	if (baud <= 57800)  return B57600;
	return B115200;	// About as high as the 8253
			// could theoretically go		
}


void PcwCommsUnix::setTX(int baud)
{
        struct termios t;

	m_txbaud = baud;
	if (m_fdout < 0) return;
        tcgetattr(m_fdout, &t);
        cfsetispeed(&t, baud2termios(baud));
        tcsetattr(m_fdout, TCSADRAIN, &t);
}


void PcwCommsUnix::setRX(int baud)
{
        struct termios t;

	m_rxbaud = baud;
	if (m_fdin < 0) return; 
        tcgetattr(m_fdin, &t);
        cfsetispeed(&t, baud2termios(baud));
        tcsetattr(m_fdin, TCSADRAIN, &t);
}



void PcwCommsUnix::setStop(int n)
{
	m_stopBits = n;
	setAllParams();
}


void PcwCommsUnix::setParity(LL_PARITY p)
{
	m_parity = p;
	setAllParams();
}


void PcwCommsUnix::setAllParams(void)
{
        struct termios t;
	if (m_fdin >= 0)
	{
	        tcgetattr(m_fdin, &t);
        	setParams(&t, m_rxbits);
	        tcsetattr(m_fdin, TCSADRAIN, &t);
	}
	if (m_fdout >= 0)
	{
	        tcgetattr(m_fdout, &t);
        	setParams(&t, m_txbits);
	        tcsetattr(m_fdout, TCSADRAIN, &t);
	}
}


void PcwCommsUnix::setTXbits(int b)
{
        struct termios t;
	m_txbits = b;

	if (m_fdout < 0) return;
       	tcgetattr(m_fdout, &t);
        setParams(&t, m_txbits);
	tcsetattr(m_fdout, TCSADRAIN, &t);
}


void PcwCommsUnix::setRXbits(int b)
{
        struct termios t;
	m_rxbits = b;

	if (m_fdin < 0) return;
       	tcgetattr(m_fdin, &t);
        setParams(&t, m_rxbits);
	tcsetattr(m_fdin, TCSADRAIN, &t);
}


	
void PcwCommsUnix::write(char c)
{
	openUp();
	
	while (!getTxBufferEmpty())
	{
		usleep(50);
	}

	m_wrpch = c;
	int err = ::write(m_fdout, &m_wrpch, 1);

	if (err == -1 && errno == EAGAIN) m_wrpend = 1;
	else				  m_wrpend = 0;
}

char PcwCommsUnix::read(void)
{
	int delay;
	openUp();

/* [JOYCE 2.1.6] Don't have an infinite while() loop here; it'll hang if 
 * there's really nothing coming down the wire. Trivial Pursuit's probe for
 * the Spectravideo joystick interface really gets this in a twist.
 *
 * We may get on better not retrying at all (in the hope that a real comms 
 * program is only going to read the receive queue when it's checked the 
 * control register and knows there's something there - while a joystick-using
 * app is just going to keep hammering at the port).
 */
//	for (delay = 0; delay < 200; delay++)
//	{	
		if (getRxCharacterAvailable())
		{
			m_rdpend = 0;
			return m_rdpch;
		}
//		usleep(50);
//	}
	return 0xFF;
}



bool PcwCommsUnix::getTxBufferEmpty(void)
{
	openUp();
	if (!isatty(m_fdout)) return true;

	if (!m_wrpend) return true;

	int err = ::write(m_fdout, &m_wrpch, 1);
	if (err == -1 && errno == EAGAIN) return false;

	m_wrpend = false;
	return true; 
}


bool PcwCommsUnix::getRxCharacterAvailable(void)
{
	openUp();
//	if (!isatty(m_fdin)) return true;
	if (m_rdpend) return true;

        int err = ::read(m_fdin, &m_rdpch, 1);
	if (err > 0)	// Character read
	{
		m_rdpend = 1;
		return true;
	}
	m_rdpend = 0;
	return false;
}

//
// Dummy CTS etc.
//
bool PcwCommsUnix::getCTS(void)
{
	return getTxBufferEmpty();
}

bool PcwCommsUnix::getAllSent(void)
{
	return getTxBufferEmpty();
}

void PcwCommsUnix::setDTR(bool b) { }
void PcwCommsUnix::setRTS(bool b) { }

void PcwCommsUnix::sendBreak(void)
{
	openUp();
	tcsendbreak(m_fdout, 0);
}


#endif	// def HAVE_TERMIOS_H
