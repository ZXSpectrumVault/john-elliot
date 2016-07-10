/*************************************************************************
**	 Copyright 2005, John Elliott                                   **
**       Copyright 1999, Caldera Thin Clients, Inc.                     **
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                **
**                                                                      **
**                  Historical Copyright                                **
**                                                                      **
**                                                                      **
**                                                                      **
**  Copyright (c) 1987, Digital Research, Inc. All Rights Reserved.     **
**  The Software Code contained in this listing is proprietary to       **
**  Digital Research, Inc., Monterey, California and is covered by U.S. **
**  and other copyright protection.  Unauthorized copying, adaptation,  **
**  distribution, use or display is prohibited and may be subject to    **
**  civil and criminal penalties.  Disclosure to others is prohibited.  **
**  For the terms and conditions of software code use refer to the      **
**  appropriate Digital Research License Agreement.                     **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"



static int compare_ipts(const void *p1, const void *p2)
{
	const long *l1 = (const long *)p1;
	const long *l2 = (const long *)p2;

	return (*l1) - (*l2);
}


/***********************************************************************
*CLC_FLIT
*        Find the Intersection points for each raster line y
*
*       Entry   CONTRL[1] = vectex count
*               PTSIN[]   = verticies
*               y         = scan line to intersect
*               fill_intersect  = 0
*
*       Exit    fill_intersect  = count of intersections
*               INTIN  = x values of intersections
*
***********************************************************************/
void SdSdl::CLC_FLIT(long count, GPoint *vertices)
{
	long *ipts = new long[(2 * (count+10))];
	GPoint *vertex = vertices;
	long *ipt = ipts;
	long npt, npt2;
	
	if (!ipts) return;

	/* First, build a list of intersection points */

	for (npt = 0; npt < count; vertex++, npt++)
	{
		long dy, dy1, dy2, dx, ax;
		long y0 = vertex[0].y;
		long y1 = vertex[1].y;
/* The corresponding lines are commented out in the Digital Research source.
 *
 *		if (y1 & 0x80000000) continue;	// look at the flag if not 
 *						// a required edge
 *		y0 &= 0x7FFFFFFF;		// get rid of flag if any
 */
		dy = y1 - y0;
		if (!dy) 
		{
			/* vertex[i].y |= 0x80000000;  // DRI again */
			continue;
		}
		dy1 = m_Y1 - y0;	/* delta y1 */
		dy2 = m_Y1 - y1;	/* delta y2 */
		
		if ((dy1 < 0 && dy2 < 0) || (dy1 >=0 && dy2 >= 0))
		{
			/* DRI commented out 
			 * if (dy1 >= 0)	vertex[0].y |= 0x80000000; */
			continue;
		}
		dx = vertex[1].x - vertex[0].x;	
		
		ax = (2 * dx * dy1) / dy;
		if (ax < 0) ax = -((1 - ax) / 2);
		else ax = (ax+1) / 2;
	
		ax += vertex[0].x;
		*(ipt++) = ax; 	/* increment fill count  and buffer pointer */
		++m_fill_intersect;
	}

	if (m_fill_intersect < 2) 	/* < 2 intersections, go home */
	{
		delete ipts;
		return;
	}
	if (m_fill_intersect == 2)	/* if 2 then simple sort and draw */
	{
		m_X1 = (ipts[0] < ipts[1]) ? ipts[0] : ipts[1];
		m_X2 = (ipts[0] < ipts[1]) ? ipts[1] : ipts[0];
		HLINE_CLIP();
		delete ipts;
		return;
	}
	/* Sort the intersections' X coordinates. GEM used bubblesort
	 * here but the C library gives us qsort. */
	qsort(ipts,  m_fill_intersect, sizeof(long), compare_ipts);
	
	/* Draw line segments */
	m_fill_intersect /= 2;
	ipt = ipts;
	for (npt = 0; npt < m_fill_intersect; npt++)
	{
		m_X1 = ipt[0];
		m_X2 = ipt[1];
		HLINE_CLIP();
		ipt += 2;
	}
	delete ipts;
}

