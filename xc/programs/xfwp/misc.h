/* $TOG: misc.h /main/4 1997/11/12 08:16:37 barstow $ */
/*

Copyright "1986-1997 The Open Group All Rights Reserved

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to use the Software without restriction,
including, without limitation, the rights to copy, modify,
merge, publish, distribute and sublicense the Software, to make,
have made, license and distribute derivative works thereof, and
to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and the following permission notice
shall be included in all copies of the Software:

THE SOFTWARE IS PROVIDED "AS IS ", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
AND NON-INFRINGEMENT. IN NO EVENT SHALL THE OPEN GROUP BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER SIABILITIY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN
THE SOFTWARE.

Except as contained in this notice, the name of The Open Group
shall not be used in advertising or otherwise to promote the use
or other dealings in this Software without prior written
authorization from The Open Group.

X Window System is a trademark of The Open Group.

*/

#ifndef _MISC_H
#define _MISC_H

/*
 * the following are timeout defaults (in seconds) on the three
 * types of ports (pm-data, client-listen and client-data)
 */
#define PM_DATA_TIMEOUT_DEFAULT		3600    /* 1 hour */
#define CLIENT_LISTEN_TIMEOUT_DEFAULT	86400   /* 24 hours */
#define CLIENT_DATA_TIMEOUT_DEFAULT	604800  /* 1 week */

#define	PM_LISTEN_PORT			"4444"

/*
 * allocate ADD_LINES entries at a time when loading the configuration
 */
#define ADD_LINES			20
#define SEPARATOR1			" \t\n"
#define SEPARATOR2			'.'


extern char* 
Malloc(
    int s);

extern int 
doConfigCheck(
    struct sockaddr_in * source_sockaddr_in,
    struct sockaddr_in * dest_sockaddr_in,
    struct config * config_info,
    int context,
    int * rule_number);
 
extern void 
doCheckTimeouts(
    struct config * config_info,
    int * nfds_ready,
    fd_set * rinit,
    fd_set * winit,
    fd_set * readable,
    fd_set * writable);

extern int
doHandleConfigFile (
    struct config * config_info);

extern void 
doWriteLogEntry(
    char                * source,
    char                * destination,
    int                 event,
    int                 rule_number,
    struct config * config_info);

extern void
doCopyFromTo(
    int fd_from,
    int fd_to,
    fd_set * rinit,
    fd_set * winit);

extern int 
doCheckServerList(
    char * server_address,
    char ** listen_port_string,
    int num_servers);

extern void 
doProcessInputArgs(
    struct config * config_info, 
    int argc, 
    char * argv[]);

extern int 
doInitDataStructs(
    struct config * config_info, 
    struct ICE_setup_info * PM_conn_setup);

#endif /* _MISC_H */
