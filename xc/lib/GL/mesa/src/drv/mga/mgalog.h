/*
 * GLX Hardware Device Driver for Matrox Millenium G200
 * Copyright (C) 1999 Wittawat Yamwong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 */
 
/* Usage:
 * - use mgaError for error messages. Always write to X error and log file.
 * - use mgaMsg for debugging. Can be disabled by undefining MGA_LOG_ENABLED.
 */
 
#ifndef MGALOG_INC
#define MGALOG_INC
#include "hwlog.h"

/* Mapping between old function names and new common code: */
/* (Feel free to replace all mgaMsg with hwMsg etc. in all *.c
 * files, I was to lazy to do this...) */
#define mgaOpenLog(f) 	hwOpenLog(f,"[mga] ")
#define mgaCloseLog 	hwCloseLog
#define mgaIsLogReady 	hwIsLogReady
#define mgaSetLogLevel  hwSetLogLevel
#define mgaGetLogLevel  hwGetLogLevel
#define mgaMsg		hwMsg
#define mgaError	hwError

#endif
