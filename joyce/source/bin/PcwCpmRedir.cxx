/*
    CPMREDIR: CP/M filesystem redirector
    Copyright (C) 1998, John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    This is a sample piece of redirection code, which is designed to work
    with CPMREDIR.RSX (see CPMREDIR.ZSM)
*/

#include "cpmredir.h"

#include "Pcw.hxx"	/* or whatever #includes you need for your emulator */

cpm_byte *RAM;

#define SCB_CDRV  (RAM[0xFBAF])	/* CCP's drive */
#define SCB_DMA   (0xFBD8)	/* CP/M's DMA address */
#define SCB_DRV   (RAM[0xFBDA])	/* CP/M's current drive */


static cpm_word peekw(cpm_word a)	/* Read a word from RAM */
{
	return RAM[a] + 256*RAM[a+1];
}


/* Return 1 to continue with BDOS call, 0 to return */

#undef de	// I want to use it as a variable name

static int redirector(Z80 *R)
{
	/* DMA address */
	static cpm_byte *pdma = NULL;
	static cpm_word  adma = 0;

	/* Parameters as various routines want them... */
	cpm_byte e    = R->e;	/*  E as byte */
	cpm_word de   = R->getDE();	/* DE as word */
	cpm_byte *pde = RAM + de;	/* DE as pointer */
	cpm_byte *rsx = RAM + R->getHL();	/* Address of RSX */
	cpm_byte *drv = rsx + 0x1B;	/* 16-byte table of redirected drives */
	cpm_byte *ptr1;			/* Pointer for misc. use */
	int retc = 1;			/* Return value */
	static int idrv = 0;		/* Current "default" drive */
	cpm_byte drive;			/* Current FCB drive */
	char dname[41];			/* Directory name */
	static cpm_byte lastdrv = 0;	/* Last drive flag from call 0x11 */

	if (peekw(SCB_DMA) != adma)
	{
		adma = peekw(SCB_DMA);
		pdma = RAM + adma;
	}

	if (idrv != SCB_DRV)
	{
		/* SCB drive manually set by the CCP */
		fcb_drive(SCB_DRV);
		idrv = SCB_DRV;
	}

	/* Convert FCB "drive" byte to real drivenumber */
	drive = *pde & 0x7F;
	if (!drive || drive == '?') drive = idrv; else --drive;

	/* Various functions... */
	switch(R->c)
	{
		case 0x3C:	/* RSX calls... */
		/* Init RSX */
		if (pde[0] == 0x79 && pde[1] == 1)	
		{
			/* RSXPB parameter 1 */
			ptr1 = RAM + peekw(de + 2);
			if (memcmp(ptr1, rsx + 0x10, 8)) return 1;
			fcb_init();
			retc = 0;
		}
		/* Make a mapping */
		if (pde[0] == 0x78 && pde[1] == 3)
		{
			ptr1 = RAM + peekw(de + 2);
			if (memcmp(ptr1, rsx + 0x10, 8)) return 1;
			ptr1 = RAM + peekw(de + 6);
			R->setHL(xlt_map(peekw(de + 4), (char *)ptr1));
			if (R->getHL() == 1) drv[peekw(de + 4)] = 1;
			retc = 0;	
		}
		/* Revoke a mapping */
		if (pde[0] == 0x77 && pde[1] == 2)
		{
			ptr1 = RAM + peekw(de + 2);
			if (memcmp(ptr1, rsx + 0x10, 8)) return 1;
			R->setHL(xlt_umap(peekw(de + 4)));
			if (R->getHL() == 1) drv[peekw(de + 4)] = 0;
			retc = 0;	
		}
		if (pde[0] == 0x41)	/* Get dir name */
		{
			/* Note dirty trick: We know where CCP keeps its 
                         * drivenumber!
                         */
			if (drv[RAM[0xFBAF]])
			{
				R->c = 9;
				R->setDE(R->getHL() + 0x13b);
				
				sprintf(dname, "=%-.38s", xlt_getcwd(RAM[0xFBAF]));
				strcat(dname, "$");

				memcpy(RAM + R->getDE(), dname, 40);
				retc = 1;
			}
		}
		break;
		
		case 0x0D:		/* Reset discs */
		R->setHL(fcb_reset());
		idrv    = 0;
		break;

		case 0x0E:		/* Select drive */
		if (drv[e]) 
                {
                     R->setHL(fcb_drive(e)); 
                     retc = 0; 
                     if (!R->getHL()) SCB_DRV = e;
                }
		idrv = e;
		break; 

		case 0x0F:		/* Open file */
		if (drv[drive]) { R->setHL(fcb_open(pde, pdma)); retc = 0; }
		break;

		case 0x10:		/* Close file */
		if (drv[drive]) { R->setHL(fcb_close(pde)); retc = 0; }
		break;
	
		case 0x11:		/* Search 1st */
		lastdrv = drv[drive];
		if (drv[drive]) { R->setHL(fcb_find1(pde, pdma)); retc = 0; }
		break;

		case 0x12:		/* Search next */
		if (lastdrv) { R->setHL(fcb_find2(pde, pdma)); retc = 0; }
		break;

		case 0x13:		/* Erase file */
		if (drv[drive]) { R->setHL(fcb_unlink(pde, pdma)); retc = 0; }
		break;

		case 0x14:		/* Read sequential */
		if (drv[drive]) { R->setHL(fcb_read(pde, pdma)); retc = 0; }
		break;
		
		case 0x15:		/* Write sequential */
		if (drv[drive]) { R->setHL(fcb_write(pde, pdma)); retc = 0; }
		break;

		case 0x16:		/* Create file */
		if (drv[drive]) { R->setHL(fcb_creat(pde, pdma)); retc = 0; }
		break;

		case 0x17:		/* Rename file */
		if (drv[drive]) { R->setHL(fcb_rename(pde, pdma)); retc = 0; }
		break;

		case 0x1A:		/* Set DMA */
		pdma = pde;
		adma = de;
		break;	

		case 0x1B:		/* Get allocation vector */
		if (drv[idrv])
		{
			R->setHL(R->getHL()  +  0x13b); 
			fcb_getalv(RAM + R->getHL(), 40);
			retc = 0;
//			R->Trace = 1;
		}
		break;

		case 0x1C:		/* Make disc R/O */
		if (drv[idrv]) { fcb_rodisk(); retc = 0; }
		break;

		case 0x1E:		/* Set attributes */
                if (drv[drive]) { R->setHL(fcb_chmod(pde, pdma)); retc = 0; }
		break;

		case 0x1F:		/* Get DPB */
		if (drv[idrv]) 
		{
			fcb_getdpb(rsx + 0x2B + (0x11 * idrv));
			R->setHL(R->getHL() + 0x2B + (0x11 * idrv));
			retc = 0;
		} 
		break;

		case 0x20:		/* Set/get user */
		fcb_user(e);
		break;

                case 0x21:		/* Read random */
                if (drv[drive]) { R->setHL(fcb_randrd(pde, pdma)); retc = 0; }
                break;

                case 0x22:		/* Write random */
                if (drv[drive]) { R->setHL(fcb_randwr(pde, pdma)); retc = 0; }
                break;

		case 0x23:		/* Get file size */
		if (drv[drive]) { R->setHL(fcb_stat(pde)); retc = 0; }
		break;

                case 0x24:		/* Get file position */
                if (drv[drive]) { R->setHL(fcb_tell(pde)); retc = 0; }
                break;

		case 0x25:		/* Reset R/O drives */
		fcb_resro(de);
		break;

		case 0x28:		/* Write random with 0 fill */
                if (drv[drive]) { R->setHL(fcb_randwz(pde, pdma)); retc = 0; }
                break;
		
		case 0x2C:		/* Set multisector count */
		fcb_multirec(e);
		break;

		case 0x2E:		/* Get free space */
		if (drv[e]) { fcb_dfree(e, pdma); retc = 0; }
		break;

		case 0x30:		/* Empty buffers */
		fcb_sync(e);
		break;

		case 0x62:		/* Free temporary blocks */
		fcb_purge();
		break;	

		case 0x63:		/* Truncate file */
                if (drv[drive]) { R->setHL(fcb_trunc(pde, pdma)); retc = 0; }
		break;

		case 0x64:		/* Set label */
		if (drv[drive]) { R->setHL(fcb_setlbl(pde, pdma)); retc = 0; }
		break;

		case 0x65:		/* Get label byte */
		if (drv[e]) { R->setHL(fcb_getlbl(e)); retc = 0; }
		break;

                case 0x66:		/* Get date */
                if (drv[drive]) { R->setHL(fcb_date(pde)); retc = 0; }
                break;

		case 0x67:		/* Set password */
		if (drv[drive]) { R->setHL(fcb_setpwd(pde, pdma)); retc = 0; }
		break;

		case 0x6A:		/* Set default password */
		fcb_defpwd(pde);
		break;

		case 0x74:		/* Set stamps */
                if (drv[drive]) { R->setHL(fcb_sdate(pde, pdma)); retc = 0; }
		break;
	}
	R->a = R->l;
	R->b = R->h;
	return retc;

}


/* Actual entry from the emulator */

void cpmredir_intercept(Z80 *R)
{
	/* This relies on the TPA being in banks 4,5,6,7 */
	RAM = PCWRAM + 0x10000L;

	if (redirector(R)) R->f |= 0x01;	/* Carry set, call main BDOS */
	else               R->f &= 0xFE;	/* Carry clear, return */
}
