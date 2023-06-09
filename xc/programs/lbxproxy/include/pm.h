/* $TOG: pm.h /main/1 1997/09/12 14:28:33 barstow $ */

/*
Copyright (c) 1997  The Open Group

Copyright (c) 1996  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.
*/

#include <X11/ICE/ICElib.h>

extern IceConn PM_iceConn;
extern int proxy_manager_fd;
extern Bool proxyMngr;

extern Bool CheckForProxyManager (
#if NeedFunctionPrototypes
    void
#endif
);

extern void ConnectToProxyManager (
#if NeedFunctionPrototypes
    void
#endif
);

extern void SendGetProxyAddrReply (
#if NeedFunctionPrototypes
    IceConn /*requestor_iceConn*/,
    int /*status*/,
    char * /*addr*/,
    char * /*error*/
#endif
);

extern void HandleProxyManagerConnection (
#if NeedFunctionPrototypes
    void
#endif
);
