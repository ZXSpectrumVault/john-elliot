/************************************************************************

    JOYCE v2.1.6 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2,2005  John Elliott <seasip.webmaster@gmail.com>

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

#ifdef HAVE_WINDOWS_H
#include <SDL/SDL_thread.h>

void ExpandError(DWORD err)
{
     LPTSTR s;
     if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			0,
			(LPTSTR)&s,
			0,
			NULL) == 0)
	{ 
		joyce_dprintf("Unknown error code %08x (%d)\n", err, 
			LOWORD(err));
	} 
	else
	{
		LPTSTR p = strrchr(s, _T('\r'));
		if(p != NULL)
	   	{
	     		*p = 0;
		}
		joyce_dprintf("Windows error %08x: %s\n", err, s);
		::LocalFree(s);
	}
}



//

//
// Win32 implementation of the abstract PcwComms class
//
class PcwCommsW32: public PcwComms
{
public:
	PcwCommsW32();
	virtual ~PcwCommsW32();
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
	void setParams(DCB *pDCB, int nbits);
	void setAllParams(void);
	void resetComm(void);
	void dump_readbuf(void);
	bool issue_read(void);
	HANDLE openComm(const char *port);
	HANDLE openNUL(int f = 0);

	HANDLE m_hFileIn, m_hFileOut;
	int m_txbits, m_rxbits, m_stopBits;
	LL_PARITY m_parity;
	int m_txbaud, m_rxbaud;
	int m_wrpend;
	char m_wrpch;
	char m_readbuf[1024];
	int  m_readbuf_r, m_readbuf_w;

	int m_txComm, m_rxComm;
	OVERLAPPED m_ovRead, m_ovWrite;
	SDL_Thread *m_thReader;

	HANDLE m_events[5];
public:
	bool m_term;
	int readThread(void);
	bool startRead(void);
};

#define EV_START 0
#define EV_STOP  1
#define EV_QUIT  2
#define EV_READ  3
#define EV_WRITE 4


static int readThread(void *data)
{
	PcwCommsW32 *pc = (PcwCommsW32 *)data;
	return pc->readThread();
}



int PcwCommsW32::readThread()
{
	BOOL b;
	DWORD nr, timeout, dw;
	bool pending = false;
	bool open = false;
	int e;
	int objcount = EV_QUIT + 1;

	while (!m_term)
	{
		timeout = INFINITE;
		if (open == true && pending == false) 
		{
			pending = startRead();
//
// If no read is pending, then just do a quick poll of the other events
// and try to launch another read.
//
			if (!pending) timeout = 0;
		}
		dw = WaitForMultipleObjects(objcount, m_events, FALSE, timeout);

		switch(dw)
		{
			case WAIT_OBJECT_0:	// Comm has opened
				objcount = EV_READ + 1;
				open = true;	
//				fprintf(stderr, "START READ\n");
				break;
			case WAIT_OBJECT_0 + 1:	// Comm has closed
				objcount = EV_QUIT + 1;
				open = false;
//				fprintf(stderr, "STOP READ\n");
				break;
			case WAIT_OBJECT_0 + 2:	// Shutting down
//				fprintf(stderr, "SHUTDOWN\n");
				return 0;
			case WAIT_OBJECT_0 + 3:	// Data read
				if (!GetOverlappedResult(m_hFileIn, &m_ovRead, &nr, FALSE))
       				{
					e = GetLastError();
					/* if (e == ERROR_IO_INCOMPLETE)
					 * {
					 *  //	fprintf(stderr, "GOVL ERR %d %d\n", e, nr);
					 * } 
					 * else */ 
					if (e != ERROR_IO_PENDING)
					{
//						fprintf(stderr, "GOVL ERR %d pending:=false\n", e);
						pending = false;
					}
//					else	// ERROR_IO_PENDING
//					{
//						fprintf(stderr, "GOVL PENDING\n", e);
//					}
					break;
				}
				else ResetEvent(m_events[EV_READ]);
				// If we got data, process them.
				if (nr) 
				{
//					fprintf(stderr, "GOVL GOT %x\n", m_readbuf[m_readbuf_w]);
					++m_readbuf_w;
					if (m_readbuf_w >= sizeof(m_readbuf)) m_readbuf_w = 0;
				}
//				else	fprintf(stderr, "GOVL 000 %x\n", m_readbuf[m_readbuf_w]);
				// Read has finished.
				pending = false;
//				fprintf(stderr, "pending:=false\n");
				break;
			}	// end switch
	}	// End while
	return 0;
}

bool PcwCommsW32::startRead(void)
{
	BOOL b;
	DWORD nr;
	
	// Read handle is open...
	b = ::ReadFile(m_hFileIn, m_readbuf + m_readbuf_w, 1, &nr, &m_ovRead);
	int e = GetLastError();
			
	if (b)
	{
		ResetEvent(m_events[EV_READ]);
		if (nr) 
		{
//			fprintf(stderr, "RDFL GOT %x\n", m_readbuf[m_readbuf_w]);
			++m_readbuf_w;
			if (m_readbuf_w >= sizeof(m_readbuf)) m_readbuf_w = 0;
		}
//		fprintf(stderr, "startRead=%d e=%d ret 0\n", b, e);
		return false; // No read pending.
	}
	else 
	{
		if (e == ERROR_IO_PENDING) 
		{
//			fprintf(stderr, "startRead=%d e=%d ret 1\n", b, e);
			return true;
		}
//		fprintf(stderr, "RDFL ERR %d\n", e);	
	}
	return false;
}
//
// Windows comms (like every other aspect of Windows programming)
// is a nightmare. It seems I have two choices: Use "overlapped I/O" 
// on the serial ports, or launch separate threads to handle the reading
// & writing, or both. But only use the overlapped I/O when actually 
// talking to real COM ports...
//

PcwComms *newComms() 
{ 
	XLOG("::newComms()");
	return new PcwCommsW32(); 
} 


PcwComms::PcwComms() 
{
	XLOG("PcwComms::PcwComms()");
	m_infile = ""; 
	m_outfile = ""; 
}


PcwComms::~PcwComms() 
{
} 



PcwCommsW32::PcwCommsW32()
{
	// EV_READ and EV_WRITE are manual-reset events; the others are
	// automatic.
	static BOOL evauto[5] = { FALSE, FALSE, FALSE, TRUE, TRUE };

	XLOG("PcwCommsW32::PcwCommsW32()");

	for (int n = 0; n < 5; n++)
	{
		m_events[n] = CreateEvent(NULL, evauto[n], FALSE, NULL);
	}

	m_wrpend = 0;
	m_wrpch  = 0;
	m_readbuf_r = m_readbuf_w = 0;
	memset(m_readbuf, 0, sizeof(m_readbuf));
	m_hFileIn = m_hFileOut = INVALID_HANDLE_VALUE;
	m_txbaud = m_rxbaud = 9600;
	m_txbits = m_rxbits = 8;
	m_stopBits = 2;
	m_parity = PE_NONE;
	m_outfile = "COM1:";
	m_infile = "COM1:";
	m_term = false;
	m_ovRead.hEvent  = m_events[EV_READ];
	m_ovWrite.hEvent = m_events[EV_WRITE];
	m_txComm = m_rxComm = -1;
	m_thReader = SDL_CreateThread(::readThread, this);
}


PcwCommsW32::~PcwCommsW32()
{
	flush();
	m_term = true;

	SetEvent(m_events[EV_QUIT]);	
	SDL_WaitThread(m_thReader, NULL);
	for (int n = 0; n < 5; n++)
	{
		CloseHandle(m_events[n]);
	}
}



void PcwCommsW32::reset(void)
{
	flush();
}

void PcwCommsW32::resetComm(void)
{
	COMMTIMEOUTS to;

	if (m_txComm) PurgeComm(m_hFileOut, PURGE_TXABORT | PURGE_TXCLEAR);
	if (m_rxComm) PurgeComm(m_hFileIn,  PURGE_RXABORT | PURGE_RXCLEAR);

	to.ReadTotalTimeoutMultiplier  = 0;
	to.WriteTotalTimeoutMultiplier = 0;
	to.ReadTotalTimeoutConstant    = 0;	
	to.WriteTotalTimeoutConstant   = 0;	//1000;	
	to.ReadIntervalTimeout         = 0;	//MAXDWORD;

	if (m_rxComm) SetCommTimeouts(m_hFileIn, &to);
	if (m_infile != m_outfile && m_txComm) SetCommTimeouts(m_hFileOut, &to);
}


void PcwCommsW32::flush(void)
{
	if (m_infile == m_outfile)
	{
		if (m_hFileIn != INVALID_HANDLE_VALUE)
			CloseHandle(m_hFileIn);
		m_hFileIn = INVALID_HANDLE_VALUE;
		m_hFileOut = INVALID_HANDLE_VALUE;
	}
	else	
	{
		if (m_hFileIn != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFileIn);
			m_hFileIn = INVALID_HANDLE_VALUE;
		}
		if (m_hFileOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFileOut);
			m_hFileOut = INVALID_HANDLE_VALUE;
		}
	}
	SetEvent(m_events[EV_STOP]);	
	m_txComm = m_rxComm = -1;
}

int baud2windows(int baud)
{
	if (baud <=    55)  return 50;
	if (baud <=    80)  return 75;
	if (baud <=   115)  return CBR_110;
	if (baud <=   140)  return 134;
	if (baud <=   170)  return 150;
	if (baud <=   325)  return CBR_300;
	if (baud <=   625)  return CBR_600;
	if (baud <=  1225)  return CBR_1200; 
	if (baud <=  1825)  return 1800; 
	if (baud <=  2425)  return CBR_2400; 
	if (baud <=  3625)  return 3600;
	if (baud <=  4825)  return CBR_4800; 
	if (baud <=  9625)  return CBR_9600; 
	if (baud <= 19250)  return CBR_19200;
	if (baud <= 38500)  return CBR_38400;
	if (baud <= 57800)  return CBR_57600;
	return CBR_115200;	// About as high as the 8253
			// could theoretically go		
}


int isComm(string s)
{
	char t[_MAX_PATH], *u, *v;
	int n;

	// Render into capitals
	strcpy(t, s.c_str());
	for (n = 0; n < strlen(t); n++) 
		t[n] = toupper(t[n]);	

	// Find the last "COM" in the filename.

	char *c = strrchr(t, 'C');
	if (!c) return 0;	
	if (c[0] != 'C' || c[1] != 'O' ||  c[2] != 'M') return 0;

	if (c[3] <  '1' || c[3] > '9')  return 0;
	if (c[4] !=  0  && c[4] != ':') return 0;
	if (c[4] == ':' && c[5] != 0)   return 0;

	return 1;
}


HANDLE PcwCommsW32::openComm(const char *port)
{
	HANDLE h = CreateFile( port, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL |
                  FILE_FLAG_OVERLAPPED, // overlapped I/O
                  NULL );

	if (h != INVALID_HANDLE_VALUE) 
	{	
		SetEvent(m_events[EV_START]);	
		return h;
	}
	DWORD dwErr = GetLastError();
	joyce_dprintf("Can't open %s: ", port);
	ExpandError(dwErr);
	joyce_dprintf("\n");
	return openNUL(FILE_FLAG_OVERLAPPED);
}



HANDLE PcwCommsW32::openNUL(int f)
{
	int a = FILE_ATTRIBUTE_NORMAL | f;
	HANDLE h;
		
	h = CreateFile( "NUL:", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  a, NULL );
	if (h != INVALID_HANDLE_VALUE) 
	{
		SetEvent(m_events[EV_START]);	
		return h;	
	}

	DWORD dwErr = GetLastError();
	joyce_dprintf("Can't open NUL: ");
	ExpandError(dwErr);
	joyce_dprintf("\n");
	return h;
}

//
// Modes of opening are:
// 1. Input & output to the same COM port
// 2. Input & output to differing files

void PcwCommsW32::openUp(void)
{
//     	DCB dcb;

	if (m_rxComm < 0) m_rxComm = isComm(m_infile);
	if (m_txComm < 0) m_txComm = isComm(m_outfile);

	if (m_rxComm || m_txComm)	// We're using COM ports
	{
		if (m_txComm && !m_rxComm)	m_infile  = m_outfile;
		else				m_outfile = m_infile;
 
		m_rxComm = m_txComm = true;
		m_ovRead.Offset  = m_ovRead.OffsetHigh = 0;
		m_ovWrite.Offset = m_ovWrite.OffsetHigh = 0;
		if (m_hFileIn == INVALID_HANDLE_VALUE) 
		{
			m_hFileIn = openComm(m_infile.c_str());	

			m_hFileOut = m_hFileIn;
			m_outfile = m_infile;	// Force input & output to
					// the same place
			resetComm();
			setAllParams();
			setRX(m_rxbaud);
			setRXbits(m_rxbits);
			setTX(m_txbaud);
			setTXbits(m_txbits);
		}
	}
	else	// We're using files
	{
		if (m_hFileIn == INVALID_HANDLE_VALUE)
			m_hFileIn = CreateFile(m_infile.c_str(), GENERIC_READ,
				       FILE_SHARE_READ | FILE_SHARE_WRITE, 
				       NULL, OPEN_EXISTING,
				       FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFileIn == INVALID_HANDLE_VALUE) 
		{
			DWORD dwErr = GetLastError();
			joyce_dprintf("Can't open %s: ", m_infile.c_str());
			ExpandError(dwErr);
			joyce_dprintf("\n");
			m_hFileIn = openNUL();
		}
		if (m_hFileOut == INVALID_HANDLE_VALUE)
			m_hFileOut = CreateFile(m_outfile.c_str(), GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFileOut == INVALID_HANDLE_VALUE)
		{
			DWORD dwErr = GetLastError();
			joyce_dprintf("Can't open %s: ", m_outfile.c_str());
			ExpandError(dwErr);
			joyce_dprintf("\n");
			m_hFileOut = openNUL();
		}
	}
}


void PcwCommsW32::setParams(DCB *pDCB, int nbits)
{
	pDCB->Parity = NOPARITY; 
	pDCB->fParity = 0;

	if (m_parity == PE_EVEN) { pDCB->fParity = 1; pDCB->Parity = EVENPARITY; }
	if (m_parity == PE_ODD)  { pDCB->fParity = 1; pDCB->Parity = ODDPARITY; }

	if (m_stopBits == 2) pDCB->StopBits = ONESTOPBIT;
	if (m_stopBits == 3) pDCB->StopBits = ONE5STOPBITS;
	if (m_stopBits == 4) pDCB->StopBits = TWOSTOPBITS;

	pDCB->ByteSize = nbits;
	pDCB->fAbortOnError = 0;	
}


BOOL GetCommState_Retry(HANDLE hFile, LPDCB lpDCB)
{
	for (int n = 0; n < 2; n++)
	{
		BOOL result = GetCommState(hFile, lpDCB);
		if (result) return true;

		if (GetLastError() != ERROR_OPERATION_ABORTED)
			return false;

		ClearCommError(hFile, NULL, NULL);
	}
	return false;
}



void PcwCommsW32::setRX(int baud)
{
	DCB dcb;
	m_rxbaud = baud;
	
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	if (!m_rxComm) 
	{
	//	COMM_LOG(( "Can't set RX baud rate: Not a comm port\n"));
		return;
	}
	if (m_hFileIn == INVALID_HANDLE_VALUE)
	{
	//	COMM_LOG(("Couldn't set baud rate: invalid handle\n"));
		return;
	}
	if (GetCommState_Retry(m_hFileIn, &dcb))
	{	
		dcb.BaudRate = baud2windows(baud);
	//	COMM_LOG(( "Set RX baud rate to %d\n", dcb.BaudRate));
	        SetCommState(m_hFileIn, &dcb);
	}
	else
	{
		DWORD dwErr = GetLastError(); 
		joyce_dprintf("GetCommState() failed: ");
		ExpandError(dwErr);
		joyce_dprintf("\n");
	}
	PurgeComm(m_hFileIn,  PURGE_RXABORT | PURGE_RXCLEAR);
}


void PcwCommsW32::setTX(int baud)
{
	DCB dcb;
	m_txbaud = baud;
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	if (!m_txComm) 
	{
//		COMM_LOG(( "Can't set TX baud rate: Not a comm port\n"));
		return;
	}
	if (m_hFileOut == INVALID_HANDLE_VALUE)
	{
//		COMM_LOG(("Couldn't set baud rate: invalid handle\n"));
		return;
	}
	if (GetCommState_Retry(m_hFileOut, &dcb))
	{	
		dcb.BaudRate = baud2windows(baud);
//		COMM_LOG(( "Set TX baud rate to %d\n", dcb.BaudRate));
	        SetCommState(m_hFileOut, &dcb);
	}
	else
	{
		DWORD dwErr = GetLastError();
		joyce_dprintf("GetCommState() failed: ");
		ExpandError(dwErr);
		joyce_dprintf("\n");
	}
	PurgeComm(m_hFileOut,  PURGE_TXABORT | PURGE_TXCLEAR);
}



void PcwCommsW32::setStop(int n)
{
	m_stopBits = n;
	setAllParams();
}


void PcwCommsW32::setParity(LL_PARITY p)
{
	m_parity = p;
	setAllParams();
}


void PcwCommsW32::setAllParams(void)
{
	DCB dcb;

	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	if (m_rxComm && m_hFileIn != INVALID_HANDLE_VALUE && GetCommState_Retry(m_hFileIn, &dcb))
	{	
        	setParams(&dcb, m_rxbits);
	        SetCommState(m_hFileIn, &dcb);
	}
	if (m_txComm && m_hFileOut != INVALID_HANDLE_VALUE && GetCommState_Retry(m_hFileOut, &dcb))
	{	
        	setParams(&dcb, m_txbits);
	        SetCommState(m_hFileOut, &dcb);
	}
}


void PcwCommsW32::setTXbits(int b)
{
	DCB dcb;
	m_txbits = b;
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	if (m_txComm && m_hFileOut != INVALID_HANDLE_VALUE && GetCommState_Retry(m_hFileOut, &dcb))
	{	
        	setParams(&dcb, m_txbits);
	        SetCommState(m_hFileOut, &dcb);
	}
}


void PcwCommsW32::setRXbits(int b)
{
	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	m_rxbits = b;
	if (m_rxComm && m_hFileIn != INVALID_HANDLE_VALUE && GetCommState_Retry(m_hFileIn, &dcb))
	{	
        	setParams(&dcb, m_rxbits);
	        SetCommState(m_hFileIn, &dcb);
	}
}


	
void PcwCommsW32::write(char c)
{
	BOOL b;
	DWORD nr;

	openUp();

	m_wrpch = c;
	if (!m_txComm)
	{
		::WriteFile(m_hFileOut, &m_wrpch, 1, &nr, NULL);
		return;
	}
	b = ::WriteFile(m_hFileOut, &m_wrpch, 1, &nr, &m_ovWrite);

	if (b) return;		// it worked

	DWORD dwErr = GetLastError();
	if (dwErr == ERROR_IO_PENDING) 
	{
		// waiting for results of write
		m_wrpend = true;
	}
	else 
	{
		joyce_dprintf("PcwCommsW32::write() ");
		ExpandError(dwErr);
		joyce_dprintf("\n");
	}
}



char PcwCommsW32::read(void)
{
	char c;

        openUp();

	if (!m_rxComm)
	{
		DWORD nr;
		BOOL b;
		b = ::ReadFile(m_hFileIn, &m_readbuf[0], 1, &nr, NULL);
		return m_readbuf[0];
	}
/* See JoyceCommsUnix.cxx for the full story; but in brief, this will
 * hang the emulator if Trivial Pursuit tries reading the Spectravideo 
 * joystick port while a CPS8256 is attached. */
/*
        while (m_readbuf_r == m_readbuf_w) 
        {
                Sleep(50);
        } */
	if (m_readbuf_r == m_readbuf_w)
	{
		return 0xFF;
	}
	c = m_readbuf[m_readbuf_r++];
	//fprintf(stderr, "Comm: read %02x %c \n", c, c);
	if (m_readbuf_r >= sizeof(m_readbuf)) m_readbuf_r = 0;
	return c;
}




bool PcwCommsW32::getTxBufferEmpty(void)
{
	BOOL b;
	DWORD nr;
	OVERLAPPED *o = &m_ovWrite;
	HANDLE h;

	openUp();
	// Nothing is being written
	if (!m_wrpend) return true;
	// Not using overlapped I/O
	if (!m_txComm) return true;

	// Something is going on...
	if (m_infile == m_outfile)  h = m_hFileIn;  
	else			    h = m_hFileOut; 
	
	if (GetOverlappedResult(h, o, &nr, FALSE))	// it finished!
	{
		m_wrpend = false;
		return true;
	}
	return false;
}


bool PcwCommsW32::getRxCharacterAvailable(void)
{
        openUp();
	if (!m_rxComm) return true;	// Reading from a file
        if (m_readbuf_r != m_readbuf_w) return true;
	return false;
}



//
// CTS etc.
//
bool PcwCommsW32::getCTS(void)
{
	if (m_rxComm)
	{
		DWORD stat;
		if (GetCommModemStatus(m_hFileIn, &stat))
			return (stat & MS_CTS_ON);
	}
	return getTxBufferEmpty();
}

bool PcwCommsW32::getAllSent(void)
{
	return getTxBufferEmpty();
}

void PcwCommsW32::setDTR(bool b) 
{ 
	if (m_rxComm)
		EscapeCommFunction(m_hFileIn, b ? SETDTR : CLRDTR);
}


void PcwCommsW32::setRTS(bool b) 
{ 
	if (m_txComm)
		EscapeCommFunction(m_hFileOut, b ? SETRTS : CLRRTS);
}

void PcwCommsW32::sendBreak(void)
{
	openUp();

	if (!m_txComm) return;
	SetCommBreak(m_hFileOut);
	Sleep(500);	
	ClearCommBreak(m_hFileOut);
}


#endif
