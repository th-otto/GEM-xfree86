/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Brian Paul <brian@precisioninsight.com>
 *
 */

#include <GL/gl.h>
#include "glapi.h"
#include "glapitable.h"


#define KEYWORD1

#define KEYWORD2

#if defined(USE_X86_ASM)
#define NAME(func) _glapi_fallback_##func
#else
#define NAME(func) gl##func
#endif

#define DISPATCH(func, args, msg)					\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   (dispatch->func) args

#define RETURN_DISPATCH(func, args, msg) 				\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   return (dispatch->func) args


#include "glapitemp.h"





