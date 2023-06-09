/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_include.h,v 1.7 2000/02/25 21:02:56 dawes Exp $ */

#ifndef __NV_INCLUDE_H__
#define __NV_INCLUDE_H__

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "compiler.h"
#include "xf86_ansic.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

#include "micmap.h"

#include "xf86DDC.h"

#include "xf86RAC.h"

#include "nv_const.h"
#ifndef NV_USE_FB
/*
 * If using cfb, cfb.h is required.  Select the others for the bpp values
 * the driver supports.
 */
#define PSZ 8   /* needed for cfb.h */
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
#else
#include "fb.h"
#endif

#include "xaa.h"
#include "xf86cmap.h"
#include "shadowfb.h"
#include "fbdevhw.h"

#include "xf86xv.h"
#include "Xv.h"

#include "vgaHW.h"

#include "xf86Cursor.h"
#include "xf86DDC.h"

#include "region.h"

#include "nv_local.h"
#include "nv_type.h"
#include "nv_proto.h"

#endif /* __NV_INCLUDE_H__ */
