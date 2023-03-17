/*
 *  video4linux Xv Driver 
 *  based on Michael Schimek's permedia 2 driver.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/v4l/v4l.c,v 1.16 2000/02/22 02:00:54 mvojkovi Exp $ */

#include "videodev.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86fbman.h"
#include "xf86xv.h"
#include "Xv.h"
#include "miscstruct.h"
#include "dgaproc.h"
#include "xf86str.h"


#include <asm/ioctl.h>		/* _IORW(xxx) #defines are here */
#if 0
typedef unsigned long ulong;
#endif

/* XXX Lots of xalloc() calls don't check for failure. */

#define DEBUG(x) (x)

static void     V4LIdentify(int flags);
static Bool     V4LProbe(DriverPtr drv, int flags);
static OptionInfoPtr V4LAvailableOptions(int chipid, int busid);

DriverRec V4L = {
        40000,
        "Xv driver for video4linux",
        V4LIdentify, /* Identify*/
        V4LProbe, /* Probe */
	V4LAvailableOptions,
        NULL,
        0
};    


#ifdef XFree86LOADER

static MODULESETUPPROTO(v4lSetup);

static XF86ModuleVersionInfo v4lVersRec =
{
        "v4l",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0, 0, 1,
        ABI_CLASS_VIDEODRV,
        ABI_VIDEODRV_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}
};

XF86ModuleData v4lModuleData = { &v4lVersRec, v4lSetup, NULL };

static pointer
v4lSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
        const char *osname;
	static Bool setupDone = FALSE;

	if (setupDone) {
	    if (errmaj)
		*errmaj = LDR_ONCEONLY;
	    return NULL;             
	}
	
	setupDone = TRUE;

        /* Check that we're being loaded on a Linux system */
        LoaderGetOS(&osname, NULL, NULL, NULL);
        if (!osname || strcmp(osname, "linux") != 0) {
                if (errmaj)
                        *errmaj = LDR_BADOS;
                if (errmin)
                        *errmin = 0;
                return NULL;
        } else {
                /* OK */

	        xf86AddDriver (&V4L, module, 0);   
	  
                return (pointer)1;
        }
}

#else

#include <fcntl.h>
#include <sys/ioctl.h>

#endif

typedef struct _PortPrivRec {
    ScrnInfoPtr                 pScrn;
    FBAreaPtr			pFBArea[2];
    int				VideoOn;	/* yes/no */
    Bool			StreamOn;

    /* file handle */
    int 			fd;
    char                        devname[16];
    int                         useCount;

    /* overlay */
    struct video_buffer		ov_fbuf;
    struct video_window		ov_win;

    /* attributes */
    struct video_picture	pict;
    struct video_audio          audio;

    XF86VideoEncodingPtr        enc;
    int                         nenc,cenc;
} PortPrivRec, *PortPrivPtr;

#define XV_ENCODING	"XV_ENCODING"
#define XV_BRIGHTNESS  	"XV_BRIGHTNESS"
#define XV_CONTRAST 	"XV_CONTRAST"
#define XV_SATURATION  	"XV_SATURATION"
#define XV_HUE		"XV_HUE"

#define XV_FREQ		"XV_FREQ"
#define XV_MUTE		"XV_MUTE"
#define XV_VOLUME      	"XV_VOLUME"

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvEncoding, xvBrightness, xvContrast, xvSaturation, xvHue;
static Atom xvFreq, xvMute, xvVolume;

static XF86VideoFormatRec
InputVideoFormats[] = {
    { 15, TrueColor },
    { 16, TrueColor },
    { 24, TrueColor },
};

/* ---------------------------------------------------------------------- */
/* forward decl */

static void V4lQueryBestSize(ScrnInfoPtr pScrn, Bool motion,
		 short vid_w, short vid_h, short drw_w, short drw_h,
		 unsigned int *p_w, unsigned int *p_h, pointer data);

/* ---------------------------------------------------------------------- */

static int V4lOpenDevice(PortPrivPtr pPPriv, ScrnInfoPtr pScrn)
{

#if 0
    /* I don't know if this is needed or not. Alan Cox says no. EE */
    if (!xf86NoSharedMem(pScrn->scrnIndex)) {
	xf86Msg(X_ERROR,"Screen %i cannot grant access to fb\n",
		pScrn->scrnIndex);
	return 1;
    }
#endif
    pPPriv->useCount++;
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
			"Xv/open: refcount=%d\n",pPPriv->useCount));

    if (pPPriv->fd == -1) {
	pPPriv->fd = open(pPPriv->devname, O_RDWR, 0);

	pPPriv->ov_fbuf.width        = pScrn->virtualX;
	pPPriv->ov_fbuf.height       = pScrn->virtualY;
	pPPriv->ov_fbuf.depth        = pScrn->bitsPerPixel;
	pPPriv->ov_fbuf.bytesperline = pScrn->displayWidth * ((pScrn->bitsPerPixel + 7)/8);
	pPPriv->ov_fbuf.base         = (pointer)(pScrn->memPhysBase + pScrn->fbOffset);
	
	if (-1 == ioctl(pPPriv->fd,VIDIOCSFBUF,&(pPPriv->ov_fbuf)))
	    perror("ioctl VIDIOCSFBUF");
    }

    if (pPPriv->fd == -1)
	return errno;
   
    return 0;
}

static void V4lCloseDevice(PortPrivPtr pPPriv)
{
    pPPriv->useCount--;
    
    DEBUG(xf86DrvMsgVerb(0, X_INFO, 2,
			"Xv/close: refcount=%d\n",pPPriv->useCount));
    if(pPPriv->useCount == 0 && pPPriv->fd != -1) {
	close(pPPriv->fd);
	pPPriv->fd = -1;
    }
}


static int
V4lPutVideo(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    struct video_clip *clip;
    BoxPtr pBox;
    unsigned int i,dx,dy,dw,dh;
    int one=1;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/PV\n"));
    /* FIXME: vid-* is ignored for now, not supported by v4l */

    V4lQueryBestSize(pScrn, 0, vid_w, vid_h, drw_w, drw_h, &dw, &dh, data);
    /* if the window is too big, center the video */
    dx = drw_x + (drw_w - dw)/2;
    dy = drw_y + (drw_h - dh)/2;
    /* bttv prefeares aligned addresses */
    dx &= ~3;
    if (dx < drw_x) dx += 4;
    if (dx+dw > drw_x+drw_w) dw -= 4;

    /* window */
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "  win: %dx%d+%d+%d\n",
		drw_w,drw_h,drw_x,drw_y));
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "  use: %dx%d+%d+%d\n",
		dw,dh,dx,dy));
    pPPriv->ov_win.x      = dx;
    pPPriv->ov_win.y      = dy;
    pPPriv->ov_win.width  = dw;
    pPPriv->ov_win.height = dh;
    pPPriv->ov_win.flags  = 0;
 
    /* clipping */
    if (pPPriv->ov_win.clips) {
	xfree(pPPriv->ov_win.clips);
	pPPriv->ov_win.clips = NULL;
    }
    pPPriv->ov_win.clipcount = REGION_NUM_RECTS(clipBoxes);
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,"  clip: have #%d\n",
		pPPriv->ov_win.clipcount));
    if (0 != pPPriv->ov_win.clipcount) {
	pPPriv->ov_win.clips = xalloc(pPPriv->ov_win.clipcount*sizeof(struct video_clip));
	memset(pPPriv->ov_win.clips,0,pPPriv->ov_win.clipcount*sizeof(struct video_clip));
	pBox = REGION_RECTS(clipBoxes);
	clip = pPPriv->ov_win.clips;
	for (i = 0; i < REGION_NUM_RECTS(clipBoxes); i++, pBox++, clip++) {
	    clip->x	 = pBox->x1 - dx;
	    clip->y      = pBox->y1 - dy;
	    clip->width  = pBox->x2 - pBox->x1;
	    clip->height = pBox->y2 - pBox->y1;
	}
    }

    /* Open a file handle to the device */

    if (!pPPriv->VideoOn) {
	if (V4lOpenDevice(pPPriv, pScrn))
	    return BadAccess;
	pPPriv->VideoOn = 1;
    }

    /* start */

    if (-1 == ioctl(pPPriv->fd,VIDIOCSWIN,&(pPPriv->ov_win)))
	perror("ioctl VIDIOCSWIN");
    if (-1 == ioctl(pPPriv->fd, VIDIOCCAPTURE, &one))
	perror("ioctl VIDIOCCAPTURE(1)");

    return Success;
}

static int
V4lPutStill(ScrnInfoPtr pScrn,
    short vid_x, short vid_y, short drw_x, short drw_y,
    short vid_w, short vid_h, short drw_w, short drw_h,
    RegionPtr clipBoxes, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/PS\n"));

    /* FIXME */
    return Success;
}

static void
V4lStopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;  
    int zero=0;

    if (!pPPriv->VideoOn) {
	DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2,
	      "Xv/StopVideo called with video already off\n"));
	return;
    }
    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/StopVideo\n"));

    if (-1 == ioctl(pPPriv->fd, VIDIOCCAPTURE, &zero))
	perror("ioctl VIDIOCCAPTURE(0)");

    V4lCloseDevice(pPPriv);
    pPPriv->VideoOn = 0;
}

/* v4l uses range 0 - 65535; Xv uses -1000 - 1000 */
static int
v4l_to_xv(int val) {
    val = val * 2000 / 65536 - 1000;
    if (val < -1000) val = -1000;
    if (val >  1000) val =  1000;
    return val;
}
static int
xv_to_v4l(int val) {
    val = val * 65536 / 2000 + 32768;
    if (val <    -0) val =     0;
    if (val > 65535) val = 65535;
    return val;
}

static int
V4lSetPortAttribute(ScrnInfoPtr pScrn,
    Atom attribute, INT32 value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data; 
    struct video_channel chan;
    int ret = Success;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/SPA %d, %d\n",
	attribute, value));

    if (V4lOpenDevice(pPPriv, pScrn))
	return BadAccess;

    if (-1 == pPPriv->fd) {
	ret = Success /* FIXME: EBUSY/ENODEV ?? */;
    } else if (attribute == xvEncoding) {
	if (value >= 0 && value < pPPriv->nenc) {
	    pPPriv->cenc = value;
	    chan.channel = value/3;
	    chan.norm    = value%3;
	    if (-1 == ioctl(pPPriv->fd,VIDIOCSCHAN,&chan))
		perror("ioctl VIDIOCSCHAN");
	} else {
	    ret = BadValue;
	}
    } else if (attribute == xvBrightness ||
               attribute == xvContrast   ||
               attribute == xvSaturation ||
               attribute == xvHue) {
	ioctl(pPPriv->fd,VIDIOCGPICT,&pPPriv->pict);
	if (attribute == xvBrightness) pPPriv->pict.brightness = xv_to_v4l(value);
	if (attribute == xvContrast)   pPPriv->pict.contrast   = xv_to_v4l(value);
	if (attribute == xvSaturation) pPPriv->pict.colour     = xv_to_v4l(value);
	if (attribute == xvHue)        pPPriv->pict.hue        = xv_to_v4l(value);
	if (-1 == ioctl(pPPriv->fd,VIDIOCSPICT,&pPPriv->pict))
	    perror("ioctl VIDIOCSPICT");
    } else if (attribute == xvMute ||
	       attribute == xvVolume) {
	ioctl(pPPriv->fd,VIDIOCGAUDIO,&pPPriv->audio);
	if (attribute == xvMute) {
	    if (value)
		pPPriv->audio.flags |= VIDEO_AUDIO_MUTE;
	    else
		pPPriv->audio.flags &= ~VIDEO_AUDIO_MUTE;
	} else if (attribute == xvVolume && (pPPriv->audio.flags & VIDEO_AUDIO_VOLUME)) {
	    pPPriv->audio.volume = xv_to_v4l(value);
	} else {
	    ret = BadValue;
	}
	if (ret != BadValue)
	    if (-1 == ioctl(pPPriv->fd,VIDIOCSAUDIO,&pPPriv->audio))
		perror("ioctl VIDIOCSAUDIO");
    } else if (attribute == xvFreq) {
	if (-1 == ioctl(pPPriv->fd,VIDIOCSFREQ,&value))
	    perror("ioctl VIDIOCSFREQ");
    } else {
	ret = BadValue;
    }

    V4lCloseDevice(pPPriv);
    return ret;
}

static int
V4lGetPortAttribute(ScrnInfoPtr pScrn, 
    Atom attribute, INT32 *value, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    int ret = Success;

    if (V4lOpenDevice(pPPriv, pScrn))
	return BadAccess;

    if (-1 == pPPriv->fd) {
	ret = Success /* FIXME: EBUSY/ENODEV ?? */;
    } else if (attribute == xvEncoding) {
	*value = pPPriv->cenc;
    } else if (attribute == xvBrightness ||
               attribute == xvContrast   ||
               attribute == xvSaturation ||
               attribute == xvHue) {
	ioctl(pPPriv->fd,VIDIOCGPICT,&pPPriv->pict);
	if (attribute == xvBrightness) *value = v4l_to_xv(pPPriv->pict.brightness);
	if (attribute == xvContrast)   *value = v4l_to_xv(pPPriv->pict.contrast);
	if (attribute == xvSaturation) *value = v4l_to_xv(pPPriv->pict.colour);
	if (attribute == xvHue)        *value = v4l_to_xv(pPPriv->pict.hue);
    } else if (attribute == xvMute ||
	       attribute == xvVolume) {
	ioctl(pPPriv->fd,VIDIOCGAUDIO,&pPPriv->audio);
	if (attribute == xvMute) {
	    *value = (pPPriv->audio.flags & VIDEO_AUDIO_MUTE) ? 1 : 0;
	} else if (attribute == xvVolume && (pPPriv->audio.flags & VIDEO_AUDIO_VOLUME)) {
	    *value = v4l_to_xv(pPPriv->audio.volume);
	} else {
	    ret = BadValue;
	}
    } else if (attribute == xvFreq) {
	ioctl(pPPriv->fd,VIDIOCGFREQ,value);
    } else {
	ret = BadValue;
    }

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/GPA %d, %d\n",
	attribute, *value));

    V4lCloseDevice(pPPriv);
    return ret;
}

static void
V4lQueryBestSize(ScrnInfoPtr pScrn, Bool motion,
    short vid_w, short vid_h, short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h, pointer data)
{
    PortPrivPtr pPPriv = (PortPrivPtr) data;
    int maxx = pPPriv->enc[pPPriv->cenc].width;
    int maxy = pPPriv->enc[pPPriv->cenc].height;

    *p_w = (drw_w < maxx) ? drw_w : maxx;
    *p_h = (drw_h < maxy) ? drw_h : maxy;

    DEBUG(xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 2, "Xv/BS %d %dx%d %dx%d\n",
			 pPPriv->cenc,drw_w,drw_h,*p_w,*p_h));
}

static
OptionInfoPtr
V4LAvailableOptions(int chipid, int busid)
{
    return NULL;
}

static void
V4LIdentify(int flags)
{
    xf86Msg(X_INFO, "v4l driver for Video4Linux\n");
}        

static char*
fixname(char *str)
{
	int s,d;
	for (s=0, d=0;; s++) {
		if (str[s] == '-')
			continue;
		str[d++] = tolower(str[s]);
		if (0 == str[s])
			break;
	}
	return str;
}

static XF86VideoEncodingPtr
V4LBuildEncodings(int fd, int *count)
{
    static struct video_capability  cap;
    static struct video_channel     channel;
    XF86VideoEncodingPtr            enc;
    int i;

    if (-1 == ioctl(fd,VIDIOCGCAP,&cap))
	return NULL;

    enc = xalloc(sizeof(XF86VideoEncodingRec)*3*cap.channels);
    memset(enc,0,sizeof(XF86VideoEncodingRec)*3*cap.channels);

    for (i = 0; i < 3*cap.channels; ) {
	channel.channel = i/3;
	if (-1 == ioctl(fd,VIDIOCGCHAN,&channel)) {
	    perror("ioctl VIDIOCGCHAN");
	    return NULL;
	}

	/* one for PAL ... */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 768;
	enc[i].height = 576;
	enc[i].rate.numerator   =  1;
	enc[i].rate.denominator = 50;
	sprintf(enc[i].name,"pal-%s",fixname(channel.name));
	i++;

	/* NTSC */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 640;
	enc[i].height = 480;
	enc[i].rate.numerator   =  1001;
	enc[i].rate.denominator = 60000;
	sprintf(enc[i].name,"ntsc-%s",fixname(channel.name));
	i++;

	/* SECAM */
	enc[i].id     = i;
	enc[i].name   = malloc(strlen(channel.name)+8);
	enc[i].width  = 768;
	enc[i].height = 576;
	enc[i].rate.numerator   =  1;
	enc[i].rate.denominator = 50;
	sprintf(enc[i].name,"secam-%s",fixname(channel.name));
	i++;
    }
    *count = i;
    return enc;
}


static XF86AttributeRec Attributes[8] = {
   {XvSettable | XvGettable, -1000,    1000, XV_ENCODING},
   {XvSettable | XvGettable, -1000,    1000, XV_BRIGHTNESS},
   {XvSettable | XvGettable, -1000,    1000, XV_CONTRAST},
   {XvSettable | XvGettable, -1000,    1000, XV_SATURATION},
   {XvSettable | XvGettable, -1000,    1000, XV_HUE},
   {XvSettable | XvGettable, -1000,    1000, XV_VOLUME},
   {XvSettable | XvGettable,     0,       1, XV_MUTE},
   {XvSettable | XvGettable,     0, 16*1000, XV_FREQ},
};


static int
V4LInit(ScrnInfoPtr pScrn, XF86VideoAdaptorPtr **adaptors)
{
    PortPrivPtr pPPriv;
    DevUnion *Private;
    XF86VideoAdaptorPtr *VAR = NULL;
    XF86VideoEncodingPtr enc;
    char dev[16];
    int  fd,i,nenc;

    DEBUG(xf86Msg(X_INFO, "v4l: init start\n"));

    for (i = 0; i < 4; i++) {
	sprintf(dev,"/dev/video%d",i);
	fd = open(dev, O_RDWR, 0);
	if (fd == -1)
	    break;
	if (NULL == (enc = V4LBuildEncodings(fd,&nenc)))
	    break;
	
	DEBUG(xf86Msg(X_INFO,  "v4l: %s ok\n",dev));

	/* our private data */
	pPPriv = xalloc(sizeof(PortPrivRec));
	if (!pPPriv)
	    return FALSE;
	memset(pPPriv,0,sizeof(PortPrivRec));
	pPPriv->fd    = -1;
	strncpy(pPPriv->devname, dev, 16);
	pPPriv->useCount=0;
	pPPriv->enc = enc;
	pPPriv->nenc = nenc;

	/* alloc VideoAdaptorRec */
	VAR = xrealloc(VAR,sizeof(XF86VideoAdaptorPtr)*(i+1));
	VAR[i] = xalloc(sizeof(XF86VideoAdaptorRec));
	if (!VAR[i])
	    return FALSE;
	memset(VAR[i],0,sizeof(XF86VideoAdaptorRec));

	/* add attribute lists */
	VAR[i]->nAttributes = 8;
	VAR[i]->pAttributes = Attributes;

	/* hook in private data */
	Private = xalloc(sizeof(DevUnion));
	if (!Private)
	    return FALSE;
	memset(Private,0,sizeof(DevUnion));
	Private->ptr = (pointer)pPPriv;
	VAR[i]->pPortPrivates = Private;
	VAR[i]->nPorts = 1;

	/* init VideoAdaptorRec */
	VAR[i]->type  = XvInputMask | XvWindowMask | XvVideoMask;
	VAR[i]->name  = "video4linux";
	VAR[i]->flags = VIDEO_INVERT_CLIPLIST;

	VAR[i]->PutVideo = V4lPutVideo;
	VAR[i]->PutStill = V4lPutStill;
	VAR[i]->StopVideo = V4lStopVideo;
	VAR[i]->SetPortAttribute = V4lSetPortAttribute;
	VAR[i]->GetPortAttribute = V4lGetPortAttribute;
	VAR[i]->QueryBestSize = V4lQueryBestSize;

	VAR[i]->nEncodings = nenc;
	VAR[i]->pEncodings = enc;
	VAR[i]->nFormats =
		sizeof(InputVideoFormats) / sizeof(InputVideoFormats[0]);
	VAR[i]->pFormats = InputVideoFormats;
	if (fd != -1)
	    close(fd);
    }

    xvEncoding   = MAKE_ATOM(XV_ENCODING);
    xvHue        = MAKE_ATOM(XV_HUE);
    xvSaturation = MAKE_ATOM(XV_SATURATION);
    xvBrightness = MAKE_ATOM(XV_BRIGHTNESS);
    xvContrast   = MAKE_ATOM(XV_CONTRAST);

    xvFreq       = MAKE_ATOM(XV_FREQ);
    xvMute       = MAKE_ATOM(XV_MUTE);
    xvVolume     = MAKE_ATOM(XV_VOLUME);

    DEBUG(xf86Msg(X_INFO, "v4l: init done, %d found\n",i));

    *adaptors = VAR;
    return i;
}

static Bool
V4LProbe(DriverPtr drv, int flags)
{
    if (flags & PROBE_DETECT)
	return FALSE;

    xf86XVRegisterGenericAdaptorDriver(V4LInit);
    return TRUE;
}
