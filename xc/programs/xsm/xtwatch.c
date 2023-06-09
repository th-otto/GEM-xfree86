/* $TOG: xtwatch.c /main/9 1998/02/09 14:16:15 kaleb $ */
/******************************************************************************

Copyright 1993, 1998  The Open Group

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
******************************************************************************/
/* $XFree86: xc/programs/xsm/xtwatch.c,v 1.3 1999/03/07 14:23:44 dawes Exp $ */

#include <X11/ICE/ICElib.h>
#include <X11/Intrinsic.h>
#include "xsm.h"
#include "xtwatch.h"

static void _XtIceWatchProc(IceConn ice_conn, IcePointer client_data, 
			    Bool opening, IcePointer *watch_data );
static void _XtProcessIceMsgProc(XtPointer client_data, int *source, 
				 XtInputId *id);


Status
InitWatchProcs(XtAppContext appContext)
{

    return (IceAddConnectionWatch (_XtIceWatchProc, (IcePointer) appContext));
}


static void
_XtIceWatchProc(IceConn ice_conn, IcePointer client_data, Bool opening, 
		IcePointer *watch_data)
{
    if (opening)
    {
	XtAppContext appContext = (XtAppContext) client_data;

	*watch_data = (IcePointer) XtAppAddInput (
	    appContext,
	    IceConnectionNumber (ice_conn),
            (XtPointer) XtInputReadMask,
	    _XtProcessIceMsgProc,
	    (XtPointer) ice_conn);
    }
    else
    {
	XtRemoveInput ((XtInputId) *watch_data);
    }
}


static void
_XtProcessIceMsgProc(XtPointer client_data, int *source, XtInputId *id)
{
    IceConn			ice_conn = (IceConn) client_data;
    IceProcessMessagesStatus	status;

    status = IceProcessMessages (ice_conn, NULL, NULL);

    if (status == IceProcessMessagesIOError)
    {
	List *cl;
	int found = 0;

	if (verbose)
	{
	    printf ("IO error on connection (fd = %d)\n",
	        IceConnectionNumber (ice_conn));
	    printf ("\n");
	}

	for (cl = ListFirst (RunningList); cl; cl = ListNext (cl))
	{
	    ClientRec *client = (ClientRec *) cl->thing;

	    if (client->ice_conn == ice_conn)
	    {
		CloseDownClient (client);
		found = 1;
		break;
	    }
	}
	 
	if (!found)
	{
	    /*
	     * The client must have disconnected before it was added
	     * to the session manager's running list (i.e. before the
	     * NewClientProc callback was invoked).
	     */

	    IceSetShutdownNegotiation (ice_conn, False);
	    IceCloseConnection (ice_conn);
	}
    }
}
