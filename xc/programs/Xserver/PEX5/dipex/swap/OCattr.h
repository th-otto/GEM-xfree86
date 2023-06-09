/* $TOG: OCattr.h /main/4 1998/02/10 12:36:22 kaleb $ */

/***********************************************************

Copyright 1989, 1990, 1991, 1998  The Open Group

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

Copyright 1989, 1990, 1991 by Sun Microsystems, Inc. 

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Sun Microsystems,
not be used in advertising or publicity pertaining to distribution of 
the software without specific, written prior permission.  

SUN MICROSYSTEMS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT 
SHALL SUN MICROSYSTEMS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

LOCAL_FLAG ErrorCode
	SwapPEXMarkerType (),
	SwapPEXMarkerScale (),
	SwapPEXMarkerColourIndex (),
	SwapPEXMarkerBundleIndex (),
	SwapPEXAtextStyle (),
	SwapPEXTextPrecision (),
	SwapPEXCharExpansion (),
	SwapPEXCharSpacing (),
	SwapPEXCharHeight (),
	SwapPEXCharUpVector (),
	SwapPEXTextPath (),
	SwapPEXTextAlignment (),
	SwapPEXLineType (),
	SwapPEXLineWidth (),
	SwapPEXLineColourIndex (),
	SwapPEXCurveApproximation (),
	SwapPEXPolylineInterp (),
	SwapPEXInteriorStyle (),
	SwapPEXSurfaceColourIndex (),
	SwapPEXSurfaceReflModel (),
	SwapPEXSurfaceInterp (),
	SwapPEXSurfaceApproximation (),
	SwapPEXCullingMode (),
	SwapPEXDistinguishFlag (),
	SwapPEXPatternSize (),
	SwapPEXPatternRefPt (),
	SwapPEXPatternAttr (),
	SwapPEXSurfaceEdgeFlag (),
	SwapPEXSurfaceEdgeType (),
	SwapPEXSurfaceEdgeWidth (),
	SwapPEXSetAsfValues (),
	SwapPEXLocalTransform (),
	SwapPEXLocalTransform2D (),
	SwapPEXGlobalTransform (),
	SwapPEXGlobalTransform2D (),
	SwapPEXModelClip (),
	SwapPEXRestoreModelClip (),
	SwapPEXPickId (),
	SwapPEXHlhsrIdentifier (),
	SwapPEXExecuteStructure (),
	SwapPEXLabel (),
	SwapPEXApplicationData (),
	SwapPEXGse (),
	SwapPEXRenderingColourModel (),
	SwapPEXOCUnused ();

LOCAL_FLAG unsigned char * SwapCoord4DList ();
LOCAL_FLAG unsigned char * SwapCoord3DList ();
LOCAL_FLAG unsigned char * SwapCoord2DList ();
LOCAL_FLAG unsigned char * SwapColour ();
LOCAL_FLAG unsigned char * SwapOptData();
LOCAL_FLAG unsigned char * SwapVertex();
LOCAL_FLAG unsigned char * SwapTrimCurve();

LOCAL_FLAG void
	SwapSurfaceApprox(),
	SwapHalfSpace (),
	SwapHalfSpace2D ();
	
