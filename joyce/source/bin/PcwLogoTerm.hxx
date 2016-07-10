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


class PcwLogoTerm : public PcwSdlTerm
{
public:
	PcwLogoTerm(PcwSystem *s);
	virtual ~PcwLogoTerm();

protected:
	Sint16 m_splitSz;
	Uint16 m_gfxY;

	void doSplit(void);
	int makerop(Uint8 pen, Uint8 *colour);

	void lios0(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios4(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios5(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios12(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios17(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios18(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios19(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios20(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios21(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios22(void) {}	/* Possible no-ops, functions implemented in Z80 */
	void lios23(void) {}	/* Possible no-ops, functions implemented in Z80 */

	void zstrcpy(word dest, char *src);
	word lios_init(word liospb, word msgbuf);
	void lios1(void);
	void lios2(void);
	void lios3(byte split);

	void inline lios6(byte ch) { if (m_splitSz) (*m_term) << ch; }
	void lios7(word T, word B);	/* Clear text lines T to B */
	void lios8(word X, word Y);	/* Move text cursor to X,Y */
	void lios9(word T, word B);	/* Scroll text window */
	word lios10(word X, word Y, byte colour, byte pen);
	word lios11(word X1, word Y1, word X2, word Y2, byte colour, byte pen);
	word lios13(word colour, word pixels);
	void lios14(void) { m_term->bel(); }
	word lios15(void);
	word  lios16(word X, word Y);
	word lios24(word buf, word recno);
	word lios25(word buf, word recno);

public:
	void drLogo(Z80 *R);

private:
	byte m_hdr1[54];	/* BMP header */
	byte m_palette[1024];	/* BMP palette */

	long m_hdrlen, m_hdrused; /* hdrlen: Length of info area */
	long m_pallen, m_palused, m_palbegin, m_palend;
	long m_piclen, m_pich, m_picw;

	int isbmp(void);
	int is8bit(void);

	void get_hdr1(void); 
	void get_palette(void);
	byte colmap[256];	// Mappings of BMP colours to JOYCE colours
	void put_palette(void);
	word get_bmp_record(word Z80addr, word recno);
	word put_bmp_record(word Z80addr, word recno);
};

