/* $TOG: rpcauth.c /main/10 1998/02/09 15:12:52 kaleb $ */
/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
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
/* $XFree86: xc/programs/Xserver/os/rpcauth.c,v 3.2 1998/12/13 07:37:48 dawes Exp $ */

/*
 * SUN-DES-1 authentication mechanism
 * Author:  Mayank Choudhary, Sun Microsystems
 */


#ifdef SECURE_RPC

#include "X.h"
#include "Xauth.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"

#include <rpc/rpc.h>

#if defined(DGUX)
#include <time.h>
#include <rpc/auth_des.h>
#endif /* DGUX */

#ifdef ultrix
#include <time.h>
#include <rpc/auth_des.h>
#endif

static enum auth_stat why;

static char * 
authdes_ezdecode(inmsg, len)
char *inmsg;
int  len;
{
    struct rpc_msg  msg;
    char            cred_area[MAX_AUTH_BYTES];
    char            verf_area[MAX_AUTH_BYTES];
    char            *temp_inmsg;
    struct svc_req  r;
    bool_t          res0, res1;
    XDR             xdr;
    SVCXPRT         xprt;

    temp_inmsg = (char *) xalloc(len);
    memmove(temp_inmsg, inmsg, len);

    memset((char *)&msg, 0, sizeof(msg));
    memset((char *)&r, 0, sizeof(r));
    memset(cred_area, 0, sizeof(cred_area));
    memset(verf_area, 0, sizeof(verf_area));

    msg.rm_call.cb_cred.oa_base = cred_area;
    msg.rm_call.cb_verf.oa_base = verf_area;
    why = AUTH_FAILED; 
    xdrmem_create(&xdr, temp_inmsg, len, XDR_DECODE);

    if ((r.rq_clntcred = (caddr_t) xalloc(MAX_AUTH_BYTES)) == NULL)
        goto bad1;
    r.rq_xprt = &xprt;

    /* decode into msg */
    res0 = xdr_opaque_auth(&xdr, &(msg.rm_call.cb_cred)); 
    res1 = xdr_opaque_auth(&xdr, &(msg.rm_call.cb_verf));
    if ( ! (res0 && res1) )
         goto bad2;

    /* do the authentication */

    r.rq_cred = msg.rm_call.cb_cred;        /* read by opaque stuff */
    if (r.rq_cred.oa_flavor != AUTH_DES) {
        why = AUTH_TOOWEAK;
        goto bad2;
    }
#ifdef SVR4
    if ((why = __authenticate(&r, &msg)) != AUTH_OK) {
#else
    if ((why = _authenticate(&r, &msg)) != AUTH_OK) {
#endif
            goto bad2;
    }
    return (((struct authdes_cred *) r.rq_clntcred)->adc_fullname.name); 

bad2:
    xfree(r.rq_clntcred);
bad1:
    return ((char *)0); /* ((struct authdes_cred *) NULL); */
}

static XID  rpc_id = (XID) ~0L;

static Bool
CheckNetName (addr, len, closure)
    unsigned char    *addr;
    int		    len;
    pointer	    closure;
{
    return (len == strlen ((char *) closure) &&
	    strncmp ((char *) addr, (char *) closure, len) == 0);
}

static char rpc_error[MAXNETNAMELEN+50];

XID
SecureRPCCheck (data_length, data, client, reason)
    register unsigned short	data_length;
    char	*data;
    ClientPtr client;
    char	**reason;
{
    char *fullname;
    
    if (rpc_id == (XID) ~0L) {
	*reason = "Secure RPC authorization not initialized";
    } else {
	fullname = authdes_ezdecode(data, data_length);
	if (fullname == (char *)0) {
	    sprintf(rpc_error, "Unable to authenticate secure RPC client (why=%d)", why);
	    *reason = rpc_error;
	} else {
	    if (ForEachHostInFamily (FamilyNetname, CheckNetName,
				     (pointer) fullname))
		return rpc_id;
	    else {
		sprintf(rpc_error, "Principal \"%s\" is not authorized to connect",
			fullname);
		*reason = rpc_error;
	    }
	}
    }
    return (XID) ~0L;
}
    

SecureRPCInit ()
{
    if (rpc_id == ~0L)
	AddAuthorization (9, "SUN-DES-1", 0, (char *) 0);
}

int
SecureRPCAdd (data_length, data, id)
unsigned short	data_length;
char	*data;
XID	id;
{
    if (data_length)
	AddHost ((pointer) 0, FamilyNetname, data_length, data);
    rpc_id = id;
}

int
SecureRPCReset ()
{
    rpc_id = (XID) ~0L;
}

XID
SecureRPCToID (data_length, data)
    unsigned short	data_length;
    char		*data;
{
    return rpc_id;
}

SecureRPCFromID (id, data_lenp, datap)
     XID id;
     unsigned short	*data_lenp;
     char	**datap;
{
    return 0;
}

SecureRPCRemove (data_length, data)
     unsigned short	data_length;
     char	*data;
{
    return 0;
}
#endif /* SECURE_RPC */
