/* $XFree86: xc/programs/Xserver/hw/xfree86/fbdevhw/fbdevhw.c,v 1.11 2000/01/21 02:30:02 dawes Exp $ */

/* all driver need this */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* pci stuff */
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "xf86cmap.h"

#include "fbdevhw.h"
#include "fbpriv.h"
#include <sys/user.h>

#define DEBUG 0

#if DEBUG
# define TRACE_ENTER(str)	ErrorF("fbdevHW: " str " %d\n",pScrn->scrnIndex)
#else
# define TRACE_ENTER(str)
#endif

/* -------------------------------------------------------------------- */

#ifdef XFree86LOADER

static MODULESETUPPROTO(fbdevhwSetup);

static XF86ModuleVersionInfo fbdevHWVersRec =
{
	"fbdevhw",
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

XF86ModuleData fbdevhwModuleData = { &fbdevHWVersRec, fbdevhwSetup, NULL };

static pointer
fbdevhwSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	const char *osname;

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
		return (pointer)1;
	}
}
	
#else /* XFree86LOADER */

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#endif /* XFree86LOADER */

/* -------------------------------------------------------------------- */
/* our private data, and two functions to allocate/free this            */

#define FBDEVHWPTRLVAL(p) (p)->privates[fbdevHWPrivateIndex].ptr
#define FBDEVHWPTR(p) ((fbdevHWPtr)(FBDEVHWPTRLVAL(p)))

static int fbdevHWPrivateIndex = -1;

typedef struct {
	/* framebuffer device: filename (/dev/fb*), handle, more */
	char*				device;
	int				fd;
	void*				fbmem;
	int				fboff;
	void*				mmio;

	/* current hardware state */
	struct fb_fix_screeninfo	fix;
	struct fb_var_screeninfo	var;

	/* saved video mode */
	struct fb_var_screeninfo	saved_var;
	struct fb_cmap			saved_cmap;
	unsigned short			*saved_red;
	unsigned short			*saved_green;
	unsigned short			*saved_blue;

	/* buildin video mode */
	DisplayModeRec			buildin;

} fbdevHWRec, *fbdevHWPtr;

Bool
fbdevHWGetRec(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr;
	
	if (fbdevHWPrivateIndex < 0)
		fbdevHWPrivateIndex = xf86AllocateScrnInfoPrivateIndex();

	if (FBDEVHWPTR(pScrn) != NULL)
		return TRUE;
	
	fPtr = FBDEVHWPTRLVAL(pScrn) = xnfcalloc(sizeof(fbdevHWRec), 1);
	return TRUE;
}

void
fbdevHWFreeRec(ScrnInfoPtr pScrn)
{
	if (fbdevHWPrivateIndex < 0)
		return;
	if (FBDEVHWPTR(pScrn) == NULL)
		return;
	xfree(FBDEVHWPTR(pScrn));
	FBDEVHWPTRLVAL(pScrn) = NULL;
}

/* -------------------------------------------------------------------- */
/* some helpers for printing debug informations                         */

#if DEBUG
static void
print_fbdev_mode(char *txt, struct fb_var_screeninfo *var)
{
	ErrorF( "fbdev %s mode:\t%d   %d %d %d %d   %d %d %d %d   %d %d:%d:%d\n",
		txt,var->pixclock,
		var->xres, var->right_margin, var->hsync_len, var->left_margin,
		var->yres, var->lower_margin, var->vsync_len, var->upper_margin,
		var->bits_per_pixel,
		var->red.length, var->green.length, var->blue.length);
}

static void
print_xfree_mode(char *txt, DisplayModePtr mode)
{
	ErrorF( "xfree %s mode:\t%d   %d %d %d %d   %d %d %d %d\n",
		txt,mode->Clock,
		mode->HDisplay, mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
		mode->VDisplay, mode->VSyncStart, mode->VSyncEnd, mode->VTotal);
}
#endif

/* -------------------------------------------------------------------- */
/* Convert timings between the XFree and the Frame Buffer Device        */

static void
xfree2fbdev_fblayout(ScrnInfoPtr pScrn, struct fb_var_screeninfo *var)
{
	var->xres_virtual   = pScrn->virtualX;
	var->yres_virtual   = pScrn->virtualY;
	var->bits_per_pixel = pScrn->bitsPerPixel;
	var->red.length     = 0;
	var->red.offset     = 0;
	var->green.length   = 0;
	var->green.offset   = 0;
	var->blue.length    = 0;
	var->blue.offset    = 0;
}

static void
xfree2fbdev_timing(DisplayModePtr mode, struct fb_var_screeninfo *var)
{
	var->xres = mode->HDisplay;
	var->yres = mode->VDisplay;
	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;
	var->xoffset = var->yoffset = 0;
	var->pixclock = mode->Clock ? 1000000000/mode->Clock : 0;
	var->right_margin = mode->HSyncStart-mode->HDisplay;
	var->hsync_len = mode->HSyncEnd-mode->HSyncStart;
	var->left_margin = mode->HTotal-mode->HSyncEnd;
	var->lower_margin = mode->VSyncStart-mode->VDisplay;
	var->vsync_len = mode->VSyncEnd-mode->VSyncStart;
	var->upper_margin = mode->VTotal-mode->VSyncEnd;
	var->sync = 0;
	if (mode->Flags & V_PHSYNC)
		var->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (mode->Flags & V_PVSYNC)
		var->sync |= FB_SYNC_VERT_HIGH_ACT;
	if (mode->Flags & V_PCSYNC)
		var->sync |= FB_SYNC_COMP_HIGH_ACT;
#if 0
	if (mode->Flags & V_BCAST)
		var->sync |= FB_SYNC_BROADCAST;
#endif
	if (mode->Flags & V_INTERLACE)
		var->vmode = FB_VMODE_INTERLACED;
	else if (mode->Flags & V_DBLSCAN)
		var->vmode = FB_VMODE_DOUBLE;
	else
		var->vmode = FB_VMODE_NONINTERLACED;
}

static void
fbdev2xfree_timing(struct fb_var_screeninfo *var, DisplayModePtr mode)
{
	mode->Clock = var->pixclock ? 1000000000/var->pixclock : 28000000;
	mode->HDisplay = var->xres;
	mode->HSyncStart = mode->HDisplay+var->right_margin;
	mode->HSyncEnd = mode->HSyncStart+var->hsync_len;
	mode->HTotal = mode->HSyncEnd+var->left_margin;
	mode->VDisplay = var->yres;
	mode->VSyncStart = mode->VDisplay+var->lower_margin;
	mode->VSyncEnd = mode->VSyncStart+var->vsync_len;
	mode->VTotal = mode->VSyncEnd+var->upper_margin;
	mode->Flags = 0;
	mode->Flags |= var->sync & FB_SYNC_HOR_HIGH_ACT ? V_PHSYNC : V_NHSYNC;
	mode->Flags |= var->sync & FB_SYNC_VERT_HIGH_ACT ? V_PVSYNC : V_NVSYNC;
	mode->Flags |= var->sync & FB_SYNC_COMP_HIGH_ACT ? V_PCSYNC : V_NCSYNC;
#if 0
	if (var->sync & FB_SYNC_BROADCAST)
		mode->Flags |= V_BCAST;
#endif
	if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED)
		mode->Flags |= V_INTERLACE;
	else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE)
		mode->Flags |= V_DBLSCAN;
	mode->SynthClock = mode->Clock;
	mode->CrtcHDisplay = mode->HDisplay;
	mode->CrtcHSyncStart = mode->HSyncStart;
	mode->CrtcHSyncEnd = mode->HSyncEnd;
	mode->CrtcHTotal = mode->HTotal;
	mode->CrtcVDisplay = mode->VDisplay;
	mode->CrtcVSyncStart = mode->VSyncStart;
	mode->CrtcVSyncEnd = mode->VSyncEnd;
	mode->CrtcVTotal = mode->VTotal;
	mode->CrtcHAdjusted = FALSE;
	mode->CrtcVAdjusted = FALSE;
}


/* -------------------------------------------------------------------- */
/* open correct framebuffer device                                      */

static struct fb2pci_entry {
	CARD32 id;
	CARD32 vendor;
	CARD32 chip;
} fb2pci_map[] = {
	{ FB_ACCEL_MATROX_MGA2064W,     PCI_VENDOR_MATROX, PCI_CHIP_MGA2064      },
	{ FB_ACCEL_MATROX_MGA1064SG,    PCI_VENDOR_MATROX, PCI_CHIP_MGA1064      },
	{ FB_ACCEL_MATROX_MGA2164W,     PCI_VENDOR_MATROX, PCI_CHIP_MGA2164      },
	{ FB_ACCEL_MATROX_MGA2164W_AGP, PCI_VENDOR_MATROX, PCI_CHIP_MGA2164_AGP  },
	{ FB_ACCEL_MATROX_MGAG100,      PCI_VENDOR_MATROX, PCI_CHIP_MGAG100      },
	{ FB_ACCEL_MATROX_MGAG200,      PCI_VENDOR_MATROX, PCI_CHIP_MGAG200      },
	{ FB_ACCEL_ATI_RAGE128,         PCI_VENDOR_ATI,    PCI_CHIP_RAGE128RE    },
	{ FB_ACCEL_ATI_RAGE128,         PCI_VENDOR_ATI,    PCI_CHIP_RAGE128RF    },
	{ FB_ACCEL_ATI_RAGE128,         PCI_VENDOR_ATI,    PCI_CHIP_RAGE128RK    },
	{ FB_ACCEL_ATI_RAGE128,         PCI_VENDOR_ATI,    PCI_CHIP_RAGE128RL    },
	{ FB_ACCEL_ATI_RAGE128,         PCI_VENDOR_ATI,    PCI_CHIP_RAGE128PF    },
	{ FB_ACCEL_3DFX_BANSHEE,        PCI_VENDOR_3DFX,   PCI_CHIP_VOODOO3      },
};
#define FB2PCICOUNT (sizeof(fb2pci_map)/sizeof(struct fb2pci_entry))

/* try to find the framebuffer device for a given PCI device */
static int
fbdev_open_pci(pciVideoPtr pPci)
{
	struct	fb_fix_screeninfo fix;
	char	filename[16];
	int	fd,i,j;

	for (i = 0; i < 4; i++) {
		sprintf(filename,"/dev/fb%d",i);
		if (-1 == (fd = open(filename,O_RDWR,0))) {
			continue;
		}
		if (-1 == ioctl(fd,FBIOGET_FSCREENINFO,(void*)&fix)) {
			close(fd);
			continue;
		}
		/* FIXME: better ask the fbdev driver for bus/device/func,
                          but there is no way to to this yet. */
		for (j = 0; j < FB2PCICOUNT; j++) {
			if (pPci->vendor   == fb2pci_map[j].vendor &&
			    pPci->chipType == fb2pci_map[j].chip   &&
			    fix.accel      == fb2pci_map[j].id)
				break;
		}
		if (j == FB2PCICOUNT) {
			close(fd);
			continue;
		}
		return fd;
	}
	return -1;
}

static int
fbdev_open(char *dev)
{
	struct fb_con2fbmap c2m;
	char   fbdev[16];
	int    fd;

	/* try argument (from XF86Config) first */
	if (NULL != dev)
		return open(dev,O_RDWR,0);

	/* second: environment variable */
	dev = getenv("FRAMEBUFFER");
	if (NULL != dev)
		return open(dev,O_RDWR,0);

	/* last try: default device */
	if (-1 == (fd = open("/dev/fb0",O_RDWR,0)))
		return -1;

	return fd;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWProbe(pciVideoPtr pPci, char *device)
{
	int fd;

	if (pPci)
		fd = fbdev_open_pci(pPci);
	else
		fd = fbdev_open(device);

	if (-1 == fd)
		return FALSE;
	close(fd);
	return TRUE;
}

Bool
fbdevHWInit(ScrnInfoPtr pScrn, pciVideoPtr pPci, char *device)
{
	fbdevHWPtr fPtr;

	TRACE_ENTER("Init");

	fbdevHWGetRec(pScrn);
	fPtr = FBDEVHWPTR(pScrn);

	/* open device */
	if (pPci)
		fPtr->fd = fbdev_open_pci(pPci);
	else
		fPtr->fd = fbdev_open(device);
	if (-1 == fPtr->fd)
		return FALSE;

	/* get current fb device settings */
	if (-1 == ioctl(fPtr->fd,FBIOGET_FSCREENINFO,(void*)(&fPtr->fix))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "ioctl FBIOGET_FSCREENINFO: %s\n",
			   strerror(errno));
		return FALSE;
	}
	if (-1 == ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->var))) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "ioctl FBIOGET_VSCREENINFO: %s\n",
			   strerror(errno));
		return FALSE;
	}

	/* we can use the current settings as "buildin mode" */
	fbdev2xfree_timing(&fPtr->var, &fPtr->buildin);
	fPtr->buildin.name  = "current";
	fPtr->buildin.next  = &fPtr->buildin;
	fPtr->buildin.prev  = &fPtr->buildin;
	fPtr->buildin.type |= M_T_BUILTIN;
	
	return TRUE;
}

char*
fbdevHWGetName(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.id;
}

int
fbdevHWGetDepth(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->var.bits_per_pixel;
}

int
fbdevHWGetType(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.type;
}

int
fbdevHWGetVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return fPtr->fix.smem_len;
}

void
fbdevHWSetVideoModes(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	int virtX = pScrn->display->virtualX;
	int virtY = pScrn->display->virtualY;
	struct fb_var_screeninfo var;
	char **modename;
	DisplayModePtr mode,this,last = NULL;

	TRACE_ENTER("VerifyModes");
	if (NULL == pScrn->display->modes)
		return;

	for (modename = pScrn->display->modes; *modename != NULL; modename++) {
		for (mode = pScrn->monitor->Modes; mode != NULL; mode = mode->next)
			if (0 == strcmp(mode->name,*modename))
				break;
		if (NULL == mode) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "\tmode \"%s\" not found\n", *modename);
			continue;
		}
		memset(&var,0,sizeof(var));
		xfree2fbdev_timing(mode,&var);
		var.xres_virtual = virtX;
		var.yres_virtual = virtY;
		var.bits_per_pixel = pScrn->depth;
		var.activate = FB_ACTIVATE_TEST;
		if (var.xres_virtual < var.xres) var.xres_virtual = var.xres;
		if (var.yres_virtual < var.yres) var.yres_virtual = var.yres;
		if (-1 == ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&var))) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
				   "\tmode \"%s\" test failed\n", *modename);
			continue;
		}
		xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			   "\tmode \"%s\" ok\n", *modename);
		if (virtX < var.xres) virtX = var.xres;
		if (virtY < var.yres) virtY = var.yres;
		if (NULL == pScrn->modes) {
			pScrn->modes = xnfalloc(sizeof(DisplayModeRec));
			this = pScrn->modes;
			memcpy(this,mode,sizeof(DisplayModeRec));
			this->next = this;
			this->prev = this;
		} else {
			this = xnfalloc(sizeof(DisplayModeRec));
			memcpy(this,mode,sizeof(DisplayModeRec));
			this->next = pScrn->modes;
			this->prev = last;
			last->next = this;
			pScrn->modes->prev = this;
		}
		last = this;
	}
	pScrn->virtualX     = virtX;
	pScrn->virtualY     = virtY;
}

DisplayModePtr
fbdevHWGetBuildinMode(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	return &fPtr->buildin;
}

void
fbdevHWUseBuildinMode(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UseBuildinMode");
	pScrn->modes    = &fPtr->buildin;
	pScrn->virtualX = pScrn->display->virtualX;
	pScrn->virtualY = pScrn->display->virtualY;
	if (pScrn->virtualX < fPtr->buildin.HDisplay)
		pScrn->virtualX = fPtr->buildin.HDisplay;
	if (pScrn->virtualY < fPtr->buildin.VDisplay)
		pScrn->virtualY = fPtr->buildin.VDisplay;
}

/* -------------------------------------------------------------------- */

void*
fbdevHWMapVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("MapVidmem");
	if (NULL == fPtr->fbmem) {
		fPtr->fboff = fPtr->fix.smem_len & (PAGE_SIZE-1);
		fPtr->fbmem = mmap(NULL, fPtr->fix.smem_len, PROT_READ | PROT_WRITE,
				   MAP_SHARED, fPtr->fd, 0);
		if (-1 == (int)fPtr->fbmem) {
			perror("mmap fbmem");
			fPtr->fbmem = NULL;
		}
	}
	pScrn->memPhysBase = (unsigned long)fPtr->fix.smem_start & (unsigned long)(PAGE_MASK);
	pScrn->fbOffset = (unsigned long)fPtr->fix.smem_start & (unsigned long)(~PAGE_MASK);
	return fPtr->fbmem;
}

int
fbdevHWLinearOffset(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("LinearOffset");
	return fPtr->fboff;
}

Bool
fbdevHWUnmapVidmem(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UnmapVidmem");
	if (NULL != fPtr->fbmem) {
		if (-1 == munmap(fPtr->fbmem, fPtr->fix.smem_len))
			perror("munmap fbmem");
		fPtr->fbmem = NULL;
	}
	return TRUE;
}

void*
fbdevHWMapMMIO(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("MapMMIO");
	if (NULL == fPtr->mmio) {
		/* tell the kernel not to use accels to speed up console scrolling */
		fPtr->var.accel_flags = 0;
		if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
			perror("FBIOPUT_VSCREENINFO");
			return FALSE;
		}
		fPtr->mmio = mmap(NULL, fPtr->fix.mmio_len, PROT_READ | PROT_WRITE,
				  MAP_SHARED, fPtr->fd, fPtr->fix.smem_len);
		if (-1 == (int)fPtr->mmio) {
			perror("mmap mmio");
			fPtr->mmio = NULL;
		}
	}
	return fPtr->mmio;
}

Bool
fbdevHWUnmapMMIO(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("UnmapMMIO");
	if (NULL != fPtr->mmio) {
		if (-1 == munmap(fPtr->mmio, fPtr->fix.mmio_len))
			perror("munmap mmio");
		fPtr->mmio = NULL;
	}
	return TRUE;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{	
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	
	TRACE_ENTER("ModeInit");
	xfree2fbdev_fblayout(pScrn, &fPtr->var);
	xfree2fbdev_timing(mode,  &fPtr->var);
#if DEBUG
	print_xfree_mode("init",mode);
	print_fbdev_mode("init",&fPtr->var);
#endif
	pScrn->vtSema = TRUE;

	/* set */
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		perror("FBIOPUT_VSCREENINFO");
		return FALSE;
	}
	/* read back */
	if (0 != ioctl(fPtr->fd,FBIOGET_FSCREENINFO,(void*)(&fPtr->fix))) {
		perror("FBIOGET_FSCREENINFO");
		return FALSE;
	}
	if (0 != ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->var))) {
		perror("FBIOGET_VSCREENINFO");
		return FALSE;
	}
	return TRUE;
}

/* -------------------------------------------------------------------- */
/* video mode save/restore                                              */

/* TODO: colormap */
void
fbdevHWSave(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("Save");
	if (0 != ioctl(fPtr->fd,FBIOGET_VSCREENINFO,(void*)(&fPtr->saved_var)))
		perror("FBIOGET_VSCREENINFO");
}

void
fbdevHWRestore(ScrnInfoPtr pScrn)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("Restore");
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->saved_var)))
		perror("FBIOPUT_VSCREENINFO");
}

/* -------------------------------------------------------------------- */
/* callback for xf86HandleColormaps                                     */

void
fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO *colors, VisualPtr pVisual)
{
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	struct fb_cmap cmap;
	unsigned short red,green,blue;
	int i;

	TRACE_ENTER("ModeInit");
	cmap.len   = 1;
	cmap.red   = &red;
	cmap.green = &green;
	cmap.blue  = &blue;
	cmap.transp = NULL;
	for (i = 0; i < numColors; i++) {
		cmap.start = indices[i];
		red   = colors[indices[i]].red   << 8;
		green = colors[indices[i]].green << 8;
		blue  = colors[indices[i]].blue  << 8;
		if (-1 == ioctl(fPtr->fd,FBIOPUTCMAP,(void*)&cmap))
			perror("ioctl FBIOPUTCMAP");
	}
}

/* -------------------------------------------------------------------- */
/* these can be hooked directly into ScrnInfoRec                        */

int
fbdevHWValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
	struct fb_var_screeninfo var;

	TRACE_ENTER("ValidMode");
	memcpy(&var,&fPtr->var,sizeof(var));
	xfree2fbdev_timing(mode, &var);
	var.activate = FB_ACTIVATE_TEST;
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		perror("FBIOPUT_VSCREENINFO");
		return MODE_BAD;
	}
	return MODE_OK;
}

Bool
fbdevHWSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("SwitchMode");
	xfree2fbdev_timing(mode, &fPtr->var);
	if (0 != ioctl(fPtr->fd,FBIOPUT_VSCREENINFO,(void*)(&fPtr->var))) {
		perror("FBIOPUT_VSCREENINFO");
		return FALSE;
	}
	return TRUE;
}

void
fbdevHWAdjustFrame(int scrnIndex, int x, int y, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

	TRACE_ENTER("AdjustFrame");
	fPtr->var.xoffset = x;
	fPtr->var.yoffset = y;
	if (-1 == ioctl(fPtr->fd,FBIOPAN_DISPLAY,(void*)&fPtr->var))
		perror("ioctl FBIOPAN_DISPLAY");
}

Bool
fbdevHWEnterVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	
	TRACE_ENTER("EnterVT");
	if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
		return FALSE;
	fbdevHWAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
	return TRUE;
}

void
fbdevHWLeaveVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

	TRACE_ENTER("LeaveVT");
	fbdevHWRestore(pScrn);
}

