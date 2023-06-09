/* $TOG: mkdirhier.c /main/3 1998/02/06 11:24:37 kaleb $ */
/*

Copyright (C) 1996, 1998 The Open Group

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

/*
 * Simple mkdirhier program for Windows NT
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>

char *
next_sep(char *path)
{
    while (*path)
	if (*path == '/' || *path == '\\')
	    return path;
	else
	    path++;
    return NULL;
}

int
main(int argc, char *argv[])
{
    char *dirname, *next, *prev;
    char buf[1024];
    struct _stat sb;

    if (argc < 2)
	exit(1);
    dirname = argv[1];

    prev = dirname;
    while (next = next_sep(prev)) {
	strncpy(buf, dirname, next - dirname);
	buf[next - dirname] = '\0';
	/* if parent dir doesn't exist yet create it */
	if (_stat(buf, &sb))
	    _mkdir(buf);	/* no error checking to avoid barfing on C: */
	prev = next + 1;
    }
    if (_mkdir(dirname) == -1) {
	perror("mkdirhier failed");
	exit(1);
    }
    exit(0);
}
