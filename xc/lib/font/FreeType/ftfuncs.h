/* $XFree86: xc/lib/font/FreeType/ftfuncs.h,v 1.8 1999/10/13 04:20:50 dawes Exp $ */

/* Number of buckets in the hashtable holding faces */
#define NUMFACEBUCKETS 32

/* Glyphs are held in segments of this size */
#define FONTSEGMENTSIZE 16

/* A structure that holds bitmap order and padding info. */

typedef struct {
  int bit;                      /* bit order */
  int byte;                     /* byte order */
  int glyph;                    /* glyph pad size */
  int scan;                     /* machine word size */
} FontBitmapFormat;

/* The data structures for holding font data */

/* Forward reference */

struct TTFInstance_s;

/* At the lowest level, there is face; TTFFaces are in one-to-one
 * correspondence with TrueType faces.  Multiple instance may share
 * the same face. */
typedef struct TTFFace_s {
  char *filename;
  TT_Face face;
  TT_Glyph glyph;
  TT_Face_Properties properties;
  struct TTFInstance_s *instances; /* linked list of associated instances */
  struct TTFFace_s *next;       /* link to next face in bucket */
} TTFFace;

/* A transformation matrix with resolution information */
typedef struct {
  double scale;
  int nonIdentity;              /* if 0, matrix is the identity */
  TT_Matrix matrix;
  int xres, yres;
} TTFNormalisedTransformation;

/* An instance builds on a face by specifying the transformation
 * matrix.  Multiple fonts may share the same instance. */

/* This structure caches bitmap data */
typedef struct TTFInstance_s {
  TTFFace *face;                /* the associated face */
  TT_Instance instance;
  TT_Instance_Metrics imetrics;
  TTFNormalisedTransformation transformation;
  int monospaced;               /* 1 if it is a monospaced instance,
                                 * 2 if it is a charcell instance */
  int width;                    /* the width of all glyphs if monospaced */
  xCharInfo *charcellMetrics;   /* the metrics if charcell is 1 */
  FontBitmapFormat bmfmt;
  unsigned nglyphs;
  CharInfoPtr *glyphs;          /* glyphs and available are used in parallel */
  int **available;              /* 0=unknown */
                                /* 1=known not to exist */
                                /* 2=known to exist, not rasterised */
                                /* 3=rasterised, glyph available */
#ifdef ANTI_ALIASING
  CharInfoPtr *glyphsAnti;
  int **availableAnti;
#endif
  int refcount;
  struct TTFInstance_s *next;   /* link to next instance */
} TTFInstance;

/* A font is an instance with coding information; fonts are in
 * one-to-one correspondence with X fonts */
typedef struct {
  TTFInstance *instance;
  struct ttf_mapping mapping;   /* defined in ft.h */
  int nranges;
  fsRange *ranges;
} TTFFont;


/* Prototypes for some local functions */

static int FreeTypeOpenFace(TTFFace **facep, char *fileName);
static void FreeTypeFreeFace(TTFFace *face);
static int 
 FreeTypeOpenInstance(TTFInstance **instancep, 
                      char *fileName, TTFNormalisedTransformation *trans,
                      int charcell, FontBitmapFormat *bmfmt);
static void FreeTypeFreeInstance(TTFInstance *instance);
static int
 FreeTypeInstanceGetGlyph(unsigned idx, CharInfoPtr *g, TTFInstance *instance,
                          int nlevels);
static int 
FreeTypeRasteriseGlyph(CharInfoPtr tgp, TTFInstance *instance,
                       int hasMetrics, int nlevels);
static void FreeTypeFreeFont(TTFFont *font);
static void FreeTypeFreeXFont(FontPtr pFont, int freeProps);
static void FreeTypeUnloadXFont(FontPtr pFont);
static int
FreeTypeAddProperties(TTFFont *font, FontScalablePtr vals, FontInfoPtr info, 
                      char *fontname, 
                      int rawAverageWidth);
static int FreeTypeFontGetGlyph(unsigned code, CharInfoPtr *g, TTFFont *font, int nlevels);
static int FreeTypeFontGetDefaultGlyph(CharInfoPtr *g, TTFFont *font, int nlevels);
static int
FreeTypeLoadFont(TTFFont **fontp, char *fileName,
                 FontScalablePtr vals, FontEntryPtr entry,
                 FontBitmapFormat *bmfmt);


