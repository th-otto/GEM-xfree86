/* $TOG: pl_lut.h /main/5 1998/02/06 16:10:34 kaleb $ */
/*

Copyright 1992, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifndef WORD64

#define BEGIN_LUTENTRY_HEADER(_name, _pBuf, _pEntry) \
    _pEntry = (_name *) _pBuf; 

#define END_LUTENTRY_HEADER(_name, _pBuf, _pEntry) \
    _pBuf += SIZEOF (_name);

#else /* WORD64 */

#define BEGIN_LUTENTRY_HEADER(_name, _pBuf, _pEntry) \
{ \
    _name tEntry; \
    _pEntry = &tEntry;

#define END_LUTENTRY_HEADER(_name, _pBuf, _pEntry) \
    memcpy (_pBuf, _pEntry, SIZEOF (_name)); \
    _pBuf += SIZEOF (_name); \
}

#endif /* WORD64 */


static PEXPointer _PEXRepackLUTEntries();

#define GetLUTEntryBuffer(_numEntries, _entryType, _buf) \
    (_buf) = (PEXPointer) Xmalloc ( \
	(unsigned) ((_numEntries) * (sizeof (_entryType))));



