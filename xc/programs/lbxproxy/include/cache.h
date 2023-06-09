/* $TOG: cache.h /main/10 1998/02/10 18:17:09 kaleb $ */
/*
Copyright 1994, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices
 * or Digital not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Network Computing Devices and Digital
 * make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NETWORK COMPUTING DEVICES AND DIGITAL DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES OR DIGITAL BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _CACHE_H_
#define	_CACHE_H_

#define	CacheWasReset		1
#define	CacheEntryFreed		2
#define	CacheEntryOld		3

#define MAX_NUM_CACHES  2

typedef unsigned long CacheID;
typedef unsigned long Cache;
typedef void (*CacheFree) ();

typedef struct _cache *CachePtr;

extern Cache CacheInit(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    unsigned long /*maxsize*/
#endif
);

extern void CacheFreeCache(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cid*/
#endif
);

extern Bool CacheTrimNeeded(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cid*/
#endif
);

extern void CacheTrim(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cid*/
#endif
);

extern Bool CacheStoreMemory(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cid*/,
    CacheID /*id*/,
    pointer /*data*/,
    unsigned long /*size*/,
    CacheFree /*free_func*/,
    Bool /*can_flush*/
#endif
);

extern pointer CacheFetchMemory(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cid*/,
    CacheID /*id*/,
    Bool /*update*/
#endif
);

extern void CacheFreeMemory(
#if NeedFunctionPrototypes
    XServerPtr /*server*/,
    Cache /*cacheid*/,
    CacheID /*cid*/,
    Bool /*notify*/
#endif
);

#endif				/* _CACHE_H_ */
