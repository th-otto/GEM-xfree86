/* $TOG: NewNDest.c /main/16 1998/02/10 18:32:58 kaleb $ */
/*

Copyright 1996, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABIL-
ITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization from
The Open Group.

*/

#include "RxPlugin.h"

/***********************************************************************
 * Functions to init and free private members
 ***********************************************************************/

void
RxpNew(PluginInstance *This)
{
#ifdef PLUGIN_TRACE
    fprintf (stderr, "%s\n", "RxpNew");
#endif
    This->x_ui_auth_id = 0;
    This->x_print_auth_id = 0;
    This->app_group = 0;
    This->toplevel_widget = NULL;
    This->client_windows = NULL;
    This->nclient_windows = 0;
}

void
RxpDestroy(PluginInstance *This)
{
#ifdef PLUGIN_TRACE
    fprintf (stderr, "%s\n", "RxpDestroy");
#endif
    if (RxGlobal.dpy != NULL) {
	RxpWmDelWinHandler (This->toplevel_widget, (XtPointer) This, NULL, NULL);
	RxpRemoveDestroyCallback(This);
	if (This->x_ui_auth_id != 0)
	    XSecurityRevokeAuthorization(RxGlobal.dpy, This->x_ui_auth_id);
	if (This->app_group != None)
	    XagDestroyApplicationGroup (RxGlobal.dpy, This->app_group);
	RxpTeardown (This);
    }
    if (RxGlobal.pdpy != NULL) {
	if (This->x_print_auth_id != 0)
	    XSecurityRevokeAuthorization(RxGlobal.pdpy, This->x_print_auth_id);
    }
    if (This->client_windows) NPN_MemFree (This->client_windows);
    RxpNew (This);
}
