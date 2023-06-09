/*-
 * Copyright (c) 1998-1999 Shunsuke Akiyama <akiyama@jp.FreeBSD.org>.
 * All rights reserved.
 * Copyright (c) 1998-1999 X-TrueType Server Project, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: fontcache.h,v 1.12 1999/01/09 06:24:30 akiyama Exp $
 */

#ifndef _FONTCACHE_H_
#define _FONTCACHE_H_

#include "fontmisc.h"
#include "fontstruct.h"
#include "fcqueue.h"

/* constant declarations */

#ifndef FC_DEFAULT_CACHE_SIZE
#define FC_DEFAULT_CACHE_SIZE	1024	/* in KB */
#endif

#define FC_CACHE_BALANCE	70	/* in percentage */
#define FC_BALANCE_LOW		10	/* in percentage */
#define FC_BALANCE_HI		90	/* in percentage */
#define FC_BALANCE_DIFFS	5	/* in percentage */

#define FC_SMALL_BITMAP_SIZE	128

#define FC_MEM_HASH_SIZE	256
#define FC_MEM_HASH_MASK	(FC_MEM_HASH_SIZE - 1)

#define FC_PURGE_PER_SCAN	2

/* data type declarations */

struct cache_entry;
struct fcbitmap;

TAILQ_HEAD(fchash_head, cache_entry);
TAILQ_HEAD(fcmem_head, fcbitmap);

struct fcbitmap {
    TAILQ_ENTRY(fcbitmap) b_hash;
    struct cache_entry *b_entry;
    int size;
    int key;
};

struct fc_entry_vfuncs {
    void (*f_private_dispose)(void *f_private);
};

struct cache_entry {
    TAILQ_ENTRY(cache_entry) c_hash;	/* Hash chain. */
    TAILQ_ENTRY(cache_entry) c_lru;	/* Font cache LRU list chain. */
    struct fchash_head *c_head;		/* Pointer to head. */
    int key;				/* hash key */
    CharInfoRec charInfo;		/* CharInfo record */
    struct fcbitmap *bmp;
    void *f_private;                    /* Font-specific private data */
    struct fc_entry_vfuncs *vfuncs;     /* virtual function table */
    int bitmapsize;			/* Bitmap size */
    char bitmap[FC_SMALL_BITMAP_SIZE];	/* Small bitmap data area */
};

struct fchash {
    int size;
    int mask;
    struct fchash_head *head;
};

struct fcmemhash {
    int index;
    struct fcmem_head head[FC_MEM_HASH_SIZE];
};

struct fc_cache_settings {
    long	himark;
    long	lowmark;
    long	balance;
};

struct fc_cache_stat {
    long	hits;
    long	misshits;
    long	purged;
    long	usage;
};

struct fc_cache_statistics {
    long			purge_runs;
    long			purge_stat;
    long			balance;
    struct fc_cache_stat	f;
    struct fc_cache_stat	v;
};

typedef struct fcbitmap FontCacheBitmap, *FontCacheBitmapPtr;
typedef struct cache_entry FontCacheEntry, *FontCacheEntryPtr;
typedef struct fchash_head FontCacheHead, *FontCacheHeadPtr;
typedef struct fcmem_head FontCacheBitmapHead, *FontCacheBitmapHeadPtr;
typedef struct fchash FCCB, *FCCBPtr;
typedef struct fcmemhash FCBCB, *FCBCBPtr;
typedef struct fc_cache_settings FontCacheSettings, *FontCacheSettingsPtr;
typedef struct fc_cache_statistics FontCacheStatistics, *FontCacheStatisticsPtr;

/* Function prototypes */

int			FontCacheInitialize();
FCCBPtr			FontCacheOpenCache(void * /* arg */);
void			FontCacheCloseCache(FCCBPtr /* this */);
FontCacheEntryPtr	FontCacheGetEntry();
int			FontCacheSearchEntry(FCCBPtr /* this */, int /* key */,
					     FontCacheEntryPtr * /* value */);
int			FontCacheInsertEntry(FCCBPtr /* this */, int /* key */,
					     FontCacheEntryPtr /* entry */);
int			FontCacheGetBitmap(FontCacheEntryPtr /* entry */,
					   int /* size */);

int		FontCacheChangeSettings(FontCacheSettingsPtr /* cinfo */);
void		FontCacheGetSettings(FontCacheSettingsPtr /* cinfo */);
void		FontCacheGetStatistics(FontCacheStatisticsPtr /* cstats */);

#endif /* _FONTCACHE_H_ */
