/* $TOG: patcache.c /main/11 1998/05/07 13:54:55 kaleb $ */

/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/font/util/patcache.c,v 3.3 1999/08/21 13:48:10 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include    <fontmisc.h>
#include    <fontstruct.h>

/*
 * Static sized hash table for looking up font name patterns
 *
 * LRU entries, reusing old entries
 */

#define NBUCKETS	16
#define NENTRIES	64

#define UNSET		(NENTRIES+1)

typedef unsigned char	EntryPtr;

typedef struct _FontPatternCacheEntry {
    struct _FontPatternCacheEntry   *next, **prev;
    short			    patlen;
    char			    *pattern;
    int				    hash;
    FontPtr			    pFont;	/* associated font */
} FontPatternCacheEntryRec, *FontPatternCacheEntryPtr;

typedef struct _FontPatternCache {
    FontPatternCacheEntryPtr	buckets[NBUCKETS];
    FontPatternCacheEntryRec	entries[NENTRIES];
    FontPatternCacheEntryPtr	free;
} FontPatternCacheRec;

/* Empty cache (for rehash) */
void
EmptyFontPatternCache (FontPatternCachePtr cache)
{
    int	    i;
    
    for (i = 0; i < NBUCKETS; i++)
	cache->buckets[i] = 0;
    for (i = 0; i < NENTRIES; i++)
    {
	cache->entries[i].next = &cache->entries[i+1];
	cache->entries[i].prev = 0;
	cache->entries[i].pFont = 0;
	xfree (cache->entries[i].pattern);
	cache->entries[i].pattern = 0;
	cache->entries[i].patlen = 0;
    }
    cache->free = &cache->entries[0];
    cache->entries[NENTRIES - 1].next = 0;
}

/* Create and initialize cache */
FontPatternCachePtr
MakeFontPatternCache (void)
{
    FontPatternCachePtr	cache;
    int			i;
    cache = (FontPatternCachePtr) xalloc (sizeof *cache);
    if (!cache)
	return 0;
    for (i = 0; i < NENTRIES; i++) {
	cache->entries[i].patlen = 0;
	cache->entries[i].pattern = 0;
	cache->entries[i].pFont = 0;
    }
    EmptyFontPatternCache (cache);
    return cache;
}

/* toss cache */
void
FreeFontPatternCache (FontPatternCachePtr cache)
{
    int	    i;

    for (i = 0; i < NENTRIES; i++)
	xfree (cache->entries[i].pattern);
    xfree (cache);
}

/* compute id for string */
static int
Hash (const char *string, int len)
{
    int	hash;

    hash = 0;
    while (len--)
	hash = (hash << 1) ^ *string++;
    if (hash < 0)
	hash = -hash;
    return hash;
}

/* add entry */
void
CacheFontPattern (FontPatternCachePtr cache, 
		  char *pattern, 
		  int patlen, 
		  FontPtr pFont)
{
    FontPatternCacheEntryPtr	e;
    char			*newpat;
    int				i;

    newpat = (char *) xalloc (patlen);
    if (!newpat)
	return;
    if (cache->free)
    {
	e = cache->free;
	cache->free = e->next;
    }
    else
    {
    	i = rand ();
    	if (i < 0)
	    i = -i;
    	i %= NENTRIES;
	e = &cache->entries[i];
	if (e->next)
	    e->next->prev = e->prev;
	*e->prev = e->next;
	xfree (e->pattern);
    }
    /* set pattern */
    memcpy (newpat, pattern, patlen);
    e->pattern = newpat;
    e->patlen = patlen;
    /* link to new hash chain */
    e->hash = Hash (pattern, patlen);
    i = e->hash % NBUCKETS;
    e->next = cache->buckets[i];
    if (e->next)
	e->next->prev = &(e->next);
    cache->buckets[i] = e;
    e->prev = &(cache->buckets[i]);
    e->pFont = pFont;
}

/* find matching entry */
FontPtr
FindCachedFontPattern (FontPatternCachePtr cache, 
		       char *pattern, 
		       int patlen)
{
    int				hash;
    int				i;
    FontPatternCacheEntryPtr	e;

    hash = Hash (pattern, patlen);
    i = hash % NBUCKETS;
    for (e = cache->buckets[i]; e; e = e->next)
    {
	if (e->patlen == patlen && e->hash == hash &&
	    !memcmp (e->pattern, pattern, patlen))
	{
	    return e->pFont;
	}
    }
    return 0;
}

void
RemoveCachedFontPattern (FontPatternCachePtr cache, 
			 FontPtr pFont)
{
    FontPatternCacheEntryPtr	e;
    int				i;

    for (i = 0; i < NENTRIES; i++)
    {
	if ((e = &cache->entries[i])->pFont == pFont)
	{
	    e->pFont = 0;
	    if (e->next)
		e->next->prev = e->prev;
	    *e->prev = e->next;
	    e->next = cache->free;
	    cache->free = e;
	    xfree (e->pattern);
	    e->pattern = 0;
	}
    }
}
