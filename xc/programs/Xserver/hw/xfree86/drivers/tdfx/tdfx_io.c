/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tdfx/tdfx_io.c,v 1.4 2000/02/23 04:47:21 martin Exp $ */

/*
 * Authors:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *
 */

#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "tdfx.h"

#define minb(p) MMIO_IN8(pTDFX->MMIOBase, (p))
#define moutb(p,v) MMIO_OUT8(pTDFX->MMIOBase, (p),(v))
#define minl(p) MMIO_IN32(pTDFX->MMIOBase, (p))
#define moutl(p,v) MMIO_OUT32(pTDFX->MMIOBase, (p),(v))

static void TDFXWriteControlPIO(TDFXPtr pTDFX, int addr, char index, char val) {
  outb(pTDFX->PIOBase+addr, index);
  outb(pTDFX->PIOBase+addr+1, val);
}

static char TDFXReadControlPIO(TDFXPtr pTDFX, int addr, char index) {
  outb(pTDFX->PIOBase+addr, index);
  return inb(pTDFX->PIOBase+addr+1);
}

static void TDFXWriteLongPIO(TDFXPtr pTDFX, int addr, int val) {
  outl(pTDFX->PIOBase+addr, val);
}

static int TDFXReadLongPIO(TDFXPtr pTDFX, int addr) {
  return inl(pTDFX->PIOBase+addr);
}

void TDFXSetPIOAccess(TDFXPtr pTDFX) {
  if (!pTDFX->PIOBase)
    ErrorF("Can not set PIO Access before PIOBase\n");
  pTDFX->writeControl=TDFXWriteControlPIO;
  pTDFX->readControl=TDFXReadControlPIO;
  pTDFX->writeLong=TDFXWriteLongPIO;
  pTDFX->readLong=TDFXReadLongPIO;
}

static void TDFXWriteControlMMIO(TDFXPtr pTDFX, int addr, char index, char val) {
  moutb(addr, index);
  moutb(addr+1, val);
}

static char TDFXReadControlMMIO(TDFXPtr pTDFX, int addr, char index) {
  moutb(addr, index);
  return minb(addr+1);
}

void TDFXWriteLongMMIO(TDFXPtr pTDFX, int addr, int val) {
  moutl(addr, val);
}

int TDFXReadLongMMIO(TDFXPtr pTDFX, int addr) {
  return minl(addr);
}

void TDFXSetMMIOAccess(TDFXPtr pTDFX) {
  if (!pTDFX->MMIOBase)
    ErrorF("Can not set MMIO access before MMIOBase\n");
  pTDFX->writeControl=TDFXWriteControlMMIO;
  pTDFX->readControl=TDFXReadControlMMIO;
  pTDFX->writeLong=TDFXWriteLongMMIO;
  pTDFX->readLong=TDFXReadLongMMIO;
}

