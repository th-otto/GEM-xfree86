/* $TOG: pf.c /main/6 1998/02/09 14:11:26 kaleb $ */
/*

Copyright 1988, 1998  The Open Group

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

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <ctype.h>
#include "xmodmap.h"

#define NOTINFILEFILENAME "commandline"
char *inputFilename = NOTINFILEFILENAME;
int lineno = 0;

void process_file (filename)
    char *filename;			/* NULL means use stdin */
{
    FILE *fp;
    char buffer[BUFSIZ];

    /* open the file, eventually we'll want to pipe through cpp */

    if (!filename) {
	fp = stdin;
	inputFilename = "stdin"; 
    } else {
	fp = fopen (filename, "r");
	if (!fp) {
	    fprintf (stderr, "%s:  unable to open file '%s' for reading\n",
		     ProgramName, filename);
	    parse_errors++;
	    return;
	}
	inputFilename = filename;
    }


    /* read the input and filter */

    if (verbose) {
	printf ("! %s:\n", inputFilename);
    }

    for (lineno = 0; ; lineno++) {
	buffer[0] = '\0';
	if (fgets (buffer, BUFSIZ, fp) == NULL)
	  break;

	process_line (buffer);
    }

    inputFilename = NOTINFILEFILENAME;
    lineno = 0;
    (void) fclose (fp);
}


void process_line (buffer)
    char *buffer;
{
    int len;
    int i;
    char *cp;

    len = strlen (buffer);

    for (i = 0; i < len; i++) {		/* look for blank lines */
	register char c = buffer[i];
	if (!(isspace(c) || c == '\n')) break;
    }
    if (i == len) return;

    cp = &buffer[i];

    if (*cp == '!') return;		/* look for comments */
    len -= (cp - buffer);		/* adjust len by how much we skipped */

					/* pipe through cpp */

					/* strip trailing space */
    for (i = len-1; i >= 0; i--) {
	register char c = cp[i];
	if (!(isspace(c) || c == '\n')) break;
    }
    if (i >= 0) cp[len = (i+1)] = '\0';  /* nul terminate */

    if (verbose) {
	printf ("! %d:  %s\n", lineno, cp);
    }

    /* handle input */
    handle_line (cp, len);
}
