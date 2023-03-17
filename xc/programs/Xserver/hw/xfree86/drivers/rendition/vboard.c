/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/vboard.c,v 1.9 2000/02/25 21:03:03 dawes Exp $ */
/*
 * includes
 */

#include "rendition.h"
#include "v1krisc.h"
#include "vboard.h"
#include "vloaduc.h"
#include "vos.h"



/* 
 * global data
 */

#include "cscode.h"

#if 0
/* Global imported during compile-time */
char MICROCODE_DIR [PATH_MAX] = MODULEDIR;
#endif


/*
 * local function prototypes
 */


/*
 * functions
 */
int
v_initboard(ScrnInfoPtr pScreenInfo)
{
  renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

  vu16 iob=pRendition->board.io_base;
  vu8 *vmb;
  vu32 offset;
  vu8 memendian;
  int c,pc;

  /* write "monitor" program to memory */
  v1k_stop(pScreenInfo);
  pRendition->board.csucode_base=0x800;
  memendian=v_in8(iob+MEMENDIAN);
  v_out8(iob+MEMENDIAN, MEMENDIAN_NO);

  /* Note that CS ucode must wait on address in csucode_base
   * when initialized for later context switch code to work. */

  ErrorF("Loading csucode @ 0x%x + 0x800\n", pRendition->board.vmem_base);
  vmb=pRendition->board.vmem_base;
  offset=pRendition->board.csucode_base;
  for (c=0; c<sizeof(csrisc)/sizeof(vu32); c++, offset+=sizeof(vu32))
    v_write_memory32(vmb, offset, csrisc[c]);

  /* Initialize the CS flip semaphore */
  v_write_memory32(vmb, 0x7f8, 0);
  v_write_memory32(vmb, 0x7fc, 0);

  /* Run the code we just transfered to the boards memory */
  /* ... and start accelerator */
  v1k_flushicache(pScreenInfo);

  v_out8(iob + STATEINDEX, STATEINDEX_PC);
  pc = v_in32(iob + STATEDATA);
  v1k_start(pScreenInfo, pRendition->board.csucode_base);

  /* Get on loading the ucode */
  v_out8(iob + STATEINDEX, STATEINDEX_PC);

  for (c = 0; c < 0xffffffL; c++){
    v1k_stop(pScreenInfo);
    pc = v_in32(iob + STATEDATA);
    v1k_continue(pScreenInfo);
    if (pc == pRendition->board.csucode_base)
      break;
  }
  if (pc != pRendition->board.csucode_base){
    ErrorF ("RENDITION: V_INITBOARD -- PC != CSUCODEBASE\n");
    ErrorF ("RENDITION: PC == 0x%x   --  CSU == 0x%x\n",pc,pRendition->board.csucode_base);
  }

  /* reset memory endian */
  v_out8(iob+MEMENDIAN, memendian);

#if 0
  if (V1000_DEVICE == pRendition->board.chip){
    c=v_load_ucfile(pScreenInfo, xf86strcat ((char *)MICROCODE_DIR,"v10002d.uc"));
  }
  else {
    /* V2x00 chip */
    c=v_load_ucfile(pScreenInfo, xf86strcat ((char *)MICROCODE_DIR,"v20002d.uc"));
  }

  if (c == -1) {
    ErrorF( "RENDITION: Microcode loading failed !!!\n");
    return 1;
  }

  pRendition->board.ucode_entry=c;

  ErrorF("UCode_Entry == 0x%x\n",pRendition->board.ucode_entry);
#endif
  /* Everything's OK */
  return 0;
}


int
v_resetboard(ScrnInfoPtr pScreenInfo)
{
  /*
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
  */
  v1k_softreset(pScreenInfo);
  return 0;
}



int
v_getmemorysize(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

#define PATTERN  0xf5faaf5f
#define START    0x12345678
#define ONEMEG   (1024L*1024L)
    vu32 offset;
    vu32 pattern;
    vu32 start;
    vu8 memendian;
#ifdef XSERVER
    vu8 modereg;

    modereg=v_in8(pRendition->board.io_base+MODEREG);
    v_out8(pRendition->board.io_base+MODEREG, NATIVE_MODE);
#endif

    /* no byte swapping */
    memendian=v_in8(pRendition->board.io_base+MEMENDIAN);
    v_out8(pRendition->board.io_base+MEMENDIAN, MEMENDIAN_NO);

    /* it looks like the v1000 wraps the memory; but for I'm not sure,
     * let's test also for non-writable offsets */
    start=v_read_memory32(pRendition->board.vmem_base, 0);
    v_write_memory32(pRendition->board.vmem_base, 0, START);
    for (offset=ONEMEG; offset<16*ONEMEG; offset+=ONEMEG) {
#ifdef DEBUG
        ErrorF( "Testing %d MB: ", offset/ONEMEG);
#endif
        pattern=v_read_memory32(pRendition->board.vmem_base, offset);
        if (START == pattern) {
#ifdef DEBUG
            ErrorF( "Back at the beginning\n");
#endif
            break;    
        }
        
        pattern^=PATTERN;
        v_write_memory32(pRendition->board.vmem_base, offset, pattern);
        
#ifdef DEBUG
        ErrorF( "%x <-> %x\n", (int)pattern, 
                    (int)v_read_memory32(pRendition->board.vmem_base, offset));
#endif

        if (pattern != v_read_memory32(pRendition->board.vmem_base, offset)) {
            offset-=ONEMEG;
            break;    
        }
        v_write_memory32(pRendition->board.vmem_base, offset, pattern^PATTERN);
    }
    v_write_memory32(pRendition->board.vmem_base, 0, start);

    if (16*ONEMEG <= offset)
        pRendition->board.mem_size=4*ONEMEG;
    else 
	    pRendition->board.mem_size=offset;

    /* restore default byte swapping */
    v_out8(pRendition->board.io_base+MEMENDIAN, MEMENDIAN_NO);

#ifdef XSERVER
    v_out8(pRendition->board.io_base+MODEREG, modereg);
#endif

    return pRendition->board.mem_size;
#undef PATTERN
#undef ONEMEG
}

void
v_check_csucode(ScrnInfoPtr pScreenInfo)
{
  renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
  vu16 iob=pRendition->board.io_base;
  vu8 *vmb;
  vu32 offset;
  int c;
  int memend;
  int mismatches=0;

  memend=v_in8(iob+MEMENDIAN);
  v_out8(iob+MEMENDIAN, MEMENDIAN_NO);

  ErrorF("Checking presence of csucode @ 0x%x + 0x800\n",
	 pRendition->board.vmem_base);

  if (0x800 != pRendition->board.csucode_base)
    ErrorF("pRendition->board.csucode_base == 0x%x\n",
	   pRendition->board.csucode_base);

  /* compare word by word */
  vmb=pRendition->board.vmem_base;
  offset=pRendition->board.csucode_base;
  for (c=0; c<sizeof(csrisc)/sizeof(vu32); c++, offset+=sizeof(vu32))
    if (csrisc[c] != v_read_memory32(vmb, offset)) {
      ErrorF("csucode mismatch in word %02d: 0x%08x should be 0x%08x\n",
	     c,
	     v_read_memory32(vmb, offset),
	     csrisc[c]);
      mismatches++;
    }
  ErrorF("Encountered %d out of %d possible mismatches\n",
	 mismatches,
	 sizeof(csrisc)/sizeof(vu32));

  v_out8(iob+MEMENDIAN, memend);
}


/*
 * local functions
 */



/*
 * end of file vboard.c
 */
