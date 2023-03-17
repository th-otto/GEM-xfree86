/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/print_edid.c,v 1.5 1999/09/25 14:37:16 dawes Exp $ */

/* print_edid.c: print out all information retrieved from display device 
 * 
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86DDC.h"

static void print_vendor(struct vendor *);
static void print_version(struct edid_version *);
static void print_display(struct disp_features *);
static void print_established_timings(struct established_timings *);
static void print_std_timings(struct std_timings *);
static void print_detailed_monitor_section(struct detailed_monitor_section *);
static void print_detailed_timings(struct detailed_timings *);

static void print_input_features(struct disp_features *);
static void print_dpms_features(struct disp_features *);
static void print_whitepoint(struct disp_features *);

xf86MonPtr
xf86PrintEDID(xf86MonPtr m)
{
    if (!(m)) return NULL;
    print_vendor(&m->vendor);
    print_version(&m->ver);
    print_display(&m->features);
    print_established_timings(&m->timings1);
    print_std_timings(m->timings2);
    print_detailed_monitor_section(m->det_mon);
    return m;
}

static void
print_vendor(struct vendor *c)
{
    ErrorF("Manufacturer: %s  ",&c->name);
    ErrorF("Model: %x ",c->prod_id);
    ErrorF("Serial#: %u  ",c->serial);
    ErrorF("Year: %u ",c->year);
    ErrorF("Week: %u\n",c->week);
}

static void
print_version(struct edid_version *c)
{
    ErrorF("EDID Version: %u.%u\n",c->version,c->revision);  
}

static void
print_display(struct disp_features *disp)
{
    print_input_features(disp);
    ErrorF("Max H-Image Size [cm]: ");
    if (disp->hsize)
	ErrorF("horiz.: %i  ",disp->hsize);
    else
	ErrorF("H-Size may change,  ");
    if (disp->vsize)
	ErrorF("vert.: %i\n",disp->vsize);
    else
	ErrorF("V-Size may change\n");
    ErrorF("Gamma: %.2f\n", disp->gamma);
    print_dpms_features(disp);
    print_whitepoint(disp);
}

static void 
print_input_features(struct disp_features *c)
{
    if (DIGITAL(c->input_type))
	ErrorF("Digital Display Input, ");
    else {
	ErrorF("Analog Display Input,  ");
	ErrorF("Input Voltage Level: ");
	switch (c->input_voltage){
	case V070:
	    ErrorF("0.700/0.300 V\n");
	    break;
	case V071:
	    ErrorF("0.714/0.286 V\n");
	    break;
	case V100:
	    ErrorF("1.000/0.400 V\n");
	    break;
	case V007:
            ErrorF("0.700/0.700 V\n");
	    break;
	default:
	    ErrorF("undefined\n");
	}
	if (SIG_SETUP(c->input_setup)) ErrorF("Signal levels configurable\n");
	ErrorF("Sync:   ");
	if (SEP_SYNC(c->input_sync)) ErrorF("Separate   ");
	if (COMP_SYNC(c->input_sync)) ErrorF("Composite   ");
	if (SYNC_O_GREEN(c->input_sync)) ErrorF("SyncOnGreen   ");
	if (SYNC_SERR(c->input_sync)) 
	    ErrorF("\nSerration on V.Sync Pulse req. if CompSync or SyncOnGreen\n");
	else ErrorF("\n");
    }
}

static void 
print_dpms_features(struct disp_features *c)
{
    ErrorF("DPMS capabilities:  ");
    if (DPMS_STANDBY(c->dpms)) ErrorF("StandBy ");
    if (DPMS_SUSPEND(c->dpms)) ErrorF("Suspend ");
    if (DPMS_OFF(c->dpms)) ErrorF("Off ");
    switch (c->display_type){
    case DISP_MONO:
	ErrorF(";   Monochorome/GrayScale Display\n");
	break;
    case DISP_RGB:
	ErrorF(";   RGB/Color Display\n");
	break;
    case DISP_MULTCOLOR:
	ErrorF(";   Non RGB Multicolor Display\n");
	break;
    default:
	break;
    }
    if (STD_COLOR_SPACE(c->msc))
      ErrorF("Default color space is primary color space\n"); 
    if (PREFERRED_TIMING_MODE(c->msc))
      ErrorF("First detailed timing is preferred mode\n"); 
    if (GFT_SUPPORTED(c->msc))
      ErrorF("GTF timings supported\n"); 
}

static void 
print_whitepoint(struct disp_features *disp)
{
  ErrorF("redX: %.3f redY: %.3f   ",
	 disp->redx,disp->redy);
  ErrorF("greenX: %.3f greenY: %.3f\n",
	 disp->greenx,disp->greeny);
  ErrorF("blueX: %.3f blueY: %.3f   ",
	 disp->bluex,disp->bluey);
  ErrorF("whiteX: %.3f whiteY: %.3f\n",
	 disp->whitex,disp->whitey);
}

static void 
print_established_timings(struct established_timings *t)
{
    unsigned char c;

    ErrorF("Supported VESA Video Modes:\n");
    c=t->t1;
    if (c&0x80) ErrorF("720x400@70Hz\n");
    if (c&0x40) ErrorF("720x400@88Hz\n");
    if (c&0x20) ErrorF("640x480@60Hz\n");
    if (c&0x10) ErrorF("640x480@67Hz\n");
    if (c&0x08) ErrorF("640x480@72Hz\n");
    if (c&0x04) ErrorF("640x480@75Hz\n");
    if (c&0x02) ErrorF("800x600@56Hz\n");
    if (c&0x01) ErrorF("800x600@60Hz\n");
    c=t->t2;
    if (c&0x80) ErrorF("800x600@72Hz\n");
    if (c&0x40) ErrorF("800x600@75Hz\n");
    if (c&0x20) ErrorF("832x624@75Hz\n");
    if (c&0x10) ErrorF("1024x768@87Hz (interlaced)\n");
    if (c&0x08) ErrorF("1024x768@60Hz\n");
    if (c&0x04) ErrorF("1024x768@70Hz\n");
    if (c&0x02) ErrorF("1024x768@75Hz\n");
    if (c&0x01) ErrorF("1280x1024@75Hz\n");
    c=t->t_manu;
    if (c&0x80) ErrorF("1152x870@75Hz\n");
    ErrorF("Manufacturer's mask: %X\n",c&0x7F);
}

static void
print_std_timings(struct std_timings *t)
{
    int i;
    ErrorF("Supported Future Video Modes:\n");
    for (i=0;i<STD_TIMINGS;i++) {
	if (t[i].hsize > 256) {  /* sanity check */
	    ErrorF("#%i: hsize: %i  vsize %i  refresh: %i  vid: %i\n",
		   i, t[i].hsize, t[i].vsize, t[i].refresh, t[i].id);
	}
    }
}

static void
print_detailed_monitor_section(struct detailed_monitor_section *m)
{
  int i,j;

  for (i=0;i<DET_TIMINGS;i++) {
    switch (m[i].type) {
    case DT:
      print_detailed_timings(&m[i].section.d_timings);
      break;
    case DS_SERIAL:
      ErrorF("Serial No: %s\n",m[i].section.serial);
      break;
    case DS_ASCII_STR:
      ErrorF(" %s\n",m[i].section.ascii_data);
      break;
    case DS_NAME:
      ErrorF("Monitor name: %s\n",m[i].section.name);
      break;
    case DS_RANGES:
      ErrorF("Ranges: V min: %i  V max: %i Hz, H min: %i  H max: %i kHz,",
	     m[i].section.ranges.min_v, m[i].section.ranges.max_v, 
	     m[i].section.ranges.min_h, m[i].section.ranges.max_h);
      if (m[i].section.ranges.max_clock != 0)
	ErrorF(" PixClock max %i MHz\n",m[i].section.ranges.max_clock);
      else
	ErrorF("\n");
      break;
    case DS_STD_TIMINGS:
      for (j = 0; j<5; i++) 
	ErrorF("#%i: hsize: %i  vsize %i  refresh: %i  "
	       "vid: %i\n",i,m[i].section.std_t[i].hsize,
	       m[i].section.std_t[j].vsize,m[i].section.std_t[j].refresh,
	       m[i].section.std_t[j].id);
      break;
    case DS_WHITE_P:
      for (j = 0; j<2; i++)
	if (m[i].section.wp[j].index != 0)
	  ErrorF("White point %i: whiteX: %f, whiteY: %f; gamma: %f\n",
		 m[i].section.wp[j].index,m[i].section.wp[j].white_x,
		 m[i].section.wp[j].white_y, m[i].section.wp[j].white_gamma);
      break;
    }
  }
}

static void
print_detailed_timings(struct detailed_timings *t)
{

  if (t->clock > 15000000) {  /* sanity check */
    ErrorF("Supported additional Video Mode:\n");
    ErrorF("clock: %.1f MHz   ",t->clock/1000000.0);
    ErrorF("Image Size:  %i x %i mm\n",t->h_size,t->v_size); 
    ErrorF("h_active: %i  h_sync: %i  h_sync_end %i h_blank_end %i ",
	   t->h_active, t->h_sync_off + t->h_active,
	   t->h_sync_off + t->h_sync_width + t->h_active,
	   t->h_active + t->h_blanking);
    ErrorF("h_border: %i\n",t->h_border);
    ErrorF("v_active: %i  v_sync: %i  v_sync_end %i v_blanking: %i ",
	   t->v_active, t->v_sync_off + t->v_active,
	   t->v_sync_off + t->v_sync_width + t->v_active,
	   t->v_active + t->v_blanking);
    ErrorF("v_border: %i\n",t->v_border);
    if (IS_STEREO(t->stereo)) {
      ErrorF("Stereo: ");
      if (IS_RIGHT_ON_SYNC(t->stereo)) 
	ErrorF("right channel on sync\n");
      else ErrorF("right channel on sync\n");
    }
  }
}
