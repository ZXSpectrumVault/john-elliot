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

enum BeeperMode { BM_NONE, BM_SOUND, BM_BORDER };

struct Wave
{
        SDL_AudioSpec spec;
        Uint8   *sound;                 /* Pointer to wave data */
        Uint32   soundlen;              /* Length of wave data */
        int      soundpos;              /* Current play position */
	int      recpos;
};



class PcwBeeper : public PcwDevice
{
public:
	PcwBeeper();
	virtual ~PcwBeeper();

public:
	virtual void beepOn(void);
	virtual void beepOff(void);
	void fillerup(Uint8 *stream, int len);
	void initSound(void);
	void deinitSound(void);
        virtual void tick(void);
	
	virtual void reset(void);
protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	int	   m_ticks;
	int	   m_beepStat;
	Wave	   m_wave;


};





