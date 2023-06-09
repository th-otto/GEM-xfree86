/*
 * some bits copied from mprof by Ben Zorn
 *
 * Copyright (c) 1995 Jeffrey Hsu
 */

/* $XFree86: xc/util/memleak/getreti386.c,v 3.3 1999/12/27 00:56:42 robin Exp $ */

#define get_current_fp(first_local)	((unsigned)&(first_local) + 4)
#define prev_fp_from_fp(fp)		*((unsigned *) fp)
#define ret_addr_from_fp(fp)		*((unsigned *) (fp+4))

#ifdef __FreeBSD__
#define CRT0_ADDRESS		0x10d3
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
#define CRT0_ADDRESS            0x109a
#endif
#ifdef linux
#define CRT0_ADDRESS		0x804a3d0
#endif

static unsigned long
junk (int *foo)
{
    return (unsigned long) foo + 4;
}

void
getStackTrace(unsigned long *results, int max)
{

	int first_local;
	unsigned long fp, ret_addr;

	first_local = 0;
	fp = junk(&first_local);
	fp = get_current_fp(first_local);
	ret_addr = ret_addr_from_fp(fp);

	while (ret_addr > CRT0_ADDRESS && max-- > 1) {
	  *results++ = ret_addr;
	  fp = prev_fp_from_fp(fp);
	  if (!fp)
	    break;
	  ret_addr = ret_addr_from_fp(fp);
	}
        *results++ = 0;

}
