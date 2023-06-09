/*
 * $TOG: cfbmap.h /main/12 1998/02/09 14:06:19 kaleb $
 *
Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/* $XFree86: xc/programs/Xserver/cfb/cfbmap.h,v 3.8 1999/12/13 02:13:07 robin Exp $ */

/*
 * Map names around so that multiple depths can be supported simultaneously
 */

#if 0
#undef QuartetBitsTable
#undef QuartetPixelMaskTable
#undef cfb8ClippedLineCopy
#undef cfb8ClippedLineGeneral 
#undef cfb8ClippedLineXor
#undef cfb8LineSS1Rect
#undef cfb8LineSS1RectCopy
#undef cfb8LineSS1RectGeneral 
#undef cfb8LineSS1RectPreviousCopy
#undef cfb8LineSS1RectXor
#undef cfb8SegmentSS1Rect
#undef cfb8SegmentSS1RectCopy
#undef cfb8SegmentSS1RectGeneral 
#undef cfb8SegmentSS1RectShiftCopy
#undef cfb8SegmentSS1RectXor
#undef cfbAllocatePrivates
#undef cfbBSFuncRec
#undef cfbBitBlt
#undef cfbBresD
#undef cfbBresS
#undef cfbChangeWindowAttributes
#undef cfbCloseScreen
#undef cfbCopyArea
#undef cfbCopyImagePlane
#undef cfbCopyPixmap
#undef cfbCopyPlane
#undef cfbCopyRotatePixmap
#undef cfbCopyWindow
#undef cfbCreateGC
#undef cfbCreatePixmap
#undef cfbCreateScreenResources
#undef cfbCreateWindow
#undef cfbDestroyPixmap
#undef cfbDestroyWindow
#undef cfbDoBitblt
#undef cfbDoBitbltCopy
#undef cfbDoBitbltGeneral
#undef cfbDoBitbltOr
#undef cfbDoBitbltXor
#undef cfbFillBoxSolid
#undef cfbFillBoxTile32
#undef cfbFillBoxTile32sCopy
#undef cfbFillBoxTile32sGeneral
#undef cfbFillBoxTileOdd
#undef cfbFillBoxTileOddCopy
#undef cfbFillBoxTileOddGeneral
#undef cfbFillPoly1RectCopy
#undef cfbFillPoly1RectGeneral
#undef cfbFillRectSolidCopy
#undef cfbFillRectSolidGeneral
#undef cfbFillRectSolidXor
#undef cfbFillRectTile32Copy
#undef cfbFillRectTile32General
#undef cfbFillRectTileOdd
#undef cfbFillSpanTile32sCopy
#undef cfbFillSpanTile32sGeneral
#undef cfbFillSpanTileOddCopy
#undef cfbFillSpanTileOddGeneral
#undef cfbFinishScreenInit
#undef cfbGCFuncs
#undef cfbGetImage
#undef cfbGetScreenPixmap
#undef cfbGetSpans
#undef cfbHorzS
#undef cfbImageGlyphBlt8
#undef cfbLineSD
#undef cfbLineSS
#undef cfbMapWindow
#undef cfbMatchCommon
#undef cfbNonTEOps
#undef cfbNonTEOps1Rect
#undef cfbPadPixmap
#undef cfbPaintWindow
#undef cfbPolyFillArcSolidCopy
#undef cfbPolyFillArcSolidGeneral
#undef cfbPolyFillRect
#undef cfbPolyGlyphBlt8
#undef cfbPolyGlyphRop8
#undef cfbPolyPoint
#undef cfbPositionWindow
#undef cfbPutImage
#undef cfbReduceRasterOp
#undef cfbRestoreAreas
#undef cfbSaveAreas
#undef cfbScreenInit
#undef cfbScreenPrivateIndex
#undef cfbSegmentSD
#undef cfbSegmentSS
#undef cfbSetScanline
#undef cfbSetScreenPixmap
#undef cfbSetSpans
#undef cfbSetupScreen
#undef cfbSolidSpansCopy
#undef cfbSolidSpansGeneral
#undef cfbSolidSpansXor
#undef cfbStippleStack
#undef cfbStippleStackTE
#undef cfbTEGlyphBlt
#undef cfbTEOps
#undef cfbTEOps1Rect
#undef cfbTile32FSCopy
#undef cfbTile32FSGeneral
#undef cfbUnmapWindow
#undef cfbUnnaturalStippleFS
#undef cfbUnnaturalTileFS
#undef cfbValidateGC
#undef cfbVertS
#undef cfbXRotatePixmap
#undef cfbYRotatePixmap
#undef cfbZeroPolyArcSS8Copy
#undef cfbZeroPolyArcSS8General
#undef cfbZeroPolyArcSS8Xor
#undef cfbendpartial
#undef cfbendtab
#undef cfbmask
#undef cfbrmask
#undef cfbstartpartial
#undef cfbstarttab
#endif

/* a losing vendor cpp dumps core if we define CFBNAME in terms of CATNAME */

#if PSZ != 8

#if PSZ == 32
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CFBNAME(subname) cfb32##subname
#else
#define CFBNAME(subname) cfb32/**/subname
#endif
#endif

#if PSZ == 24
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CFBNAME(subname) cfb24##subname
#else
#define CFBNAME(subname) cfb24/**/subname
#endif
#endif

#if PSZ == 16
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CFBNAME(subname) cfb16##subname
#else
#define CFBNAME(subname) cfb16/**/subname
#endif
#endif

#if PSZ == 4
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CFBNAME(subname) cfb4##subname
#else
#define CFBNAME(subname) cfb4/**/subname
#endif
#endif

#ifndef CFBNAME
cfb can not hack PSZ yet
#endif

#undef CATNAME

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif

#define QuartetBitsTable CFBNAME(QuartetBitsTable)
#define QuartetPixelMaskTable CFBNAME(QuartetPixelMaskTable)
#define cfb8ClippedLineCopy CFBNAME(ClippedLineCopy)
#define cfb8ClippedLineGeneral  CFBNAME(ClippedLineGeneral )
#define cfb8ClippedLineXor CFBNAME(ClippedLineXor)
#define cfb8LineSS1Rect CFBNAME(LineSS1Rect)
#define cfb8LineSS1RectCopy CFBNAME(LineSS1RectCopy)
#define cfb8LineSS1RectGeneral  CFBNAME(LineSS1RectGeneral )
#define cfb8LineSS1RectPreviousCopy CFBNAME(LineSS1RectPreviousCopy)
#define cfb8LineSS1RectXor CFBNAME(LineSS1RectXor)
#define cfb8SegmentSS1Rect CFBNAME(SegmentSS1Rect)
#define cfb8SegmentSS1RectCopy CFBNAME(SegmentSS1RectCopy)
#define cfb8SegmentSS1RectGeneral  CFBNAME(SegmentSS1RectGeneral )
#define cfb8SegmentSS1RectShiftCopy CFBNAME(SegmentSS1RectShiftCopy)
#define cfb8SegmentSS1RectXor CFBNAME(SegmentSS1RectXor)
#define cfbAllocatePrivates CFBNAME(AllocatePrivates)
#define cfbBSFuncRec CFBNAME(BSFuncRec)
#define cfbBitBlt CFBNAME(BitBlt)
#define cfbBresD CFBNAME(BresD)
#define cfbBresS CFBNAME(BresS)
#define cfbChangeWindowAttributes CFBNAME(ChangeWindowAttributes)
#define cfbCloseScreen CFBNAME(CloseScreen)
#define cfbCopyArea CFBNAME(CopyArea)
#define cfbCopyImagePlane CFBNAME(CopyImagePlane)
#define cfbCopyPixmap CFBNAME(CopyPixmap)
#define cfbCopyPlane CFBNAME(CopyPlane)
#define cfbCopyRotatePixmap CFBNAME(CopyRotatePixmap)
#define cfbCopyWindow CFBNAME(CopyWindow)
#define cfbCreateGC CFBNAME(CreateGC)
#define cfbCreatePixmap CFBNAME(CreatePixmap)
#define cfbCreateScreenResources CFBNAME(CreateScreenResources)
#define cfbCreateWindow CFBNAME(CreateWindow)
#define cfbDestroyPixmap CFBNAME(DestroyPixmap)
#define cfbDestroyWindow CFBNAME(DestroyWindow)
#define cfbDoBitblt CFBNAME(DoBitblt)
#define cfbDoBitbltCopy CFBNAME(DoBitbltCopy)
#define cfbDoBitbltGeneral CFBNAME(DoBitbltGeneral)
#define cfbDoBitbltOr CFBNAME(DoBitbltOr)
#define cfbDoBitbltXor CFBNAME(DoBitbltXor)
#define cfbFillBoxSolid CFBNAME(FillBoxSolid)
#define cfbFillBoxTile32 CFBNAME(FillBoxTile32)
#define cfbFillBoxTile32sCopy CFBNAME(FillBoxTile32sCopy)
#define cfbFillBoxTile32sGeneral CFBNAME(FillBoxTile32sGeneral)
#define cfbFillBoxTileOdd CFBNAME(FillBoxTileOdd)
#define cfbFillBoxTileOddCopy CFBNAME(FillBoxTileOddCopy)
#define cfbFillBoxTileOddGeneral CFBNAME(FillBoxTileOddGeneral)
#define cfbFillPoly1RectCopy CFBNAME(FillPoly1RectCopy)
#define cfbFillPoly1RectGeneral CFBNAME(FillPoly1RectGeneral)
#define cfbFillRectSolidCopy CFBNAME(FillRectSolidCopy)
#define cfbFillRectSolidGeneral CFBNAME(FillRectSolidGeneral)
#define cfbFillRectSolidXor CFBNAME(FillRectSolidXor)
#define cfbFillRectTile32Copy CFBNAME(FillRectTile32Copy)
#define cfbFillRectTile32General CFBNAME(FillRectTile32General)
#define cfbFillRectTileOdd CFBNAME(FillRectTileOdd)
#define cfbFillSpanTile32sCopy CFBNAME(FillSpanTile32sCopy)
#define cfbFillSpanTile32sGeneral CFBNAME(FillSpanTile32sGeneral)
#define cfbFillSpanTileOddCopy CFBNAME(FillSpanTileOddCopy)
#define cfbFillSpanTileOddGeneral CFBNAME(FillSpanTileOddGeneral)
#define cfbFinishScreenInit CFBNAME(FinishScreenInit)
#define cfbGCFuncs CFBNAME(GCFuncs)
#define cfbGetImage CFBNAME(GetImage)
#define cfbGetScreenPixmap CFBNAME(GetScreenPixmap)
#define cfbGetSpans CFBNAME(GetSpans)
#define cfbHorzS CFBNAME(HorzS)
#define cfbImageGlyphBlt8 CFBNAME(ImageGlyphBlt8)
#define cfbLineSD CFBNAME(LineSD)
#define cfbLineSS CFBNAME(LineSS)
#define cfbMapWindow CFBNAME(MapWindow)
#define cfbMatchCommon CFBNAME(MatchCommon)
#define cfbNonTEOps CFBNAME(NonTEOps)
#define cfbNonTEOps1Rect CFBNAME(NonTEOps1Rect)
#define cfbPadPixmap CFBNAME(PadPixmap)
#define cfbPaintWindow CFBNAME(PaintWindow)
#define cfbPolyFillArcSolidCopy CFBNAME(PolyFillArcSolidCopy)
#define cfbPolyFillArcSolidGeneral CFBNAME(PolyFillArcSolidGeneral)
#define cfbPolyFillRect CFBNAME(PolyFillRect)
#define cfbPolyGlyphBlt8 CFBNAME(PolyGlyphBlt8)
#define cfbPolyGlyphRop8 CFBNAME(PolyGlyphRop8)
#define cfbPolyPoint CFBNAME(PolyPoint)
#define cfbPositionWindow CFBNAME(PositionWindow)
#define cfbPutImage CFBNAME(PutImage)
#define cfbReduceRasterOp CFBNAME(ReduceRasterOp)
#define cfbRestoreAreas CFBNAME(RestoreAreas)
#define cfbSaveAreas CFBNAME(SaveAreas)
#define cfbScreenInit CFBNAME(ScreenInit)
#define cfbScreenPrivateIndex CFBNAME(ScreenPrivateIndex)
#define cfbSegmentSD CFBNAME(SegmentSD)
#define cfbSegmentSS CFBNAME(SegmentSS)
#define cfbSetScanline CFBNAME(SetScanline)
#define cfbSetScreenPixmap CFBNAME(SetScreenPixmap)
#define cfbSetSpans CFBNAME(SetSpans)
#define cfbSetupScreen CFBNAME(SetupScreen)
#define cfbSolidSpansCopy CFBNAME(SolidSpansCopy)
#define cfbSolidSpansGeneral CFBNAME(SolidSpansGeneral)
#define cfbSolidSpansXor CFBNAME(SolidSpansXor)
#define cfbStippleStack CFBNAME(StippleStack)
#define cfbStippleStackTE CFBNAME(StippleStackTE)
#define cfbTEGlyphBlt CFBNAME(TEGlyphBlt)
#define cfbTEOps CFBNAME(TEOps)
#define cfbTEOps1Rect CFBNAME(TEOps1Rect)
#define cfbTile32FSCopy CFBNAME(Tile32FSCopy)
#define cfbTile32FSGeneral CFBNAME(Tile32FSGeneral)
#define cfbUnmapWindow CFBNAME(UnmapWindow)
#define cfbUnnaturalStippleFS CFBNAME(UnnaturalStippleFS)
#define cfbUnnaturalTileFS CFBNAME(UnnaturalTileFS)
#define cfbValidateGC CFBNAME(ValidateGC)
#define cfbVertS CFBNAME(VertS)
#define cfbXRotatePixmap CFBNAME(XRotatePixmap)
#define cfbYRotatePixmap CFBNAME(YRotatePixmap)
#define cfbZeroPolyArcSS8Copy CFBNAME(ZeroPolyArcSSCopy)
#define cfbZeroPolyArcSS8General CFBNAME(ZeroPolyArcSSGeneral)
#define cfbZeroPolyArcSS8Xor CFBNAME(ZeroPolyArcSSXor)
#define cfbendpartial CFBNAME(endpartial)
#define cfbendtab CFBNAME(endtab)
#define cfbmask CFBNAME(mask)
#define cfbrmask CFBNAME(rmask)
#define cfbstartpartial CFBNAME(startpartial)
#define cfbstarttab CFBNAME(starttab)

#endif /* PSZ != 8 */
