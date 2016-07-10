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

#include "Pcw.hxx"

static byte tone[6] = { 0x80, 0xEE, 0xEE, 
			0x80, 0x11, 0x11 };

PcwBeeper::PcwBeeper()
{
        m_ticks = 0;
        m_wave.recpos = 0;
        m_wave.soundpos = 0;
}

PcwBeeper::~PcwBeeper()
{
        deinitSound();
}

void fillerup(void *userdata, Uint8 *stream, int len)
{
	PcwBeeper *p = (PcwBeeper *)userdata;

	if (p) p->fillerup(stream, len);
}

void PcwBeeper::fillerup(Uint8 *stream, int len)
{
        Uint8 *waveptr;
        int    waveleft, sp;

	waveptr = m_wave.sound;
	// Append the bytes written since last sound output. If the buffer
	// underruns repeat the bytes we did get.
	waveleft = m_wave.recpos;
	if (!waveleft) 
	{
		memset(waveptr, m_wave.spec.silence, m_wave.soundlen);
		while(len)
		{
			waveleft = m_wave.soundlen;
			if (waveleft > len) waveleft = len;
 		       	SDL_MixAudio(stream, waveptr, len, SDL_MIX_MAXVOLUME);
			len            -= waveleft;
			stream         += waveleft;
		}
		return;
	}


	if (waveleft > len) waveleft = len;
       	SDL_MixAudio(stream, waveptr, waveleft, SDL_MIX_MAXVOLUME);
	len            -= waveleft;
	stream         += waveleft;

	// Underrun. Pad the buffer out.
	if (m_beepStat) while(len)
	{
		waveleft = 6;
		waveptr  = tone;
		if (waveleft > len) waveleft = len;
   	    	SDL_MixAudio(stream, waveptr, waveleft, SDL_MIX_MAXVOLUME);
		len            -= waveleft;
		stream         += waveleft;
	}
	else
	{
		memset(waveptr, m_wave.spec.silence, m_wave.soundlen);
		while(len)
		{
			waveleft = m_wave.soundlen;
			if (waveleft > len) waveleft = len;
 		       	SDL_MixAudio(stream, waveptr, len, SDL_MIX_MAXVOLUME);
			len            -= waveleft;
			stream         += waveleft;
		}
	}
	m_wave.recpos   = 0;
}

void PcwBeeper::tick(void)
{
	++m_ticks;
	if (m_ticks < 160) return;

	if (!m_wave.sound) return;
	if (m_beepStat) m_wave.sound[m_wave.recpos] = tone[m_wave.recpos % 6];
	else		m_wave.sound[m_wave.recpos] = m_wave.spec.silence;

	// Don't loop if the buffer overfills. Instead, just 
	// stop.	
	++m_wave.recpos;
	if (m_wave.recpos >= m_wave.soundlen) --m_wave.recpos;
	m_ticks = 0;
}



void PcwBeeper::beepOn(void)
{
	m_beepStat = 1;
}

void PcwBeeper::beepOff(void)
{
	m_beepStat = 0;
}


void PcwBeeper::deinitSound(void)
{
	if (m_wave.sound)
	{
		SDL_LockAudio();
		SDL_PauseAudio(1);
		SDL_UnlockAudio();
		SDL_CloseAudio(); 
		free(m_wave.sound);
		m_wave.sound = NULL;
	}
}

//
// Samples, frequency etc:
// We are sampling at 25000Hz. This means that we need a sample byte 
// every 160 CPU cycles.
//
//
void PcwBeeper::initSound(void)
{
	m_beepStat = 0;
	m_wave.spec.freq    = 25000;	
	m_wave.spec.format  = AUDIO_U8;
	m_wave.spec.channels= 1;
	m_wave.spec.samples = 256;
        m_wave.spec.callback = ::fillerup;
        m_wave.spec.userdata = this;
	if (SDL_OpenAudio(&m_wave.spec, NULL) < 0)
	{
		m_wave.spec.userdata = NULL;
		m_wave.sound = NULL;
		joyce_dprintf("Can't open audio : %s\n", SDL_GetError());
	}
	else
	{
		m_wave.sound = (Uint8 *)malloc(m_wave.spec.size);
		if (m_wave.sound)
		{
			m_wave.soundlen = m_wave.spec.size;
			m_wave.soundpos = 0;
			m_wave.recpos = 0;
			SDL_PauseAudio(0);
		}
		else SDL_CloseAudio();
	}
}



void PcwBeeper::reset(void)
{
	beepOff();

        m_ticks = 0;
        m_wave.recpos = 0;
        m_wave.soundpos = 0;
}


bool PcwBeeper::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

bool PcwBeeper::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

