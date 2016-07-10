/*************************************************************************
**                                                                      **
**	 Copyright 2005, John Elliott                                   **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
*************************************************************************/



class VdiDriver
{
protected:
	VdiDriver() {}
public:
	virtual bool open(const GWSParams *wsp, GWSResult *wsr) = 0;
	virtual void close() = 0;
	virtual void clear() = 0;
	virtual void flush() = 0;
        virtual void escape(long type, long pcount, const GPoint *pt, 
			long icount, const long *ival, wchar_t *text,
			long *pcout, GPoint *ptout,
			long *icout, long *intout) = 0;
	virtual void enterCur() = 0;
	virtual void exitCur() = 0;
	virtual void polyLine   (long count, const GPoint *pt) = 0;
	virtual void polyMarker (long count, const GPoint *pt) = 0;
	virtual void graphicText(const GPoint *pt, const wchar_t *str) = 0;
	virtual void fillArea(long count, const GPoint *pt) = 0;
        virtual void gdp(long type, long pcount, const GPoint *pt, long icount,
		                    const long *ival, wchar_t *text) = 0;

	virtual void stHeight(const GPoint *height, GPoint *result) = 0;
	virtual long stRotation(long angle) = 0;
	virtual void setColor(long index, short r, short g, short b) = 0;
	virtual long slType(long type) = 0;
	virtual void slWidth(const GPoint *width, GPoint *result) = 0;
	virtual long slColor(long index) = 0;
	virtual long smType(long type) = 0;
	virtual void smHeight(const GPoint *height, GPoint *result) = 0;
	virtual long smColor(long color) = 0;
	
	virtual long stFont	(long font) = 0;
	virtual long stColor	(long color) = 0;
	virtual long sfStyle	(long style) = 0;
	virtual long sfIndex	(long index) = 0;
	virtual long sfColor	(long color) = 0;
	virtual long qColor	(long color, short mode, short *r, short *g, short *b) = 0;
	virtual short swrMode	(short mode) = 0;
	virtual short qLocator  (short device, const GPoint *ptin,
					GPoint *ptout, short *termch) = 0;
	virtual short sinMode	(short device, short mode) = 0;
	virtual void queryLine	(long *styles, GPoint *width) = 0;
	virtual void queryMarker(long *styles, GPoint *size) = 0;
	virtual void queryFill	(long *styles) = 0;
	virtual void queryText	(long *styles, GPoint *size) = 0;
	virtual void stAlign	(long h, long v, long *ah, long *av) = 0;

	virtual VdiDriver *openVirtual(const GWSParams *wsp, GWSResult *wsr) = 0;
	virtual void closeVirtual() = 0;

	virtual void queryExt       (int question, GWSResult *wsr) = 0;
	virtual void contourFill    (long fill, const GPoint *xy)  = 0;
	virtual long sfPerimeter    (long visible) = 0;
	virtual int  getPixel	    (const GPoint *xy, long *pel, long *index) = 0;
	virtual void queryTextExtent(const wchar_t *str, GPoint *extent) = 0;

	virtual USERMOT exMotv(USERMOT um) = 0;
	virtual USERCUR exCurv(USERCUR uc) = 0;
	virtual USERBUT exButv(USERBUT ub) = 0;
};

