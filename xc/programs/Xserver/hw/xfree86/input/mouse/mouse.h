/* $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.h,v 1.8 2000/02/10 22:33:42 dawes Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

#ifndef _X_MOUSE_H
#define _X_MOUSE_H

/* Private interface for the mouse driver. */

/* Protocol IDs.  These are for internal use only. */
typedef enum {
    PROT_UNKNOWN = -2,
    PROT_UNSUP = -1,		/* protocol is not supported */
    PROT_MS = 0,
    PROT_MSC,
    PROT_MM,
    PROT_LOGI,
    PROT_LOGIMAN,
    PROT_MMHIT,
    PROT_GLIDE,
    PROT_IMSERIAL,
    PROT_THINKING,
    PROT_ACECAD,
    PROT_PS2,
    PROT_IMPS2,
    PROT_EXPPS2,
    PROT_THINKPS2,
    PROT_MMPS2,
    PROT_GLIDEPS2,
    PROT_NETPS2,
    PROT_NETSCPS2,
    PROT_BM,
    PROT_AUTO,
    PROT_SYSMOUSE,
    PROT_NUMPROTOS	/* This must always be last. */
} ProtocolID;

typedef struct {
    const char *	name;
    int			class;
    const char **	defaults;
    ProtocolID		id;
} MouseProtocolRec, *MouseProtocolPtr;

/* mouse proto flags */
#define MPF_NONE		0x00
#define MPF_SAFE		0x01

/* pnp.c */
int MouseGetPnpProtocol(InputInfoPtr pInfo);

#endif /* _X_MOUSE_H */
