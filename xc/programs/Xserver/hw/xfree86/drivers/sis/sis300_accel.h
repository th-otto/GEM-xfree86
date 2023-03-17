/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 *           Xavier Ducoin <x.ducoin@lectra.com>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis300_accel.h,v 1.1 2000/02/12 20:45:32 dawes Exp $ */


/* Definitions for the SIS engine communication. */

#define PATREGSIZE	384
#define BR(x)	(0x8200 | (x) << 2)
#define PBR(x)	(0x8300 | (x) << 2)

/* Definitions for the SiS300 engine command */
#define BITBLT                  0x00000000
#define COLOREXP                0x00000001
#define ENCOLOREXP		0x00000002
#define MULTIPLE_SCANLINE       0x00000003
#define LINE                    0x00000004
#define TRAPAZOID_FILL          0x00000005
#define TRANSPARENT_BITBLT      0x00000006

#define SRCVIDEO		0x00000000
#define SRCSYSTEM		0x00000010
#define SRCAGP                  0x00000020

#define PATFG   		0x00000000
#define PATPATREG		0x00000040
#define PATMONO   	 	0x00000080

#define X_INC                   0x00010000
#define X_DEC                   0x00000000
#define Y_INC                   0x00020000
#define Y_DEC                   0x00000000

#define NOCLIP			0x00000000
#define NOMERGECLIP		0x04000000
#define CLIPENABLE		0x00040000
#define CLIPWITHOUTMERGE	0x04040000

#define OPAQUE                  0x00000000
#define TRANSPARENT		0x00100000

#define DSTAGP			0x02000000
#define DSTVIDEO                0x02000000

/* Line */
#define LINE_STYLE		0x00800000
#define NO_RESET_COUNTER	0x00400000
#define NO_LAST_PIXEL		0x00200000

/* Macros to do useful things with the SIS BitBLT engine */

/* 
   bit 31 2D engine: 1 is idle,
   bit 30 3D engine: 1 is idle,
   bit 29 Command queue: 1 is empty
*/

#define SiSIdle \
  while( (MMIO_IN16(pSiS->IOBase, BR(16)+2) & 0xE000) != 0xE000){};

#define SiSSetupSRCBase(base) \
	MMIO_OUT32(pSiS->IOBase, BR(0), base);

#define SiSSetupSRCPitch(pitch) \
	MMIO_OUT16(pSiS->IOBase, BR(1), pitch);

#define SiSSetupSRCXY(x,y) \
	MMIO_OUT32(pSiS->IOBase, BR(2), (x)<<16 | (y) );

#define SiSSetupDSTBase(base) \
	MMIO_OUT32(pSiS->IOBase, BR(4), base);

#define SiSSetupDSTXY(x,y) \
	MMIO_OUT32(pSiS->IOBase, BR(3), (x)<<16 | (y) );

#define SiSSetupDSTRect(x,y) \
	MMIO_OUT32(pSiS->IOBase, BR(5), (y)<<16 | (x) );

#define SiSSetupDSTColorDepth(bpp) \
	MMIO_OUT16(pSiS->IOBase, BR(1)+2, bpp);

#define SiSSetupRect(w,h) \
	MMIO_OUT32(pSiS->IOBase, BR(6), (h)<<16 | (w) );

#define SiSSetupPATFG(color) \
	MMIO_OUT32(pSiS->IOBase, BR(7), color);

#define SiSSetupPATBG(color) \
	MMIO_OUT32(pSiS->IOBase, BR(8), color);

#define SiSSetupSRCFG(color) \
	MMIO_OUT32(pSiS->IOBase, BR(9), color);

#define SiSSetupSRCBG(color) \
	MMIO_OUT32(pSiS->IOBase, BR(10), color);

#define SiSSetupMONOPAT(p0,p1) \
	MMIO_OUT32(pSiS->IOBase, BR(11), p0); \
	MMIO_OUT32(pSiS->IOBase, BR(12), p1);

#define SiSSetupClipLT(left,top) \
	MMIO_OUT32(pSiS->IOBase, BR(13), ((left) & 0xFFFF) | (top)<<16 );

#define SiSSetupClipRB(right,bottom) \
	MMIO_OUT32(pSiS->IOBase, BR(14), ((right) & 0xFFFF) | (bottom)<<16 );

#define SiSSetupROP(rop) \
	pSiS->CommandReg = (rop) << 8;

#define SiSSetupCMDFlag(flags) \
	pSiS->CommandReg |= (flags);

#define SiSDoCMD \
	MMIO_OUT32(pSiS->IOBase, BR(15), pSiS->CommandReg); \
	MMIO_OUT32(pSiS->IOBase, BR(16), 0);

#define SiSSetupX0Y0(x,y) \
	MMIO_OUT32(pSiS->IOBase, BR(2), (y)<<16 | (x) );

#define SiSSetupX1Y1(x,y) \
	MMIO_OUT32(pSiS->IOBase, BR(3), (y)<<16 | (x) );

#define SiSSetupLineCount(c) \
	MMIO_OUT16(pSiS->IOBase, BR(6), c);

#define SiSSetupStylePeriod(p) \
	MMIO_OUT16(pSiS->IOBase, BR(6)+2, p);

#define SiSSetupStyleLow(ls) \
	MMIO_OUT32(pSiS->IOBase, BR(11), ls);

#define SiSSetupStyleHigh(ls) \
	MMIO_OUT32(pSiS->IOBase, BR(12), ls);

