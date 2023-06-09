/* $TOG: greet.h /main/5 1998/02/09 13:55:28 kaleb $ */
/*

Copyright 1994, 1998  The Open Group

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
/* $XFree86: xc/programs/xdm/greet.h,v 1.4 1999/02/28 11:20:12 dawes Exp $ */

/*
 * greet.h - interface to xdm's dynamically-loadable modular greeter
 */

#include <X11/Xlib.h>

/*
 * Do this rather than break a build over a const-mismatch
 */
#if defined(__linux__) || defined(CSRG_BASED)
#define CRYPT_ARGS    const char *s1, const char *s2
#define GETSPNAM_ARGS const char *name
#define GETPWNAM_ARGS const char *name
#else
#define CRYPT_ARGS    /*unknown*/
#define GETSPNAM_ARGS /*unknown*/
#define GETPWNAM_ARGS /*unknown*/
#endif

#if defined(__FreeBSD__) || defined(__bsdi__) || defined(__osf__)
#define SETGRENT_TYPE int
#else
#define SETGRENT_TYPE void
#endif

struct dlfuncs {
    int (*_PingServer)(struct display *d, Display *alternateDpy);
    void (*_SessionPingFailed)(struct display *d);
    void (*_Debug)(char * fmt, ...);
    void (*_RegisterCloseOnFork)(int fd);
    void (*_SecureDisplay)(struct display *d, Display *dpy);
    void (*_UnsecureDisplay)(struct display *d, Display *dpy);
    void (*_ClearCloseOnFork)(int fd);
    void (*_SetupDisplay)(struct display *d);
    void (*_LogError)(char * fmt, ...);
    void (*_SessionExit)(struct display *d, int status, int removeAuth);
    void (*_DeleteXloginResources)(struct display *d, Display *dpy);
    int (*_source)(char **environ, char *file);
    char **(*_defaultEnv)(void);
    char **(*_setEnv)(char **e, char *name, char *value);
    char **(*_parseArgs)(char **argv, char *string);
    void (*_printEnv)(char **e);
    char **(*_systemEnv)(struct display *d, char *user, char *home);
    void (*_LogOutOfMem)(char * fmt, ...);
    SETGRENT_TYPE (*_setgrent)(void);		/* no longer used */
    struct group *(*_getgrent)(void);	/* no longer used */
    void (*_endgrent)(void);		/* no longer used */
#ifdef USESHADOW
    struct spwd *(*_getspnam)(GETSPNAM_ARGS);
    void (*_endspent)(void);
#endif
    struct passwd *(*_getpwnam)(GETPWNAM_ARGS);
    char *(*_crypt)(CRYPT_ARGS);
};

/*
 * Return values for GreetUser();
 * Values must be explictly defined because the greet library
 * may come from a different vendor.
 * Negative values indicate an error.
 */
typedef enum {
    Greet_Session_Over = 0,	/* session managed and over */
    Greet_Success = 1,		/* greet succeeded, session not managed */
    Greet_Failure = -1		/* greet failed */
} greet_user_rtn;

/*
 * GreetUser can either handle the user's session or allow xdm to do it.
 * The return or exit status of GreetUser indicates to xdm whether it
 * should start a session.
 * 
 * GreetUser is passed the xdm struct display pointer, a pointer to a
 * Display, and pointers to greet and verify structs.  If it expectes xdm
 * to run the session, it fills in the Display pointer and the fields
 * of the greet and verify structs.
 * 
 * The verify struct includes the uid, gid, arguments to run the session,
 * environment for the session, and environment for startup/reset.
 * 
 * The greet struct includes the user's name and password but these are
 * really only needed if xdm is compiled with a user-based authorization
 * option such as SECURE_RPC or K5AUTH.
 */

extern greet_user_rtn GreetUser(
#if NeedFunctionPrototypes
    struct display *d,
    Display **dpy,
    struct verify_info *verify,
    struct greet_info *greet,
    struct dlfuncs *dlfcns
#endif
);

typedef greet_user_rtn (*GreetUserProc)(
#if NeedFunctionPrototypes
    struct display *,
    Display **,
    struct verify_info *,
    struct greet_info *,
    struct dlfuncs *dlfcns
#endif
);

#ifdef GREET_LIB
/*
 * The greeter uses some symbols from the main xdm executable.  Since some
 * dynamic linkers are broken, we need to fix things up so that the symbols
 * are referenced indirectly through function pointers.  The definitions
 * here, are used to hold the pointers to the functions in the main xdm
 * executable.  The pointers are filled in when the GreetUser function is
 * called, with the pointer values passed as a paramter.
 */

extern	int     (*__xdm_PingServer)();
extern	int     (*__xdm_SessionPingFailed)();
extern	int     (*__xdm_Debug)();
extern	int     (*__xdm_RegisterCloseOnFork)();
extern	int     (*__xdm_SecureDisplay)();
extern	int     (*__xdm_UnsecureDisplay)();
extern	int     (*__xdm_ClearCloseOnFork)();
extern	int     (*__xdm_SetupDisplay)();
extern	int     (*__xdm_LogError)();
extern	int     (*__xdm_SessionExit)();
extern	int     (*__xdm_DeleteXloginResources)();
extern	int     (*__xdm_source)();
extern	char    **(*__xdm_defaultEnv)();
extern	char    **(*__xdm_setEnv)();
extern	char    **(*__xdm_parseArgs)();
extern	int     (*__xdm_printEnv)();
extern	char    **(*__xdm_systemEnv)();
extern	int     (*__xdm_LogOutOfMem)();
extern	void    (*__xdm_setgrent)();
extern	struct group    *(*__xdm_getgrent)();
extern	void    (*__xdm_endgrent)();
#ifdef USESHADOW
extern	struct spwd   *(*__xdm_getspnam)();
extern	void   (*__xdm_endspent)();
#endif
extern	struct passwd   *(*__xdm_getpwnam)();
extern	char     *(*__xdm_crypt)();

/*
 * Force the shared library to call through the function pointer
 * initialized during the initial call into the library.
 */

#define	PingServer	(*__xdm_PingServer)
#define	SessionPingFailed	(*__xdm_SessionPingFailed)
#define	Debug	(*__xdm_Debug)
#define	RegisterCloseOnFork	(*__xdm_RegisterCloseOnFork)
#define	SecureDisplay	(*__xdm_SecureDisplay)
#define	UnsecureDisplay	(*__xdm_UnsecureDisplay)
#define	ClearCloseOnFork	(*__xdm_ClearCloseOnFork)
#define	SetupDisplay	(*__xdm_SetupDisplay)
#define	LogError	(*__xdm_LogError)
#define	SessionExit	(*__xdm_SessionExit)
#define	DeleteXloginResources	(*__xdm_DeleteXloginResources)
#define	source	(*__xdm_source)
#define	defaultEnv	(*__xdm_defaultEnv)
#define	setEnv	(*__xdm_setEnv)
#define	parseArgs	(*__xdm_parseArgs)
#define	printEnv	(*__xdm_printEnv)
#define	systemEnv	(*__xdm_systemEnv)
#define	LogOutOfMem	(*__xdm_LogOutOfMem)
#define	setgrent	(*__xdm_setgrent)
#define	getgrent	(*__xdm_getgrent)
#define	endgrent	(*__xdm_endgrent)
#ifdef USESHADOW
#define	getspnam	(*__xdm_getspnam)
#define	endspent	(*__xdm_endspent)
#endif
#define	getpwnam	(*__xdm_getpwnam)
#define	crypt		(*__xdm_crypt)

#endif /* GREET_LIB */
