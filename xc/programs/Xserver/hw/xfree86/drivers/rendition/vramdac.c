/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vramdac.c,v 1.9 2000/02/25 21:03:06 dawes Exp $ */
/*
 * includes
 */

#include "rendition.h"
#include "vramdac.h"
#include "vos.h"
#include "v1kregs.h"
#include "v2kregs.h"

/*
 * defines
 */

#undef DEBUG

/* directly accessable RAMDAC registers */
#define BT485_WRITE_ADDR        0x00
#define BT485_RAMDAC_DATA       0x01    
#define BT485_PIXEL_MASK        0x02
#define BT485_READ_ADDR         0x03
#define BT485_CURS_WR_ADDR      0x04
#define BT485_CURS_DATA         0x05
#define BT485_COMMAND_REG_0     0x06
#define BT485_CURS_RD_ADDR      0x07
#define BT485_COMMAND_REG_1     0x08
#define BT485_COMMAND_REG_2     0x09
#define BT485_STATUS_REG        0x0a
#define BT485_CURS_RAM_DATA     0x0b
#define BT485_CURS_X_LOW        0x0c
#define BT485_CURS_X_HIGH       0x0d
#define BT485_CURS_Y_LOW        0x0e
#define BT485_CURS_Y_HIGH       0x0f

/* indirectly accessable ramdac registers */
#define BT485_COMMAND_REG_3     0x01

/* bits in command register 0 */
#define BT485_CR0_EXTENDED_REG_ACCESS   0x80
#define BT485_CR0_SCLK_SLEEP_DISABLE    0x40
#define BT485_CR0_BLANK_PEDESTAL        0x20
#define BT485_CR0_SYNC_ON_BLUE          0x10
#define BT485_CR0_SYNC_ON_GREEN         0x08
#define BT485_CR0_SYNC_ON_RED           0x04
#define BT485_CR0_8_BIT_DAC             0x02
#define BT485_CR0_SLEEP_ENABLE          0x01

/* bits in command register 1 */
#define BT485_CR1_24BPP             0x00
#define BT485_CR1_16BPP             0x20
#define BT485_CR1_8BPP              0x40
#define BT485_CR1_4BPP              0x60
#define BT485_CR1_1BPP              0x80
#define BT485_CR1_BYPASS_CLUT       0x10
#define BT485_CR1_565_16BPP         0x08
#define BT485_CR1_555_16BPP         0x00
#define BT485_CR1_1_TO_1_16BPP      0x04
#define BT485_CR1_2_TO_1_16BPP      0x00
#define BT485_CR1_PD7_PIXEL_SWITCH  0x02
#define BT485_CR1_PIXEL_PORT_CD     0x01
#define BT485_CR1_PIXEL_PORT_AB     0x00

/* bits in command register 2 */
#define BT485_CR2_SCLK_DISABLE      0x80
#define BT485_TEST_PATH_SELECT      0x40
#define BT485_PIXEL_INPUT_GATE      0x20
#define BT485_PIXEL_CLK_SELECT      0x10
#define BT485_INTERLACE_SELECT      0x08
#define BT485_16BPP_CLUT_PACKED     0x04
#define BT485_X_WINDOW_CURSOR       0x03
#define BT485_2_COLOR_CURSOR        0x02
#define BT485_3_COLOR_CURSOR        0x01
#define BT485_DISABLE_CURSOR        0x00
#define BT485_CURSOR_MASK           0x03

/* bits in command register 3 */
#define BT485_4BPP_NIBBLE_SWAP      0x10
#define BT485_CLOCK_DOUBLER         0x08
#define BT485_64_BY_64_CURSOR       0x04
#define BT485_32_BY_32_CURSOR       0x00
#define BT485_SIZE_MASK             0x04

/* special constants for the Brooktree BT485 RAMDAC */
#define BT485_INPUT_LIMIT           110000000 



/*
 * local function prototypes
 */

static void Bt485_write_masked(vu16 port, vu8 reg, vu8 mask, vu8 data);
static void Bt485_write_cmd3_masked(vu16 port, vu8 mask, vu8 data);
static vu8 Bt485_read_masked(vu16 port, vu8 reg, vu8 mask);
static vu8 Bt485_read_cmd3_masked(vu16 port, vu8 mask);



/*
 * global data
 */

int Cursor_size=0;



/*
 * functions
 */

/*
 * int v_initdac(ScrnInfoPtr pScreenInfo, vu8 bpp, vu8 doubleclock)
 *
 * Used to initialize the ramdac. Palette-bypass is dis-/enabled with respect
 * to the color depth, the cursor is disabled by default. If needed (i.e. if
 * the corresponding field in the v_board_t struct is set), the clock doubling
 * is turned on.
 */
int
v_initdac(ScrnInfoPtr pScreenInfo, vu8 bpp, vu8 doubleclock)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob=pRendition->board.io_base+RAMDACBASEADDR;
    vu8 cmd3_data=0;

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_initdac called\n");
#endif

    if (doubleclock)
        cmd3_data|=BT485_CLOCK_DOUBLER;

    switch (bpp) {
        case 1:
        case 4:
			ErrorF("%s %s: color depth %d not (yet ?) supported\n",
				X_CONFIG, /*vga256InfoRec.name*/"Rendition", bpp);
			return -1;

        case 8:
            v_out8(iob+BT485_COMMAND_REG_0, BT485_CR0_EXTENDED_REG_ACCESS |
             	                            BT485_CR0_8_BIT_DAC);
            v_out8(iob+BT485_COMMAND_REG_1, BT485_CR1_8BPP |
                                            BT485_CR1_PIXEL_PORT_AB);
            v_out8(iob+BT485_COMMAND_REG_2, BT485_PIXEL_INPUT_GATE |
                                            BT485_DISABLE_CURSOR);
            break;

        case 15:
            v_out8(iob+BT485_COMMAND_REG_0, BT485_CR0_EXTENDED_REG_ACCESS |
                                            BT485_CR0_8_BIT_DAC);
            v_out8(iob+BT485_COMMAND_REG_1, BT485_CR1_16BPP |
                                            BT485_CR1_BYPASS_CLUT |
                                            BT485_CR1_555_16BPP |
                                            BT485_CR1_2_TO_1_16BPP |
                                            BT485_CR1_PIXEL_PORT_AB);
            v_out8(iob+BT485_COMMAND_REG_2, BT485_PIXEL_INPUT_GATE |
                                            BT485_DISABLE_CURSOR);
            break;

        case 16:
            v_out8(iob+BT485_COMMAND_REG_0, BT485_CR0_EXTENDED_REG_ACCESS |
                                            BT485_CR0_8_BIT_DAC);
            v_out8(iob+BT485_COMMAND_REG_1, BT485_CR1_16BPP |
                                            BT485_CR1_BYPASS_CLUT |
                                            BT485_CR1_565_16BPP |
                                            BT485_CR1_2_TO_1_16BPP |
                                            BT485_CR1_PIXEL_PORT_AB);
            v_out8(iob+BT485_COMMAND_REG_2, BT485_PIXEL_INPUT_GATE |
                                            BT485_DISABLE_CURSOR);
            break;

        case 32:
            v_out8(iob+BT485_COMMAND_REG_0, BT485_CR0_EXTENDED_REG_ACCESS |
                                            BT485_CR0_8_BIT_DAC);
            v_out8(iob+BT485_COMMAND_REG_1, BT485_CR1_24BPP |
                                            BT485_CR1_BYPASS_CLUT |
                                            BT485_CR1_PIXEL_PORT_AB);
            v_out8(iob+BT485_COMMAND_REG_2, BT485_PIXEL_INPUT_GATE |
                                            BT485_DISABLE_CURSOR);
            break;

        default:
            ErrorF( "%s %s: Color depth not supported (%d bpp)\n",
	    	    X_CONFIG, /*vga256InfoRec.name*/"Rendition", bpp);
            return -1;
            break;
    }

    v_out8(iob+BT485_WRITE_ADDR, BT485_COMMAND_REG_3);
    v_out8(iob+BT485_STATUS_REG, cmd3_data);
/*
    Bt485_write_masked(iob, BT485_COMMAND_REG_0, 0x7f, 0x00);
*/
    v_out8(iob+BT485_PIXEL_MASK, 0xff);

    return 0;
}



/*
 * void v_enablecursor(ScrnInfoPtr pScreenInfo, int type, int size)
 *
 * Used to enable the hardware cursor. Size indicates, whether to use no cursor
 * at all, a 32x32 or a 64x64 cursor. The type selects a two-color, three-color
 * or X-window-like cursor. Valid values are defined in vramdac.h. 
 *
 */
void
v_enablecursor(ScrnInfoPtr pScreenInfo, int type, int size)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

    static vu8 ctypes[]={ BT485_DISABLE_CURSOR, BT485_2_COLOR_CURSOR,
                      BT485_3_COLOR_CURSOR, BT485_X_WINDOW_CURSOR };
    static vu8 csizes[]={ BT485_32_BY_32_CURSOR, BT485_64_BY_64_CURSOR };
  
    vu16 iob=pRendition->board.io_base+RAMDACBASEADDR;

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_enablecursor called type=0x%x\n",type);
#endif

#if 0
    /* ensure proper ranges */
    size=1; /* Enforce 64x64 Cursor */
#endif

    /* type goes to command register 2 */
    Bt485_write_masked(iob, BT485_COMMAND_REG_2, ~BT485_CURSOR_MASK, 
                       ctypes[type]);
  
    /* size is in command register 3 */
    Bt485_write_cmd3_masked(iob, ~BT485_SIZE_MASK, csizes[size]);

    if (type)
      Cursor_size=(size ? 64 : 32);

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_enablecursor Exit\n");
#endif

}




/*
 * void v_movecursor(ScrnInfoPtr pScreenInfo, vu16 x, vu16 y, vu8 xo, vu8 yo)
 *
 * Moves the cursor to the specified location. To hide the cursor, call
 * this routine with x=0x0 and y=0x0.
 *
 */
void
v_movecursor(ScrnInfoPtr pScreenInfo, vu16 x, vu16 y, vu8 xo, vu8 yo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob=pRendition->board.io_base+RAMDACBASEADDR;

    x+=Cursor_size-xo;
    y+=Cursor_size-yo;

    v_out8(iob+BT485_CURS_X_LOW, x&0xff);
    v_out8(iob+BT485_CURS_X_HIGH, (x>>8)&0x0f);
    v_out8(iob+BT485_CURS_Y_LOW, y&0xff);
    v_out8(iob+BT485_CURS_Y_HIGH, (y>>8)&0x0f);
}



/*
 * void v_setcursorcolor(ScrnInfoPtr pScreenInfo, vu32 bg, vu32 fg)
 *
 * Sets the color of the cursor -- should be revised for use with 3 colors!
 *
 */
void
v_setcursorcolor(ScrnInfoPtr pScreenInfo, vu32 fg, vu32 bg)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob=pRendition->board.io_base+RAMDACBASEADDR;

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_setcursorcolor called FG=0x%x BG=0x%x\n",
	    fg,bg);
#endif

    v_out8(iob+BT485_CURS_WR_ADDR, 0x00);

    /* load the cursor color 0 */
    v_out8(iob+BT485_CURS_DATA, 0x00);
    v_out8(iob+BT485_CURS_DATA, 0x00);
    v_out8(iob+BT485_CURS_DATA, 0x00);

    /* load the cursor color 1 */
    v_out8(iob+BT485_CURS_DATA, (fg>>16)&0xff);
    v_out8(iob+BT485_CURS_DATA, (fg>>8)&0xff);
    v_out8(iob+BT485_CURS_DATA, fg&0xff);

    /* load the cursor color 2 */
    v_out8(iob+BT485_CURS_DATA, 0x00);
    v_out8(iob+BT485_CURS_DATA, 0x00);
    v_out8(iob+BT485_CURS_DATA, 0x00);

    /* load the cursor color 3 */
    v_out8(iob+BT485_CURS_DATA, (bg>>16)&0xff);
    v_out8(iob+BT485_CURS_DATA, (bg>>8)&0xff);
    v_out8(iob+BT485_CURS_DATA, bg&0xff);
}



/*
 * Oh god, this code is quite a mess ... should be re-written soon.
 * But for now I'm happy it works ;) <ml> 
 *
 */
void
v_loadcursor(ScrnInfoPtr pScreenInfo, vu8 size, vu8 *cursorimage)
{
    int c, bytes, row;
    vu8 *src = cursorimage;
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob=pRendition->board.io_base+RAMDACBASEADDR;
    vu8 tmp;
    vu8 memend; /* Added for byte-swap fix */

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_loadcursor called\n");
#endif

    if (NULL == cursorimage) 
        return;

    /* Following two lines added for the byte-swap fix */
    memend = v_in8(pRendition->board.io_base + MEMENDIAN);
    v_out8(pRendition->board.io_base + MEMENDIAN, MEMENDIAN_HW);

    size&=1;
    if (size)
        bytes=64;
    else
        bytes=32;
    bytes=(bytes*bytes)>>3;

    if (pRendition->board.chip == V1000_DEVICE) {
      /* now load the cursor data into the cursor ram */

      tmp=v_in8(iob+BT485_COMMAND_REG_0)&0x7f;
      v_out8(iob+BT485_COMMAND_REG_0, tmp|0x80);

      v_out8(iob+BT485_WRITE_ADDR, BT485_COMMAND_REG_3);

      tmp=v_in8(iob+BT485_STATUS_REG)&0xf8;
      v_out8(iob+BT485_STATUS_REG, tmp|(size<<2));
      v_out8(iob+BT485_WRITE_ADDR, 0x00);

      /* output cursor image */
      src=cursorimage;
      
      /* First plane data */
      for (c=0; c<bytes; c++)  {
        v_out8(iob+BT485_CURS_RAM_DATA, *src);
        src+=2;
      }

      /* Second plane data */
      src=cursorimage+1;
      for (c=0; c<bytes; c++)  {
        v_out8(iob+BT485_CURS_RAM_DATA, *src);
        src+=2;
      }
    }
    else {
      /* V2x00 HW-Cursor, supports only 64x64x2 size */

      v_out32(iob+CURSORBASE, pRendition->board.hwcursor_membase);

      /* First plane data */
      for (row=0; row<64; row++)
	for (c=0, src=cursorimage+1+16*row; c<8; c++, src+=2)
	  v_write_memory8(pRendition->board.vmem_base, 16*(63-row)+c,
     			  (c&1)?(*(src-2)):(*(src+2)));
      /* Second plane data */
      for (row=0; row<64; row++)
	for (c=0, src=cursorimage+16*row; c<8; c++, src+=2)
	  v_write_memory8(pRendition->board.vmem_base, 8+16*(63-row)+c,
			  (c&1)?(*(src-2)):(*(src+2)));

    }
    /* Following line added for the byte-swap fix */
    v_out8(pRendition->board.io_base + MEMENDIAN, memend);
}



/* NOTE: count is the actual number of colors decremented by 1 */

void
v_setpalette(ScrnInfoPtr pScreenInfo, int numColors, int *indices,
		LOCO *colors, VisualPtr pVisual)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob=pRendition->board.io_base;
    vu32 crtc_status;
    int i, index;
    int c;

#ifdef DEBUG
    ErrorF ("Rendition: Debug v_setpalette called\n");
#endif

    while (1) {
        crtc_status=v_in32(iob+CRTCSTATUS);
        if (crtc_status & CRTCSTATUS_VERT_SYNC) 
            break;
    };

    iob+=RAMDACBASEADDR;

    for (i = 0; i < numColors; i++) {
        index = indices[i];
	v_out8(iob+BT485_WRITE_ADDR, index);

        v_out8(iob+BT485_RAMDAC_DATA, colors[index].red);
        v_out8(iob+BT485_RAMDAC_DATA, colors[index].green);
        v_out8(iob+BT485_RAMDAC_DATA, colors[index].blue);
    }



}




/*
 * local functions
 */

/*
 * static void Bt485_write_masked(vu16 port, vu8 reg, vu8 mask, vu8 data)
 *
 *
 */
static void
Bt485_write_masked(vu16 port, vu8 reg, vu8 mask, vu8 data)
{
    vu8 tmp;

    tmp=v_in8(port+reg)&mask;
    v_out8(port+reg, tmp|data);
}



/*
 * static void Bt485_write_cmd3_masked(vu16 port, vu8 mask, vu8 data)
 *
 *
 */
static void
Bt485_write_cmd3_masked(vu16 port, vu8 mask, vu8 data)
{
/*
    Bt485_write_masked(port, BT485_COMMAND_REG_0, 0x7f, 0x80);
*/
    v_out8(port+BT485_WRITE_ADDR, BT485_COMMAND_REG_3);
    Bt485_write_masked(port, BT485_STATUS_REG, mask, data);
/*
    Bt485_write_masked(port, BT485_COMMAND_REG_0, 0x7f, 0x00);
*/
}



/*
 * static vu8 Bt485_read_masked(vu16 port, vu8 reg, vu8 mask)
 *
 *
 */
static vu8
Bt485_read_masked(vu16 port, vu8 reg, vu8 mask)
{
    return v_in8(port+reg)&mask;
}



/*
 * static vu8 Bt485_read_cmd3_masked(vu16 port, vu8 mask)
 *
 *
 */
static vu8
Bt485_read_cmd3_masked(vu16 port, vu8 mask)
{
    vu8 value;

    Bt485_write_masked(port, BT485_COMMAND_REG_0, 0x7f, 0x80);
    v_out8(port+BT485_WRITE_ADDR, BT485_COMMAND_REG_3);
    value=Bt485_read_masked(port, BT485_STATUS_REG, mask);
    Bt485_write_masked(port, BT485_COMMAND_REG_0, 0x7f, 0x00);

    return value;
}



/*
 * end of file vramdac.c
 */
