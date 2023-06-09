/* $TOG: XAuth.h /main/5 1998/02/10 18:36:58 kaleb $ */
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

#ifndef _XAuth_h
#define _XAuth_h

extern int
GetXAuth(Display *dpy, RxXAuthentication auth_name, char *auth_data,
	 Bool trusted, XID group, unsigned int timeout, Bool want_revoke_event,
	 char **auth_string_ret, XSecurityAuthorization *auth_id_ret,
	 int *event_type_base_ret);

#endif /* _XAuth_h */
