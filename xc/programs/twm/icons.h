/*
 * 
Copyright 1989, 1998  The Open Group

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
 * */
/* $XFree86: xc/programs/twm/icons.h,v 1.3 1999/02/20 15:07:24 hohndel Exp $ */

/**********************************************************************
 *
 * $TOG: icons.h /main/6 1998/02/09 13:48:29 kaleb $
 *
 * Icon releated definitions
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef ICONS_H
#define ICONS_H

typedef struct IconRegion
{
    struct IconRegion	*next;
    int			x, y, w, h;
    int			grav1, grav2;
    int			stepx, stepy;	/* allocation granularity */
    struct IconEntry	*entries;
} IconRegion;

typedef struct IconEntry
{
    struct IconEntry	*next;
    int			x, y, w, h;
    TwmWindow		*twm_win;
    short 		used;
}IconEntry;

extern int roundUp ( int v, int multiple );
extern void PlaceIcon ( TwmWindow *tmp_win, int def_x, int def_y, 
		       int *final_x, int *final_y );
extern void IconUp ( TwmWindow *tmp_win );
extern void IconDown ( TwmWindow *tmp_win );
extern void AddIconRegion ( char *geom, int grav1, int grav2, 
			   int stepx, int stepy );
extern void CreateIconWindow ( TwmWindow *tmp_win, int def_x, int def_y );

#endif /* ICONS_H */
