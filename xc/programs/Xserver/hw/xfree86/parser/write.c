/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/write.c,v 1.10 2000/03/06 22:59:34 dawes Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

/* View/edit this file with tab stops set to 4 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#if (defined(X_NOT_STDC_ENV) || (defined(sun) && !defined(SVR4)) || defined(macII)) && !defined(__GLIBC__)
#ifndef strerror
extern char *sys_errlist[];
extern int sys_nerr;
#define strerror(n) \
    (((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error")
#endif
#endif

#if defined(SVR4) || defined(__linux__) || defined(CSRG_BASED)
#define HAS_SAVED_IDS_AND_SETEUID
#endif
#if defined(__EMX__) || defined(WIN32)
#define HAS_NO_UIDS
#endif

#ifdef HAS_NO_UIDS
#define doWriteConfigFile xf86WriteConfigFile
#define Local /**/
#else
#define Local static
#endif

Local int
doWriteConfigFile (const char *filename, XF86ConfigPtr cptr)
{
	FILE *cf;

	if ((cf = fopen (filename, "w")) == NULL)
	{
		return 0;
	}

	printLayoutSection (cf, cptr->conf_layout_lst);

	fprintf (cf, "Section \"Files\"\n");
	printFileSection (cf, cptr->conf_files);
	fprintf (cf, "EndSection\n\n");

	fprintf (cf, "Section \"Module\"\n");
	printModuleSection (cf, cptr->conf_modules);
	fprintf (cf, "EndSection\n\n");

	printVendorSection (cf, cptr->conf_vendor_lst);

	printServerFlagsSection (cf, cptr->conf_flags);

	printInputSection (cf, cptr->conf_input_lst);

	printVideoAdaptorSection (cf, cptr->conf_videoadaptor_lst);

	printMonitorSection (cf, cptr->conf_monitor_lst);

	printDeviceSection (cf, cptr->conf_device_lst);

	printScreenSection (cf, cptr->conf_screen_lst);

	fprintf (cf, "Section \"DRI\"\n");
	printDRISection (cf, cptr->conf_dri);
	fprintf (cf, "EndSection\n\n");

	fclose(cf);
	return 1;
}

#ifndef HAS_NO_UIDS

int
xf86WriteConfigFile (const char *filename, XF86ConfigPtr cptr)
{
	int ret;

#if !defined(HAS_SAVED_IDS_AND_SETEUID)
	int pid, p;
	int status;
	void (*csig)(int);
#else
	int ruid, euid;
#endif

	if (getuid() != geteuid())
	{

#if !defined(HAS_SAVED_IDS_AND_SETEUID)
		/* Need to fork to change ruid without loosing euid */
#ifdef SIGCHLD
		csig = signal(SIGCHLD, SIG_DFL);
#endif
		switch ((pid = fork()))
		{
		case -1:
			ErrorF("xf86WriteConfigFile(): fork failed (%s)\n",
					strerror(errno));
			return 0;
		case 0: /* child */
			setuid(getuid());
			ret = doWriteConfigFile(filename, cptr);
			exit(ret);
			break;
		default: /* parent */
			do
			{
				p = waitpid(pid, &status, 0);
			} while (p == -1 && errno == EINTR);
		}
#ifdef SIGCHLD
		signal(SIGCHLD, csig);
#endif
		if (p != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0)
			return 1;	/* success */
		else
			return 0;

#else /* HAS_SAVED_IDS_AND_SETEUID */

		ruid = getuid();
		euid = geteuid();

		if (seteuid(ruid) == -1)
		{
			ErrorF("xf86WriteConfigFile(): seteuid(%d) failed (%s)\n",
					ruid, strerror(errno));
			return 0;
		}
		ret = doWriteConfigFile(filename, cptr);

		if (seteuid(euid) == -1)
		{
			ErrorF("xf86WriteConfigFile(): seteuid(%d) failed (%s)\n",
					euid, strerror(errno));
		}
		return ret;

#endif /* HAS_SAVED_IDS_AND_SETEUID */

	}
	else
	{
		return doWriteConfigFile(filename, cptr);
	}
}

#endif /* !HAS_NO_UIDS */
