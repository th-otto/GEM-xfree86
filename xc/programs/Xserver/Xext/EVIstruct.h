/* $TOG: EVIstruct.h /main/1 1997/11/24 16:48:35 kaleb $ */
/************************************************************
Copyright (c) 1997 by Silicon Graphics Computer Systems, Inc.
Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.
SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.
********************************************************/
#ifndef EVI_STRUCT_H
#define EVI_STRUCT_H
/*
 ******************************************************************************
 ** Per-ddx data
 ******************************************************************************
 */
typedef int (*GetVisualInfoProc)(
#if NeedNestedPrototypes
	VisualID32*,
	int,
	xExtendedVisualInfo**,
	int*,
	VisualID32**,
	int*
#endif
);
typedef void (*FreeVisualInfoProc)(
#if NeedNestedPrototypes
    xExtendedVisualInfo*,
    VisualID32*
#endif
);
typedef struct _EviPrivRec {
    GetVisualInfoProc getVisualInfo;
    FreeVisualInfoProc freeVisualInfo;
} EviPrivRec, *EviPrivPtr;
extern EviPrivPtr eviDDXInit();
extern void eviDDXReset();
#endif /* EVI_STRUCT_H */
