/* $TOG: ErrHndlr.c /main/14 1998/02/06 17:20:03 kaleb $ */
/*

Copyright 1986, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/ErrHndlr.c,v 1.3 1999/03/02 11:49:21 dawes Exp $ */

#include "Xlibint.h"

extern int _XDefaultError();
extern int _XDefaultIOError();
/* 
 * XErrorHandler - This procedure sets the X non-fatal error handler
 * (_XErrorFunction) to be the specified routine.  If NULL is passed in
 * the original error handler is restored.
 */
 
#if NeedFunctionPrototypes
XErrorHandler XSetErrorHandler(XErrorHandler handler)
#else
XErrorHandler XSetErrorHandler(handler)
    register XErrorHandler handler;
#endif
{
    int (*oldhandler)();

    _XLockMutex(_Xglobal_lock);
    oldhandler = _XErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultError;

    if (handler != NULL) {
	_XErrorFunction = handler;
    }
    else {
	_XErrorFunction = _XDefaultError;
    }
    _XUnlockMutex(_Xglobal_lock);

    return (XErrorHandler) oldhandler;
}

/* 
 * XIOErrorHandler - This procedure sets the X fatal I/O error handler
 * (_XIOErrorFunction) to be the specified routine.  If NULL is passed in 
 * the original error handler is restored.
 */
 
#if NeedFunctionPrototypes
XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler)
#else
XIOErrorHandler XSetIOErrorHandler(handler)
    register XIOErrorHandler handler;
#endif
{
    int (*oldhandler)();

    _XLockMutex(_Xglobal_lock);
    oldhandler = _XIOErrorFunction;

    if (!oldhandler)
	oldhandler = _XDefaultIOError;

    if (handler != NULL) {
	_XIOErrorFunction = handler;
    }
    else {
	_XIOErrorFunction = _XDefaultIOError;
    }
    _XUnlockMutex(_Xglobal_lock);

    return (XIOErrorHandler) oldhandler;
}
