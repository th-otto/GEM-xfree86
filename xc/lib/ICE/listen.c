/* $TOG: listen.c /main/16 1998/02/06 13:57:14 kaleb $ */
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

Author: Ralph Mor,  X Consortium
******************************************************************************/

#include <X11/ICE/ICElib.h>
#include "ICElibint.h"
#include <X11/Xtrans.h>
#include <stdio.h>


Status
IceListenForConnections (countRet, listenObjsRet, errorLength, errorStringRet)

int		*countRet;
IceListenObj	**listenObjsRet;
int		errorLength;
char		*errorStringRet;

{
    struct _IceListenObj	*listenObjs;
    char			*networkId;
    int				fd, transCount, partial, i, j;
    Status			status = 1;
    XtransConnInfo		*transConns = NULL;


    if ((_IceTransMakeAllCOTSServerListeners (NULL, &partial,
	&transCount, &transConns) < 0) || (transCount < 1))
    {
	*listenObjsRet = NULL;
	*countRet = 0;

        strncpy (errorStringRet,
	    "Cannot establish any listening sockets", errorLength);

	return (0);
    }

    if ((listenObjs = (struct _IceListenObj *) malloc (
	transCount * sizeof (struct _IceListenObj))) == NULL)
    {
	for (i = 0; i < transCount; i++)
	    _IceTransClose (transConns[i]);
	free ((char *) transConns);
	return (0);
    }

    *countRet = 0;

    for (i = 0; i < transCount; i++)
    {
	networkId = _IceTransGetMyNetworkId (transConns[i]);

	if (networkId)
	{
	    listenObjs[*countRet].trans_conn = transConns[i];
	    listenObjs[*countRet].network_id = networkId;
		
	    (*countRet)++;
	}
    }

    if (*countRet == 0)
    {
	*listenObjsRet = NULL;

        strncpy (errorStringRet,
	    "Cannot establish any listening sockets", errorLength);

	status = 0;
    }
    else
    {
	*listenObjsRet = (IceListenObj *) malloc (
	    *countRet * sizeof (IceListenObj));

	if (*listenObjsRet == NULL)
	{
	    strncpy (errorStringRet, "Malloc failed", errorLength);

	    status = 0;
	}
	else
	{
	    for (i = 0; i < *countRet; i++)
	    {
		(*listenObjsRet)[i] = (IceListenObj) malloc (
		    sizeof (struct _IceListenObj));

		if ((*listenObjsRet)[i] == NULL)
		{
		    strncpy (errorStringRet, "Malloc failed", errorLength);

		    for (j = 0; j < i; j++)
			free ((char *) (*listenObjsRet)[j]);

		    free ((char *) *listenObjsRet);

		    status = 0;
		}
		else
		{
		    *((*listenObjsRet)[i]) = listenObjs[i];
		}
	    }
	}
    }

    if (status == 1)
    {
	if (errorStringRet && errorLength > 0)
	    *errorStringRet = '\0';
	
	for (i = 0; i < *countRet; i++)
	{
	    (*listenObjsRet)[i]->host_based_auth_proc = NULL;
	}
    }
    else
    {
	for (i = 0; i < transCount; i++)
	    _IceTransClose (transConns[i]);
    }

    free ((char *) listenObjs);
    free ((char *) transConns);

    return (status);
}



int
IceGetListenConnectionNumber (listenObj)

IceListenObj listenObj;

{
    return (_IceTransGetConnectionNumber (listenObj->trans_conn));
}



char *
IceGetListenConnectionString (listenObj)

IceListenObj listenObj;

{
    char *networkId;

    networkId = (char *) malloc (strlen (listenObj->network_id) + 1);

    if (networkId)
	strcpy (networkId, listenObj->network_id);

    return (networkId);
}



char *
IceComposeNetworkIdList (count, listenObjs)

int		count;
IceListenObj	*listenObjs;

{
    char *list;
    int len = 0;
    int i;

    if (count < 1 || listenObjs == NULL)
	return (NULL);

    for (i = 0; i < count; i++)
	len += (strlen (listenObjs[i]->network_id) + 1);

    list = (char *) malloc (len);

    if (list == NULL)
	return (NULL);
    else
    {
	int doneCount = 0;

	list[0] = '\0';

	for (i = 0; i < count; i++)
	{
	    if (_IceTransIsLocal (listenObjs[i]->trans_conn))
	    {
		strcat (list, listenObjs[i]->network_id);
		doneCount++;
		if (doneCount < count)
		    strcat (list, ",");
	    }
	}

	if (doneCount < count)
	{
	    for (i = 0; i < count; i++)
	    {
		if (!_IceTransIsLocal (listenObjs[i]->trans_conn))
		{
		    strcat (list, listenObjs[i]->network_id);
		    doneCount++;
		    if (doneCount < count)
			strcat (list, ",");
		}
	    }
	}

	return (list);
    }
}



void
IceFreeListenObjs (count, listenObjs)

int	     count;
IceListenObj *listenObjs;

{
    int i;

    for (i = 0; i < count; i++)
    {
	free (listenObjs[i]->network_id);
	_IceTransClose (listenObjs[i]->trans_conn);
	free ((char *) listenObjs[i]);
    }

    free ((char *) listenObjs);
}



/*
 * Allow host based authentication for the ICE Connection Setup.
 * Do not confuse with the host based authentication callbacks that
 * can be set up in IceRegisterForProtocolReply.
 */

void
IceSetHostBasedAuthProc (listenObj, hostBasedAuthProc)

IceListenObj		listenObj;
IceHostBasedAuthProc	hostBasedAuthProc;

{
    listenObj->host_based_auth_proc = hostBasedAuthProc;
}
