/*
 * $TOG: mipsstack.s /main/4 1998/02/09 11:40:17 kaleb $
 *
Copyright 1992, 1998  The Open Group

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

	.globl	getReturnAddress
	.ent	getReturnAddress
getReturnAddress:
	.frame	$sp, 0, $31
	move	$2,$31
	j	$31
	.end	getReturnAddress

	.globl	getStackPointer
	.ent	getStackPointer
getStackPointer:
	.frame	$sp, 0, $31
	move	$2,$29
	j	$31
	.end	getStackPointer
