XCOMM!/bin/sh

XCOMM $XConsortium: startx.cpp,v 1.4 91/08/22 11:41:29 rws Exp $
XCOMM $XFree86: xc/programs/xinit/startx.cpp,v 3.2 1998/12/20 11:58:22 dawes Exp $
XCOMM 
XCOMM This is just a sample implementation of a slightly less primitive 
XCOMM interface than xinit.  It looks for user .xinitrc and .xserverrc
XCOMM files, then system xinitrc and xserverrc files, else lets xinit choose
XCOMM its default.  The system xinitrc should probably do things like check
XCOMM for .Xresources files and merge them in, startup up a window manager,
XCOMM and pop a clock and serveral xterms.
XCOMM
XCOMM Site administrators are STRONGLY urged to write nicer versions.
XCOMM 

bindir=BINDIR

userclientrc=$HOME/.xinitrc
userserverrc=$HOME/.xserverrc
sysclientrc=XINITDIR/xinitrc
sysserverrc=XINITDIR/xserverrc
clientargs=""
serverargs=""

if [ -f $userclientrc ]; then
    clientargs=$userclientrc
else if [ -f $sysclientrc ]; then
    clientargs=$sysclientrc
fi
fi

if [ -f $userserverrc ]; then
    serverargs=$userserverrc
else if [ -f $sysserverrc ]; then
    serverargs=$sysserverrc
fi
fi

display=:0
whoseargs="client"
while [ "x$1" != "x" ]; do
    case "$1" in
	/''*|\.*)	if [ "$whoseargs" = "client" ]; then
		    if [ "x$clientargs" = x ]; then
			clientargs="$1"
		    else
			clientargs="$clientargs $1"
		    fi
		else
		    if [ "x$serverargs" = x ]; then
			serverargs="$1"
		    else
			serverargs="$serverargs $1"
		    fi
		fi ;;
	--)	whoseargs="server" ;;
	*)	if [ "$whoseargs" = "client" ]; then
		    clientargs="$clientargs $1"
		else
    		    case "$1" in
		        :[0-9]) display="$1"
		        ;;
                        *) serverargs="$serverargs $1"
			;;
		    esac
		fi ;;
    esac
    shift
done

XCOMM set up default Xauth info for this machine
mcookie=`mcookie`
serverargs="$serverargs -auth $HOME/.Xauthority"
xauth add $display . $mcookie
xauth add `hostname -f`$display . $mcookie

xinit $clientargs -- $server $display $serverargs

XCOMM various machines need special cleaning up,
XCOMM which should be done here

#ifdef macII
Xrepair
screenrestore
#endif

#if defined(sun) && !defined(i386)
kbd_mode -a
#endif
