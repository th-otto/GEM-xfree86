/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_apm.c,v 3.5 2000/03/01 00:25:26 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "lnx.h"
#include <linux/apm_bios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#define APM_PROC   "/proc/apm"
#define APM_DEVICE "/dev/apm_bios"

#ifndef APM_STANDBY_FAILED
# define APM_STANDBY_FAILED 0xf000
#endif
#ifndef APM_SUSPEND_FAILED
# define APM_SUSPEND_FAILED 0xf001
#endif

static void lnxCloseAPM(void);
static pointer APMihPtr = NULL;

static struct {
    apm_event_t apmLinux;
    pmEvent xf86;
} LinuxToXF86[] = {
    { APM_SYS_STANDBY, XF86_APM_SYS_STANDBY },
    { APM_SYS_SUSPEND, XF86_APM_SYS_SUSPEND },
    { APM_NORMAL_RESUME, XF86_APM_NORMAL_RESUME },
    { APM_CRITICAL_RESUME, XF86_APM_CRITICAL_RESUME },
    { APM_LOW_BATTERY, XF86_APM_LOW_BATTERY },
    { APM_POWER_STATUS_CHANGE, XF86_APM_POWER_STATUS_CHANGE },
    { APM_UPDATE_TIME, XF86_APM_UPDATE_TIME },
    { APM_CRITICAL_SUSPEND, XF86_APM_CRITICAL_SUSPEND },
    { APM_USER_STANDBY, XF86_APM_USER_STANDBY },
    { APM_USER_SUSPEND, XF86_APM_USER_SUSPEND },
    { APM_STANDBY_RESUME, XF86_APM_STANDBY_RESUME },
#if defined(APM_CAPABILITY_CHANGED)
    { APM_CAPABILITY_CHANGED, XF86_CAPABILITY_CHANGED },
#endif
#if 0
    { APM_STANDBY_FAILED, XF86_APM_STANDBY_FAILED },
    { APM_SUSPEND_FAILED, XF86_APM_SUSPEND_FAILED }
#endif
};

#define numApmEvents (sizeof(LinuxToXF86) / sizeof(LinuxToXF86[0]))

/*
 * APM is still under construction.
 * I'm not sure if the places where I initialize/deinitialize
 * apm is correct. Also I don't know what to do in SETUP state.
 * This depends if wakeup gets called in this situation, too.
 * Also we need to check if the action that is taken on an
 * event is reasonable.
 */
static int
lnxPMGetEventFromOs(int fd, pmEvent *events, int num)
{
    int i,j,n;
    apm_event_t linuxEvents[8];
#if 0
    if (wait) {
	FD_ZERO( &fds );
	FD_SET( fd, &fds );
	ret = select( fd + 1, &fds, NULL, NULL, NULL );
	if (ret <= 0)
	    return 0;
    }
#endif
    if ((n = read( fd, linuxEvents, num * sizeof(apm_event_t) )) == -1)
	return 0;
    n /= sizeof(apm_event_t);
    for (i = 0; i < n; i++) {
	for (j = 0; j < numApmEvents; j++)
	    if (LinuxToXF86[j].apmLinux == linuxEvents[i]) {
		events[i] = LinuxToXF86[j].xf86;
		break;
	    }
	if (j == numApmEvents)
	    events[i] = XF86_APM_UNKNOWN;
    }
    return n;
}

static pmWait
lnxPMConfirmEventToOs(int fd, pmEvent event)
{
    switch (event) {
    case XF86_APM_SYS_STANDBY:
    case XF86_APM_USER_STANDBY:
	if (ioctl( fd, APM_IOC_STANDBY, NULL ) == 0)
	    return PM_WAIT;  /* should we stop the Xserver in standby, too? */
	else
	    return PM_NONE;
    case XF86_APM_SYS_SUSPEND:
    case XF86_APM_CRITICAL_SUSPEND:
    case XF86_APM_USER_SUSPEND:
	if (ioctl( fd, APM_IOC_SUSPEND, NULL ) == 0)
	    return PM_WAIT;
	else
	    return PM_NONE;
    case XF86_APM_STANDBY_RESUME:
    case XF86_APM_NORMAL_RESUME:
    case XF86_APM_CRITICAL_RESUME:
    case XF86_APM_STANDBY_FAILED:
    case XF86_APM_SUSPEND_FAILED:
	return PM_CONTINUE;
    default:
	return PM_NONE;
    }
}

PMClose
xf86OSPMOpen(void)
{
   int fd;    

   if (APMihPtr || !xf86Info.pmFlag)
       return NULL;
   
   if (access( APM_PROC, R_OK ) || ((fd = open( APM_PROC, O_RDONLY)) == -1))
       return NULL;
   close( fd );

   if ((fd = open( APM_DEVICE, O_RDWR )) > -1) {
       xf86PMGetEventFromOs = lnxPMGetEventFromOs;
       xf86PMConfirmEventToOs = lnxPMConfirmEventToOs;
       APMihPtr = xf86AddInputHandler(fd,xf86HandlePMEvents,NULL);
       return lnxCloseAPM;
   }
   return NULL;
}

static void
lnxCloseAPM(void)
{
    int fd;
    
    if (APMihPtr) {
	fd = xf86RemoveInputHandler(APMihPtr);
	close(fd);
	APMihPtr = NULL;
    }
}

