/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128_reg.h,v 1.6 2000/02/23 04:47:19 martin Exp $ */
/**************************************************************************

Copyright 1999 ATI Technologies Inc. and Precision Insight, Inc.,
                                         Cedar Park, Texas. 
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Rickard E. Faith <faith@precisioninsight.com>
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 */

#ifndef _R128_REG_H_
#define _R128_REG_H_
#include <compiler.h>

#if defined(__powerpc__)

static inline void regw(volatile unsigned long base_addr, unsigned long regindex, unsigned long regdata)
{
 asm volatile ("stwbrx %1,%2,%3; eieio"
          : "=m" (*(volatile unsigned *)(base_addr+regindex))
          : "r" (regdata), "b" (regindex), "r" (base_addr));
}

static inline void regw16(volatile unsigned long base_addr, unsigned long regindex, unsigned short regdata)
{
  asm volatile ("sthbrx %0,%1,%2; eieio": : "r"(regdata), "b"(regindex), "r"(base_addr));
}

static inline unsigned long regr(volatile unsigned long base_addr, unsigned long regindex)
{
  register unsigned long val;
  asm volatile ("lwbrx %0,%1,%2; eieio"
           : "=r"(val)
           : "b"(regindex), "r"(base_addr),
             "m" (*(volatile unsigned *)(base_addr+regindex)));
  return(val);
}

static inline unsigned short regr16(volatile unsigned long base_addr, unsigned long regindex)
{
  register unsigned short val;
  asm volatile ("lhbrx %0,%1,%2; eieio": "=r"(val):"b"(regindex), "r"(base_addr));
  return(val);
}

				/* Memory mapped register access macros */
#define INREG(addr)         regr(((unsigned long)R128MMIO),(addr))
#define INREG8(addr)        *(volatile CARD8  *)(R128MMIO + (addr))
#define INREG16(addr)       regr16(((unsigned long)R128MMIO), (addr))
#define OUTREG(addr, val)   regw(((unsigned long)R128MMIO), (addr), (val))
#define OUTREG8(addr, val)  *(volatile CARD8  *)(R128MMIO + (addr)) = (val)
#define OUTREG16(addr, val) regw16(((unsigned long)R128MMIO), (addr), (val))
#define ADDRREG(addr)       ((volatile CARD32 *)(R128MMIO + (addr)))

#define R128MMIO_VARS()                                                     \
    unsigned char *R128MMIO   = R128PTR(pScrn)->MMIO

#else
				/* Memory mapped register access macros */
#define INREG8(addr)        MMIO_IN8(R128MMIO, addr)
#define INREG16(addr)       MMIO_IN16(R128MMIO, addr)
#define INREG(addr)         MMIO_IN32(R128MMIO, addr)
#define OUTREG8(addr, val)  MMIO_OUT8(R128MMIO, addr, val)
#define OUTREG16(addr, val) MMIO_OUT16(R128MMIO, addr, val)
#define OUTREG(addr, val)   MMIO_OUT32(R128MMIO, addr, val)

#define ADDRREG(addr)       ((volatile CARD32 *)(R128MMIO + (addr)))

#define R128MMIO_VARS()                                                     \
    unsigned char *R128MMIO   = R128PTR(pScrn)->MMIO

#endif

#define OUTREGP(addr, val, mask)   \
    do {                           \
        CARD32 tmp = INREG(addr);  \
        tmp &= (mask);             \
        tmp |= (val);              \
        OUTREG(addr, tmp);         \
    } while (0)

#define OUTPLL(addr, val)                                                 \
    do {                                                                  \
        OUTREG8(R128_CLOCK_CNTL_INDEX, ((addr) & 0x1f) | R128_PLL_WR_EN); \
        OUTREG(R128_CLOCK_CNTL_DATA, val);                                \
    } while (0)

#define OUTPLLP(pScrn, addr, val, mask)                                   \
    do {                                                                  \
        CARD32 tmp = INPLL(pScrn, addr);                                  \
        tmp &= (mask);                                                    \
        tmp |= (val);                                                     \
        OUTPLL(addr, tmp);                                                \
    } while (0)

#define OUTPAL_START(idx)                                                 \
    do {                                                                  \
        OUTREG8(R128_PALETTE_INDEX, (idx));                               \
    } while (0)

#define OUTPAL_NEXT(r, g, b)                                              \
    do {                                                                  \
        OUTREG(R128_PALETTE_DATA, ((r) << 16) | ((g) << 8) | (b));        \
    } while (0)

#define OUTPAL_NEXT_CARD32(v)                                             \
    do {                                                                  \
        OUTREG(R128_PALETTE_DATA, (v & 0x00ffffff));                      \
    } while (0)

#define OUTPAL(idx, r, g, b)                                              \
    do {                                                                  \
        OUTPAL_START((idx));                                              \
        OUTPAL_NEXT((r), (g), (b));                                       \
    } while (0)

#define INPAL_START(idx)                                                  \
    do {                                                                  \
        OUTREG(R128_PALETTE_INDEX, (idx) << 16);                          \
    } while (0)

#define INPAL_NEXT() INREG(R128_PALETTE_DATA)

#define R128_ADAPTER_ID                   0x0f2c /* PCI */
#define R128_AGP_APER_OFFSET              0x0178
#define R128_AGP_BASE                     0x0170
#define R128_AGP_CNTL                     0x0174
#define R128_AGP_COMMAND                  0x0f58 /* PCI */
#define R128_AGP_PLL_CNTL                 0x0010 /* PLL */
#define R128_AGP_STATUS                   0x0f54 /* PCI */
#define R128_AMCGPIO_A_REG                0x01a0
#define R128_AMCGPIO_EN_REG               0x01a8
#define R128_AMCGPIO_MASK                 0x0194
#define R128_AMCGPIO_Y_REG                0x01a4
#define R128_ATTRDR                       0x03c1 /* VGA */
#define R128_ATTRDW                       0x03c0 /* VGA */
#define R128_ATTRX                        0x03c0 /* VGA */
#define R128_AUX_SC_CNTL                  0x1660
#define R128_AUX1_SC_BOTTOM               0x1670
#define R128_AUX1_SC_LEFT                 0x1664
#define R128_AUX1_SC_RIGHT                0x1668
#define R128_AUX1_SC_TOP                  0x166c
#define R128_AUX2_SC_BOTTOM               0x1680
#define R128_AUX2_SC_LEFT                 0x1674
#define R128_AUX2_SC_RIGHT                0x1678
#define R128_AUX2_SC_TOP                  0x167c
#define R128_AUX3_SC_BOTTOM               0x1690
#define R128_AUX3_SC_LEFT                 0x1684
#define R128_AUX3_SC_RIGHT                0x1688
#define R128_AUX3_SC_TOP                  0x168c

#define R128_BASE_CODE                    0x0f0b
#define R128_BIOS_0_SCRATCH               0x0010
#define R128_BIOS_1_SCRATCH               0x0014
#define R128_BIOS_2_SCRATCH               0x0018
#define R128_BIOS_3_SCRATCH               0x001c
#define R128_BIOS_ROM                     0x0f30 /* PCI */
#define R128_BIST                         0x0f0f /* PCI */
#define R128_BRUSH_DATA0                  0x1480
#define R128_BRUSH_DATA1                  0x1484
#define R128_BRUSH_DATA10                 0x14a8
#define R128_BRUSH_DATA11                 0x14ac
#define R128_BRUSH_DATA12                 0x14b0
#define R128_BRUSH_DATA13                 0x14b4
#define R128_BRUSH_DATA14                 0x14b8
#define R128_BRUSH_DATA15                 0x14bc
#define R128_BRUSH_DATA16                 0x14c0
#define R128_BRUSH_DATA17                 0x14c4
#define R128_BRUSH_DATA18                 0x14c8
#define R128_BRUSH_DATA19                 0x14cc
#define R128_BRUSH_DATA2                  0x1488
#define R128_BRUSH_DATA20                 0x14d0
#define R128_BRUSH_DATA21                 0x14d4
#define R128_BRUSH_DATA22                 0x14d8
#define R128_BRUSH_DATA23                 0x14dc
#define R128_BRUSH_DATA24                 0x14e0
#define R128_BRUSH_DATA25                 0x14e4
#define R128_BRUSH_DATA26                 0x14e8
#define R128_BRUSH_DATA27                 0x14ec
#define R128_BRUSH_DATA28                 0x14f0
#define R128_BRUSH_DATA29                 0x14f4
#define R128_BRUSH_DATA3                  0x148c
#define R128_BRUSH_DATA30                 0x14f8
#define R128_BRUSH_DATA31                 0x14fc
#define R128_BRUSH_DATA32                 0x1500
#define R128_BRUSH_DATA33                 0x1504
#define R128_BRUSH_DATA34                 0x1508
#define R128_BRUSH_DATA35                 0x150c
#define R128_BRUSH_DATA36                 0x1510
#define R128_BRUSH_DATA37                 0x1514
#define R128_BRUSH_DATA38                 0x1518
#define R128_BRUSH_DATA39                 0x151c
#define R128_BRUSH_DATA4                  0x1490
#define R128_BRUSH_DATA40                 0x1520
#define R128_BRUSH_DATA41                 0x1524
#define R128_BRUSH_DATA42                 0x1528
#define R128_BRUSH_DATA43                 0x152c
#define R128_BRUSH_DATA44                 0x1530
#define R128_BRUSH_DATA45                 0x1534
#define R128_BRUSH_DATA46                 0x1538
#define R128_BRUSH_DATA47                 0x153c
#define R128_BRUSH_DATA48                 0x1540
#define R128_BRUSH_DATA49                 0x1544
#define R128_BRUSH_DATA5                  0x1494
#define R128_BRUSH_DATA50                 0x1548
#define R128_BRUSH_DATA51                 0x154c
#define R128_BRUSH_DATA52                 0x1550
#define R128_BRUSH_DATA53                 0x1554
#define R128_BRUSH_DATA54                 0x1558
#define R128_BRUSH_DATA55                 0x155c
#define R128_BRUSH_DATA56                 0x1560
#define R128_BRUSH_DATA57                 0x1564
#define R128_BRUSH_DATA58                 0x1568
#define R128_BRUSH_DATA59                 0x156c
#define R128_BRUSH_DATA6                  0x1498
#define R128_BRUSH_DATA60                 0x1570
#define R128_BRUSH_DATA61                 0x1574
#define R128_BRUSH_DATA62                 0x1578
#define R128_BRUSH_DATA63                 0x157c
#define R128_BRUSH_DATA7                  0x149c
#define R128_BRUSH_DATA8                  0x14a0
#define R128_BRUSH_DATA9                  0x14a4
#define R128_BRUSH_SCALE                  0x1470
#define R128_BRUSH_Y_X                    0x1474
#define R128_BUS_CNTL                     0x0030
#	define R128_BUS_RD_DISCARD_EN	  (1 << 24)
#	define R128_BUS_RD_ABORT_EN	  (1 << 25)
#	define R128_BUS_MSTR_DISCONNECT_EN (1 << 28)
#	define R128_BUS_WRT_BURST         (1 << 29)
#	define R128_BUS_READ_BURST	  (1 << 30)
#define R128_BUS_CNTL1                    0x0034

#define R128_CACHE_CNTL                   0x1724
#define R128_CACHE_LINE                   0x0f0c /* PCI */
#define R128_CAP0_TRIG_CNTL               0x0950 /* ? */
#define R128_CAP1_TRIG_CNTL               0x09c0 /* ? */
#define R128_CAPABILITIES_ID              0x0f50 /* PCI */
#define R128_CAPABILITIES_PTR             0x0f34 /* PCI */
#define R128_CLK_PIN_CNTL                 0x0001 /* PLL */
#define R128_CLOCK_CNTL_DATA              0x000c
#define R128_CLOCK_CNTL_INDEX             0x0008
#       define R128_PLL_WR_EN             (1 << 7)
#       define R128_PLL_DIV_SEL           (3 << 8)
#define R128_CLR_CMP_CLR_3D               0x1a24
#define R128_CLR_CMP_CLR_DST              0x15c8
#define R128_CLR_CMP_CLR_SRC              0x15c4
#define R128_CLR_CMP_CNTL                 0x15c0
#       define R128_SRC_CMP_NEQ_COLOR     (5 <<  0)
#       define R128_CLR_CMP_SRC_SOURCE    (1 << 24)
#define R128_CLR_CMP_MASK                 0x15cc
#       define R128_CLR_CMP_MSK           0xffffffff
#define R128_CLR_CMP_MASK_3D              0x1A28
#define R128_COMMAND                      0x0f04 /* PCI */
#define R128_COMPOSITE_SHADOW_ID          0x1a0c
#define R128_CONFIG_APER_0_BASE           0x0100
#define R128_CONFIG_APER_1_BASE           0x0104
#define R128_CONFIG_APER_SIZE             0x0108
#define R128_CONFIG_BONDS                 0x00e8
#define R128_CONFIG_CNTL                  0x00e0
#define R128_CONFIG_MEMSIZE               0x00f8
#define R128_CONFIG_MEMSIZE_EMBEDDED      0x0114
#define R128_CONFIG_REG_1_BASE            0x010c
#define R128_CONFIG_REG_APER_SIZE         0x0110
#define R128_CONFIG_XSTRAP                0x00e4
#define R128_CONSTANT_COLOR_C             0x1d34
#define R128_CRC_CMDFIFO_ADDR             0x0740
#define R128_CRC_CMDFIFO_DOUT             0x0744
#define R128_CRTC_CRNT_FRAME              0x0214
#define R128_CRTC_DEBUG                   0x021c
#define R128_CRTC_EXT_CNTL                0x0054
#       define R128_CRTC_VGA_XOVERSCAN    (1 <<  0)
#       define R128_VGA_ATI_LINEAR        (1 <<  3)
#       define R128_XCRT_CNT_EN           (1 <<  6)
#       define R128_CRTC_HSYNC_DIS        (1 <<  8)
#       define R128_CRTC_VSYNC_DIS        (1 <<  9)
#       define R128_CRTC_DISPLAY_DIS      (1 << 10)
#define R128_CRTC_EXT_CNTL_DPMS_BYTE      0x0055
#       define R128_CRTC_HSYNC_DIS_BYTE   (1 <<  0)
#       define R128_CRTC_VSYNC_DIS_BYTE   (1 <<  1)
#       define R128_CRTC_DISPLAY_DIS_BYTE (1 <<  2)
#define R128_CRTC_GEN_CNTL                0x0050
#       define R128_CRTC_DBL_SCAN_EN      (1 <<  0)
#       define R128_CRTC_INTERLACE_EN     (1 <<  1)
#       define R128_CRTC_CUR_EN           (1 << 16)
#       define R128_CRTC_CUR_MODE_MASK    (7 << 17)
#       define R128_CRTC_EXT_DISP_EN      (1 << 24)
#       define R128_CRTC_EN               (1 << 25)
#define R128_CRTC_GUI_TRIG_VLINE          0x0218
#define R128_CRTC_H_SYNC_STRT_WID         0x0204
#       define R128_CRTC_H_SYNC_POL       (1 << 23)
#define R128_CRTC_H_TOTAL_DISP            0x0200
#define R128_CRTC_OFFSET                  0x0224
#define R128_CRTC_OFFSET_CNTL             0x0228
#define R128_CRTC_PITCH                   0x022c
#define R128_CRTC_STATUS                  0x005c
#       define R128_CRTC_VBLANK_SAVE      (1 <<  1)
#define R128_CRTC_V_SYNC_STRT_WID         0x020c
#       define R128_CRTC_V_SYNC_POL       (1 << 23)
#define R128_CRTC_V_TOTAL_DISP            0x0208
#define R128_CRTC_VLINE_CRNT_VLINE        0x0210
#       define R128_CRTC_CRNT_VLINE_MASK  (0x7ff << 16)
#define R128_CRTC8_DATA                   0x03d5 /* VGA, 0x3b5 */
#define R128_CRTC8_IDX                    0x03d4 /* VGA, 0x3b4 */
#define R128_CUR_CLR0                     0x026c
#define R128_CUR_CLR1                     0x0270
#define R128_CUR_HORZ_VERT_OFF            0x0268
#define R128_CUR_HORZ_VERT_POSN           0x0264
#define R128_CUR_OFFSET                   0x0260
#       define R128_CUR_LOCK              (1 << 31)

#define R128_DAC_CNTL                     0x0058
#       define R128_DAC_RANGE_CNTL        (3 <<  0)
#       define R128_DAC_BLANKING          (1 <<  2)
#       define R128_DAC_8BIT_EN           (1 <<  8)
#       define R128_DAC_VGA_ADR_EN        (1 << 13)
#       define R128_DAC_MASK_ALL          (0xff << 24)
#define R128_DAC_CRC_SIG                  0x02cc
#define R128_DAC_DATA                     0x03c9 /* VGA */
#define R128_DAC_MASK                     0x03c6 /* VGA */
#define R128_DAC_R_INDEX                  0x03c7 /* VGA */
#define R128_DAC_W_INDEX                  0x03c8 /* VGA */
#define R128_DDA_CONFIG                   0x02e0
#define R128_DDA_ON_OFF                   0x02e4
#define R128_DEFAULT_OFFSET               0x16e0
#define R128_DEFAULT_PITCH                0x16e4
#define R128_DEFAULT_SC_BOTTOM_RIGHT      0x16e8
#       define R128_DEFAULT_SC_RIGHT_MAX  (0x1fff <<  0)
#       define R128_DEFAULT_SC_BOTTOM_MAX (0x1fff << 16)
#define R128_DESTINATION_3D_CLR_CMP_MSK   0x1824
#define R128_DESTINATION_3D_CLR_CMP_VAL   0x1820
#define R128_DEVICE_ID                    0x0f02 /* PCI */
#define R128_DP_BRUSH_BKGD_CLR            0x1478
#define R128_DP_BRUSH_FRGD_CLR            0x147c
#define R128_DP_CNTL                      0x16c0
#       define R128_DST_X_LEFT_TO_RIGHT   (1 <<  0)
#       define R128_DST_Y_TOP_TO_BOTTOM   (1 <<  1)
#define R128_DP_CNTL_XDIR_YDIR_YMAJOR     0x16d0
#       define R128_DST_Y_MAJOR             (1 <<  2)
#       define R128_DST_Y_DIR_TOP_TO_BOTTOM (1 << 15)
#       define R128_DST_X_DIR_LEFT_TO_RIGHT (1 << 31)
#define R128_DP_DATATYPE                  0x16c4
#define R128_DP_GUI_MASTER_CNTL           0x146c
#       define R128_GMC_SRC_PITCH_OFFSET_CNTL (1    <<  0)
#       define R128_GMC_DST_PITCH_OFFSET_CNTL (1    <<  1)
#       define R128_GMC_SRC_CLIPPING          (1    <<  2)
#       define R128_GMC_DST_CLIPPING          (1    <<  3)
#       define R128_GMC_BRUSH_DATATYPE_MASK   (0x0f <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_BG  (0    <<  4)
#       define R128_GMC_BRUSH_8X8_MONO_FG_LA  (1    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_BG  (4    <<  4)
#       define R128_GMC_BRUSH_1X8_MONO_FG_LA  (5    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_BG (6    <<  4)
#       define R128_GMC_BRUSH_32x1_MONO_FG_LA (7    <<  4)
#       define R128_GMC_BRUSH_8x8_COLOR       (10   <<  4)
#       define R128_GMC_BRUSH_1X8_COLOR       (12   <<  4)
#       define R128_GMC_BRUSH_SOLID_COLOR     (13   <<  4)
#       define R128_GMC_DST_DATATYPE_MASK     (0x0f <<  8)
#       define R128_GMC_DST_DATATYPE_SHIFT    8
#       define R128_GMC_SRC_DATATYPE_MASK       (3    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_BG (0    << 12)
#       define R128_GMC_SRC_DATATYPE_MONO_FG_LA (1    << 12)
#       define R128_GMC_SRC_DATATYPE_COLOR      (3    << 12)
#       define R128_GMC_BYTE_PIX_ORDER        (1    << 14)
#       define R128_GMC_BYTE_LSB_TO_MSB       (1    << 14)
#       define R128_GMC_CONVERSION_TEMP       (1    << 15)
#       define R128_GMC_ROP3_MASK             (0xff << 16)
#       define R128_DP_SRC_SOURCE_MASK        (7    << 24)
#       define R128_DP_SRC_SOURCE_MEMORY      (2    << 24)
#       define R128_DP_SRC_SOURCE_HOST_DATA   (3    << 24)
#       define R128_GMC_3D_FCN_EN             (1    << 27)
#       define R128_GMC_CLR_CMP_CNTL_DIS      (1    << 28)
#       define R128_AUX_CLIP_DIS              (1    << 29)
#       define R128_GMC_WR_MSK_DS             (1    << 30)
#       define R128_GMC_LD_BRUSH_Y_X          (1    << 31)
#       define R128_ROP3_ZERO             0x00000000
#       define R128_ROP3_DSa              0x00880000
#       define R128_ROP3_SDna             0x00440000
#       define R128_ROP3_S                0x00cc0000
#       define R128_ROP3_DSna             0x00220000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DSx              0x00660000
#       define R128_ROP3_DSo              0x00ee0000
#       define R128_ROP3_DSon             0x00110000
#       define R128_ROP3_DSxn             0x00990000
#       define R128_ROP3_Dn               0x00550000
#       define R128_ROP3_SDno             0x00dd0000
#       define R128_ROP3_Sn               0x00330000
#       define R128_ROP3_DSno             0x00bb0000
#       define R128_ROP3_DSan             0x00770000
#       define R128_ROP3_ONE              0x00ff0000
#       define R128_ROP3_DPa              0x00a00000
#       define R128_ROP3_PDna             0x00500000
#       define R128_ROP3_P                0x00f00000
#       define R128_ROP3_DPna             0x000a0000
#       define R128_ROP3_D                0x00aa0000
#       define R128_ROP3_DPx              0x005a0000
#       define R128_ROP3_DPo              0x00fa0000
#       define R128_ROP3_DPon             0x00050000
#       define R128_ROP3_PDxn             0x00a50000
#       define R128_ROP3_PDno             0x00f50000
#       define R128_ROP3_Pn               0x000f0000
#       define R128_ROP3_DPno             0x00af0000
#       define R128_ROP3_DPan             0x005f0000


#define R128_DP_GUI_MASTER_CNTL_C         0x1c84
#define R128_DP_MIX                       0x16c8
#define R128_DP_SRC_BKGD_CLR              0x15dc
#define R128_DP_SRC_FRGD_CLR              0x15d8
#define R128_DP_WRITE_MASK                0x16cc
#define R128_DST_BRES_DEC                 0x1630
#define R128_DST_BRES_ERR                 0x1628
#define R128_DST_BRES_INC                 0x162c
#define R128_DST_BRES_LNTH                0x1634
#define R128_DST_BRES_LNTH_SUB            0x1638
#define R128_DST_HEIGHT                   0x1410
#define R128_DST_HEIGHT_WIDTH             0x143c
#define R128_DST_HEIGHT_WIDTH_8           0x158c
#define R128_DST_HEIGHT_WIDTH_BW          0x15b4
#define R128_DST_HEIGHT_Y                 0x15a0
#define R128_DST_OFFSET                   0x1404
#define R128_DST_PITCH                    0x1408
#define R128_DST_PITCH_OFFSET             0x142c
#define R128_DST_PITCH_OFFSET_C           0x1c80
#define R128_DST_WIDTH                    0x140c
#define R128_DST_WIDTH_HEIGHT             0x1598
#define R128_DST_WIDTH_X                  0x1588
#define R128_DST_WIDTH_X_INCY             0x159c
#define R128_DST_X                        0x141c
#define R128_DST_X_SUB                    0x15a4
#define R128_DST_X_Y                      0x1594
#define R128_DST_Y                        0x1420
#define R128_DST_Y_SUB                    0x15a8
#define R128_DST_Y_X                      0x1438

#define R128_EXT_MEM_CNTL                 0x0144

#define R128_FCP_CNTL                     0x0012 /* PLL */
#define R128_FLUSH_1                      0x1704
#define R128_FLUSH_2                      0x1708
#define R128_FLUSH_3                      0x170c
#define R128_FLUSH_4                      0x1710
#define R128_FLUSH_5                      0x1714
#define R128_FLUSH_6                      0x1718
#define R128_FLUSH_7                      0x171c

#define R128_GEN_INT_CNTL                 0x0040
#define R128_GEN_INT_STATUS               0x0044
#       define R128_VSYNC_INT_AK          (1 <<  2)
#       define R128_VSYNC_INT             (1 <<  2)
#define R128_GEN_RESET_CNTL               0x00f0
#       define R128_SOFT_RESET_GUI        (1 <<  0)
#define R128_GENENB                       0x03c3 /* VGA */
#define R128_GENFC_RD                     0x03ca /* VGA */
#define R128_GENFC_WT                     0x03da /* VGA, 0x03ba */
#define R128_GENMO_RD                     0x03cc /* VGA */
#define R128_GENMO_WT                     0x03c2 /* VGA */
#define R128_GENS0                        0x03c2 /* VGA */
#define R128_GENS1                        0x03da /* VGA, 0x03ba */
#define R128_GPIO_MONID                   0x0068
#       define R128_GPIO_MONID_A_0        (1 <<  0)
#       define R128_GPIO_MONID_A_1        (1 <<  1)
#       define R128_GPIO_MONID_A_2        (1 <<  2)
#       define R128_GPIO_MONID_A_3        (1 <<  3)
#       define R128_GPIO_MONID_Y_0        (1 <<  8)
#       define R128_GPIO_MONID_Y_1        (1 <<  9)
#       define R128_GPIO_MONID_Y_2        (1 << 10)
#       define R128_GPIO_MONID_Y_3        (1 << 11)
#       define R128_GPIO_MONID_EN_0       (1 << 16)
#       define R128_GPIO_MONID_EN_1       (1 << 17)
#       define R128_GPIO_MONID_EN_2       (1 << 18)
#       define R128_GPIO_MONID_EN_3       (1 << 19)
#       define R128_GPIO_MONID_MASK_0     (1 << 24)
#       define R128_GPIO_MONID_MASK_1     (1 << 25)
#       define R128_GPIO_MONID_MASK_2     (1 << 26)
#       define R128_GPIO_MONID_MASK_3     (1 << 27)
#define R128_GPIO_MONIDB                  0x006c
#define R128_GRPH8_DATA                   0x03cf /* VGA */
#define R128_GRPH8_IDX                    0x03ce /* VGA */
#define R128_GUI_DEBUG0                   0x16a0
#define R128_GUI_DEBUG1                   0x16a4
#define R128_GUI_DEBUG2                   0x16a8
#define R128_GUI_DEBUG3                   0x16ac
#define R128_GUI_DEBUG4                   0x16b0
#define R128_GUI_DEBUG5                   0x16b4
#define R128_GUI_DEBUG6                   0x16b8
#define R128_GUI_PROBE                    0x16bc
#define R128_GUI_SCRATCH_REG0             0x15e0
#define R128_GUI_SCRATCH_REG1             0x15e4
#define R128_GUI_SCRATCH_REG2             0x15e8
#define R128_GUI_SCRATCH_REG3             0x15ec
#define R128_GUI_SCRATCH_REG4             0x15f0
#define R128_GUI_SCRATCH_REG5             0x15f4
#define R128_GUI_STAT                     0x1740
#       define R128_GUI_FIFOCNT_MASK      0x0fff
#       define R128_GUI_ACTIVE            (1 << 31)

#define R128_HEADER                       0x0f0e /* PCI */
#define R128_HOST_DATA0                   0x17c0
#define R128_HOST_DATA1                   0x17c4
#define R128_HOST_DATA2                   0x17c8
#define R128_HOST_DATA3                   0x17cc
#define R128_HOST_DATA4                   0x17d0
#define R128_HOST_DATA5                   0x17d4
#define R128_HOST_DATA6                   0x17d8
#define R128_HOST_DATA7                   0x17dc
#define R128_HOST_DATA_LAST               0x17e0
#define R128_HOST_PATH_CNTL               0x0130
#define R128_HTOTAL_CNTL                  0x0009 /* PLL */
#define R128_HW_DEBUG                     0x0128

#define R128_I2C_CNTL_1                   0x0094 /* ? */
#define R128_INTERRUPT_LINE               0x0f3c /* PCI */
#define R128_INTERRUPT_PIN                0x0f3d /* PCI */
#define R128_IO_BASE                      0x0f14 /* PCI */

#define R128_LATENCY                      0x0f0d /* PCI */
#define R128_LEAD_BRES_DEC                0x1608
#define R128_LEAD_BRES_ERR                0x1600
#define R128_LEAD_BRES_INC                0x1604
#define R128_LEAD_BRES_LNTH               0x161c
#define R128_LEAD_BRES_LNTH_SUB           0x1624

#define R128_MAX_LATENCY                  0x0f3f /* PCI */
#define R128_MCLK_CNTL                    0x000f /* PLL */
#       define R128_FORCE_GCP             (1 << 16)
#       define R128_FORCE_PIPE3D_CPP      (1 << 17)
#define R128_MDGPIO_A_REG                 0x01ac
#define R128_MDGPIO_EN_REG                0x01b0
#define R128_MDGPIO_MASK                  0x0198
#define R128_MDGPIO_Y_REG                 0x01b4
#define R128_MEM_ADDR_CONFIG              0x0148
#define R128_MEM_BASE                     0x0f10 /* PCI */
#define R128_MEM_CNTL                     0x0140
#define R128_MEM_INIT_LAT_TIMER           0x0154
#define R128_MEM_INTF_CNTL                0x014c
#define R128_MEM_SDRAM_MODE_REG           0x0158
#define R128_MEM_STR_CNTL                 0x0150
#define R128_MEM_VGA_RP_SEL               0x003c
#define R128_MEM_VGA_WP_SEL               0x0038
#define R128_MIN_GRANT                    0x0f3e /* PCI */
#define R128_MISC_3D_STATE_CNTL_REG       0x1CA0
#define R128_MM_DATA                      0x0004
#define R128_MM_INDEX                     0x0000
#define R128_MPLL_CNTL                    0x000e /* PLL */
#define R128_MPP_TB_CONFIG                0x01c0 /* ? */
#define R128_MPP_GP_CONFIG                0x01c8 /* ? */

#define R128_N_VIF_COUNT                  0x0248

#define R128_OV0_SCALE_CNTL               0x0420 /* ? */
#define R128_OVR_CLR                      0x0230
#define R128_OVR_WID_LEFT_RIGHT           0x0234
#define R128_OVR_WID_TOP_BOTTOM           0x0238

#define R128_PALETTE_DATA                 0x00b4
#define R128_PALETTE_INDEX                0x00b0
#define R128_PC_DEBUG_MODE                0x1760
#define R128_PC_GUI_CTLSTAT               0x1748
#define R128_PC_GUI_MODE                  0x1744
#define R128_PC_NGUI_CTLSTAT              0x0184
#       define R128_PC_FLUSH_ALL          0x00ff
#       define R128_PC_BUSY               (1 << 31)
#define R128_PC_NGUI_MODE                 0x0180
#define R128_PCI_GART_PAGE                0x017c
#define R128_PLANE_3D_MASK_C              0x1d44
#define R128_PLL_TEST_CNTL                0x0013 /* PLL */
#define R128_PMI_CAP_ID                   0x0f5c /* PCI */
#define R128_PMI_DATA                     0x0f63 /* PCI */
#define R128_PMI_NXT_CAP_PTR              0x0f5d /* PCI */
#define R128_PMI_PMC_REG                  0x0f5e /* PCI */
#define R128_PMI_PMCSR_REG                0x0f60 /* PCI */
#define R128_PMI_REGISTER                 0x0f5c /* PCI */
#define R128_PPLL_CNTL                    0x0002 /* PLL */
#       define R128_PPLL_RESET                (1 <<  0)
#       define R128_PPLL_SLEEP                (1 <<  1)
#       define R128_PPLL_ATOMIC_UPDATE_EN     (1 << 16)
#       define R128_PPLL_VGA_ATOMIC_UPDATE_EN (1 << 17)
#define R128_PPLL_DIV_0                   0x0004 /* PLL */
#define R128_PPLL_DIV_1                   0x0005 /* PLL */
#define R128_PPLL_DIV_2                   0x0006 /* PLL */
#define R128_PPLL_DIV_3                   0x0007 /* PLL */
#       define R128_PPLL_FB3_DIV_MASK     0x07ff
#       define R128_PPLL_POST3_DIV_MASK   0x00070000
#define R128_PPLL_REF_DIV                 0x0003 /* PLL */
#       define R128_PPLL_REF_DIV_MASK     0x03ff
#       define R128_PPLL_ATOMIC_UPDATE_R  (1 << 15) /* same as _W */
#       define R128_PPLL_ATOMIC_UPDATE_W  (1 << 15) /* same as _R */
#define R128_PWR_MNGMT_CNTL_STATUS        0x0f60 /* PCI */
#define R128_REG_BASE                     0x0f18 /* PCI */
#define R128_REGPROG_INF                  0x0f09 /* PCI */
#define R128_REVISION_ID                  0x0f08 /* PCI */

#define R128_SC_BOTTOM                    0x164c
#define R128_SC_BOTTOM_RIGHT              0x16f0
#define R128_SC_BOTTOM_RIGHT_C            0x1c8c
#define R128_SC_LEFT                      0x1640
#define R128_SC_RIGHT                     0x1644
#define R128_SC_TOP                       0x1648
#define R128_SC_TOP_LEFT                  0x16ec
#define R128_SC_TOP_LEFT_C                0x1c88
#define R128_SEQ8_DATA                    0x03c5 /* VGA */
#define R128_SEQ8_IDX                     0x03c4 /* VGA */
#define R128_SNAPSHOT_F_COUNT             0x0244
#define R128_SNAPSHOT_VH_COUNTS           0x0240
#define R128_SNAPSHOT_VIF_COUNT           0x024c
#define R128_SRC_OFFSET                   0x15ac
#define R128_SRC_PITCH                    0x15b0
#define R128_SRC_PITCH_OFFSET             0x1428
#define R128_SRC_SC_BOTTOM                0x165c
#define R128_SRC_SC_BOTTOM_RIGHT          0x16f4
#define R128_SRC_SC_RIGHT                 0x1654
#define R128_SRC_X                        0x1414
#define R128_SRC_X_Y                      0x1590
#define R128_SRC_Y                        0x1418
#define R128_SRC_Y_X                      0x1434
#define R128_STATUS                       0x0f06 /* PCI */
#define R128_SUBPIC_CNTL                  0x0540 /* ? */
#define R128_SUB_CLASS                    0x0f0a /* PCI */
#define R128_SURFACE_DELAY                0x0b00
#define R128_SURFACE0_INFO                0x0b0c
#define R128_SURFACE0_LOWER_BOUND         0x0b04
#define R128_SURFACE0_UPPER_BOUND         0x0b08
#define R128_SURFACE1_INFO                0x0b1c
#define R128_SURFACE1_LOWER_BOUND         0x0b14
#define R128_SURFACE1_UPPER_BOUND         0x0b18
#define R128_SURFACE2_INFO                0x0b2c
#define R128_SURFACE2_LOWER_BOUND         0x0b24
#define R128_SURFACE2_UPPER_BOUND         0x0b28
#define R128_SURFACE3_INFO                0x0b3c
#define R128_SURFACE3_LOWER_BOUND         0x0b34
#define R128_SURFACE3_UPPER_BOUND         0x0b38
#define R128_SW_SEMAPHORE                 0x013c

#define R128_TEST_DEBUG_CNTL              0x0120
#define R128_TEST_DEBUG_MUX               0x0124
#define R128_TEST_DEBUG_OUT               0x012c
#define R128_TRAIL_BRES_DEC               0x1614
#define R128_TRAIL_BRES_ERR               0x160c
#define R128_TRAIL_BRES_INC               0x1610
#define R128_TRAIL_X                      0x1618
#define R128_TRAIL_X_SUB                  0x1620
#define R128_VCLK_ECP_CNTL                0x0008 /* PLL */
#define R128_VENDOR_ID                    0x0f00 /* PCI */
#define R128_VGA_DDA_CONFIG               0x02e8
#define R128_VGA_DDA_ON_OFF               0x02ec
#define R128_VID_BUFFER_CONTROL           0x0900
#define R128_VIDEOMUX_CNTL                0x0190
#define R128_VIPH_CONTROL                 0x01D0 /* ? */

#define R128_WAIT_UNTIL                   0x1720

#define R128_X_MPLL_REF_FB_DIV            0x000a /* PLL */
#define R128_XCLK_CNTL                    0x000d /* PLL */
#define R128_XDLL_CNTL                    0x000c /* PLL */
#define R128_XPLL_CNTL                    0x000b /* PLL */

#define R128_SCALE_3D_CNTL                0x1a00

#endif
