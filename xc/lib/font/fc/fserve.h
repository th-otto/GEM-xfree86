/* $XConsortium: fserve.h,v 1.7 93/08/24 18:49:12 gildea Exp $ */
/*
 * Copyright 1990 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Network Computing Devices not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Network Computing
 * Devices makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * NETWORK COMPUTING DEVICES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL NETWORK COMPUTING DEVICES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  	Dave Lemke, Network Computing Devices, Inc
 *
 */
/* $XFree86: xc/lib/font/fc/fserve.h,v 1.3 1999/12/13 02:52:52 robin Exp $ */

#ifndef _FSERVE_H_
#define _FSERVE_H_
/*
 * font server data structures
 */

/* types of block records */
#define	FS_OPEN_FONT		1
#define	FS_LOAD_GLYPHS		2
#define	FS_LIST_FONTS		3
#define	FS_LIST_WITH_INFO	4

/* states of OpenFont */
#define	FS_OPEN_REPLY		0
#define	FS_INFO_REPLY		1
#define	FS_EXTENT_REPLY		2
#define	FS_GLYPHS_REPLY		3
#define	FS_DONE_REPLY		4
#define FS_DEPENDING		5

/* status of ListFontsWithInfo */
#define	FS_LFWI_WAITING		0
#define	FS_LFWI_REPLY		1
#define	FS_LFWI_FINISHED	2

/* states of connection */
#define FS_CONN_CLOSED		0
#define FS_CONN_CONNECTING	1
#define FS_CONN_READ_HEADER	2
#define FS_CONN_READ_DATA	3

#define	AccessDone	0x400

typedef struct _fs_font_data *FSFontDataPtr;
typedef struct _fs_blocked_font *FSBlockedFontPtr;
typedef struct _fs_blocked_glyphs *FSBlockedGlyphPtr;
typedef struct _fs_blocked_list *FSBlockedListPtr;
typedef struct _fs_blocked_list_info *FSBlockedListInfoPtr;
typedef struct _fs_block_data *FSBlockDataPtr;
typedef struct _fs_font_table *FSFontTablePtr;
typedef struct _fs_fpe_data *FSFpePtr;

typedef struct _fs_blocked_bitmaps *FSBlockedBitmapPtr;
typedef struct _fs_blocked_extents *FSBlockedExtentPtr;

extern void _fs_convert_char_info ( fsXCharInfo *src, xCharInfo *dst );
extern void _fs_free_props (FontInfoPtr pfi);
extern FontPtr fs_create_font (FontPathElementPtr   fpe,
			       char		    *name,
			       int		    namelen,
			       fsBitmapFormat	    format,
			       fsBitmapFormatMask   fmask);

extern int fs_load_all_glyphs ( FontPtr pfont );
extern int _fs_load_glyphs ( pointer client, FontPtr pfont, Bool range_flag, 
			     unsigned int nchars, int item_size, 
			     unsigned char *data );
extern void fs_register_fpe_functions ( void );
extern void check_fs_register_fpe_functions ( void );

/*
 * These should be declared elsewhere, but I'm concerned that moving them
 * would cause problems building other pieces
 */
extern FontPtr find_old_font (Font id);
extern int  set_font_authorizations (char **a, int *len, pointer client);
extern long   GetTimeInMillis (void);


#endif				/* _FSERVE_H_ */
