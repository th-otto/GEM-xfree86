/* $TOG: config.h /main/5 1998/02/09 13:45:26 kaleb $ */

/*
Copyright 1996, 1998  The Open Group

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


Status
GetConfig (
    char *configFile,
    char *proxyService,
    Bool *managed,
    char **startCommand,
    char **proxyAddress);

#ifdef NEED_STRCASECMP
int
ncasecmp (
    char *str1,
    char *str2,
    int n);
#else
#include <string.h>
#define ncasecmp(s1,s2,n) strncasecmp(s1,s2,n)
#endif /* NEED_STRCASECMP */
