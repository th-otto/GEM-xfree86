/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/configProcs.h,v 1.12 1999/09/04 13:04:54 dawes Exp $ */

/* Private procs.  Public procs are in xf86Parser.h and xf86Optrec.h */

/* Device.c */
XF86ConfDevicePtr parseDeviceSection(void);
void printDeviceSection(FILE *cf, XF86ConfDevicePtr ptr);
void freeDeviceList(XF86ConfDevicePtr ptr);
int validateDevice(XF86ConfigPtr p);
/* Files.c */
XF86ConfFilesPtr parseFilesSection(void);
void printFileSection(FILE *cf, XF86ConfFilesPtr ptr);
void freeFiles(XF86ConfFilesPtr p);
/* Flags.c */
XF86ConfFlagsPtr parseFlagsSection(void);
void printServerFlagsSection(FILE *f, XF86ConfFlagsPtr flags);
void freeFlags(XF86ConfFlagsPtr flags);
/* Input.c */
XF86ConfInputPtr parseInputSection(void);
void printInputSection(FILE *f, XF86ConfInputPtr ptr);
void freeInputList(XF86ConfInputPtr ptr);
int validateInput (XF86ConfigPtr p);
/* Keyboard.c */
XF86ConfInputPtr parseKeyboardSection(void);
/* Layout.c */
XF86ConfLayoutPtr parseLayoutSection(void);
void printLayoutSection(FILE *cf, XF86ConfLayoutPtr ptr);
void freeLayoutList(XF86ConfLayoutPtr ptr);
void freeAdjacencyList(XF86ConfAdjacencyPtr ptr);
void freeInputrefList(XF86ConfInputrefPtr ptr);
int validateLayout(XF86ConfigPtr p);
/* Module.c */
XF86LoadPtr parseModuleSubSection(XF86LoadPtr head, char *name);
XF86ConfModulePtr parseModuleSection(void);
void printModuleSection(FILE *cf, XF86ConfModulePtr ptr);
XF86LoadPtr addNewLoadDirective(XF86LoadPtr head, char *name, int type, XF86OptionPtr opts);
void freeModules(XF86ConfModulePtr ptr);
/* Monitor.c */
XF86ConfModeLinePtr parseModeLine(void);
XF86ConfModeLinePtr parseVerboseMode(void);
XF86ConfMonitorPtr parseMonitorSection(void);
XF86ConfModesPtr parseModesSection(void);
void printMonitorSection(FILE *cf, XF86ConfMonitorPtr ptr);
void printModesSection(FILE *cf, XF86ConfModesPtr ptr);
void freeMonitorList(XF86ConfMonitorPtr ptr);
void freeModesList(XF86ConfModesPtr ptr);
void freeModeLineList(XF86ConfModeLinePtr ptr);
int validateMonitor(XF86ConfigPtr p, XF86ConfScreenPtr screen);
/* Pointer.c */
XF86ConfInputPtr parsePointerSection(void);
/* Screen.c */
XF86ConfDisplayPtr parseDisplaySubSection(void);
XF86ConfScreenPtr parseScreenSection(void);
void printScreenSection(FILE *cf, XF86ConfScreenPtr ptr);
void freeScreenList(XF86ConfScreenPtr ptr);
void freeAdaptorLinkList(XF86ConfAdaptorLinkPtr ptr);
void freeDisplayList(XF86ConfDisplayPtr ptr);
void freeModeList(XF86ModePtr ptr);
int validateScreen(XF86ConfigPtr p);
/* Vendor.c */
XF86ConfVendorPtr parseVendorSection(void);
void freeVendorList(XF86ConfVendorPtr p);
void printVendorSection(FILE * cf, XF86ConfVendorPtr ptr);
/* Video.c */
XF86ConfVideoPortPtr parseVideoPortSubSection(void);
XF86ConfVideoAdaptorPtr parseVideoAdaptorSection(void);
void printVideoAdaptorSection(FILE *cf, XF86ConfVideoAdaptorPtr ptr);
void freeVideoAdaptorList(XF86ConfVideoAdaptorPtr ptr);
void freeVideoPortList(XF86ConfVideoPortPtr ptr);
int validateConfig(XF86ConfigPtr p);
/* scan.c */
unsigned int StrToUL(char *str);
int xf86GetToken(xf86ConfigSymTabRec *tab);
void xf86UnGetToken(int token);
char *xf86TokenString(void);
void xf86ParseError(char *format, ...);
void xf86ParseWarning(char *format, ...);
void xf86ValidationError(char *format, ...);
void SetSection(char *section);
int getStringToken(xf86ConfigSymTabRec *tab);
/* write.c */
/* DRI.c */
XF86ConfBuffersPtr parseBuffers (void);
void freeBuffersList (XF86ConfBuffersPtr ptr);
XF86ConfDRIPtr parseDRISection (void);
void printDRISection (FILE * cf, XF86ConfDRIPtr ptr);
void freeDRI (XF86ConfDRIPtr ptr);

/* Externally provided functions */
void ErrorF(const char *f, ...);
void VErrorF(const char *f, va_list args);
