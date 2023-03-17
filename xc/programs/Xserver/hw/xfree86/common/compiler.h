/* $XFree86: xc/programs/Xserver/hw/xfree86/common/compiler.h,v 3.58 2000/03/05 16:59:10 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: compiler.h /main/16 1996/10/25 15:38:34 kaleb $ */

#ifndef _COMPILER_H
#define _COMPILER_H

#ifndef __STDC__
# ifdef signed
#  undef signed
# endif
# ifdef volatile
#  undef volatile
# endif
# ifdef const
#  undef const
# endif
# define signed /**/
# ifdef __GNUC__
#  define volatile __volatile__
#  define const __const__
#  ifdef PC98
#   undef NO_INLINE
#  endif
# else
#  define const /**/
#  ifdef PC98
#   define __inline__ /**/
#  endif
#  ifdef __HIGHC__
#   define __inline__ _Inline
#  endif
# endif /* __GNUC__ */
#endif /* !__STDC__ */

#if defined(IODEBUG) && defined(__GNUC__)
#define outb RealOutb
#define outw RealOutw
#define outl RealOutl
#define inb RealInb
#define inw RealInw
#define inl RealInl
#endif

#if defined(QNX4) /* Do this for now to keep Watcom happy */
#define outb outp
#define outw outpw
#define outl outpd 
#define inb inp
#define inw inpw
#define inl inpd

/* Define the ffs function for inlining */
extern int ffs(unsigned long);
#pragma aux ffs_ = \
        "bsf edx, eax"          \
        "jnz bits_set"          \
        "xor eax, eax"          \
        "jmp exit1"             \
        "bits_set:"             \
        "mov eax, edx"          \
        "inc eax"               \
        "exit1:"                \
        __parm [eax]            \
        __modify [eax edx]      \
        __value [eax]           \
        ;
#endif

#if defined(NO_INLINE) || defined(DO_PROTOTYPES)

#if !defined(__sparc__)

extern void outb(unsigned short, unsigned char);
extern void outw(unsigned short, unsigned short);
extern void outl(unsigned short, unsigned int);
extern unsigned int inb(unsigned short);
extern unsigned int inw(unsigned short);
extern unsigned int inl(unsigned short);

#else /* __sparc__ */

extern void outb(unsigned long, unsigned char);
extern void outw(unsigned long, unsigned short);
extern void outl(unsigned long, unsigned int);
extern unsigned int inb(unsigned long);
extern unsigned int inw(unsigned long);
extern unsigned int inl(unsigned long);

#endif /* __sparc__ */

extern unsigned long ldq_u(unsigned long *);
extern unsigned long ldl_u(unsigned int *);
extern unsigned long ldw_u(unsigned short *);
extern void stq_u(unsigned long, unsigned long *);
extern void stl_u(unsigned long, unsigned int *);
extern void stw_u(unsigned long, unsigned short *);
extern void mem_barrier(void);
extern void write_mem_barrier(void);
extern void stl_brx(unsigned long, volatile unsigned char *, int);
extern void stw_brx(unsigned short, volatile unsigned char *, int);
extern unsigned long ldl_brx(volatile unsigned char *, int);
extern unsigned short ldw_brx(volatile unsigned char *, int);
extern unsigned char rdinx(unsigned short, unsigned char);
extern void wrinx(unsigned short, unsigned char, unsigned char);
extern void modinx(unsigned short, unsigned char, unsigned char, unsigned char);
extern int testrg(unsigned short, unsigned char);
extern int testinx2(unsigned short, unsigned char, unsigned char);
extern int testinx(unsigned short, unsigned char);

#endif

#ifndef NO_INLINE

#ifdef __GNUC__

#if (defined(linux) || defined(__FreeBSD__)) && defined(__alpha__)

#ifdef linux
/* for Linux on Alpha, we use the LIBC _inx/_outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern void _outb(char val, unsigned short port);
static __inline__ void
outb(unsigned short port, unsigned char val)
{
    _outb(val, port);
}

extern void _outw(short val, unsigned short port);
static __inline__ void
outw(unsigned short port, unsigned short val)
{
    _outw(val, port);
}

extern void _outl(int val, unsigned short port);
static __inline__ void
outl(unsigned short port, unsigned int val)
{
    _outl(val, port);
}

extern unsigned int _inb(unsigned short port);
static __inline__ unsigned int
inb(unsigned short port)
{
  return _inb(port);
}

extern unsigned int _inw(unsigned short port);
static __inline__ unsigned int
inw(unsigned short port)
{
  return _inw(port);
}

extern unsigned int _inl(unsigned short port);
static __inline__ unsigned int
inl(unsigned short port)
{
  return _inl(port);
}

#endif /* linux */

#if defined(__FreeBSD__) && !defined(DO_PROTOTYPES)

/* for FreeBSD on Alpha, we use the libio inx/outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern void outb(unsigned int port, unsigned char val);
extern void outw(unsigned int port, unsigned short val);
extern void outl(unsigned int port, unsigned int val);
extern unsigned char inb(unsigned int port);
extern unsigned short inw(unsigned int port);
extern unsigned int inl(unsigned int port);

#endif /* __FreeBSD__ && !DO_PROTOTYPES */

/*
 * inline functions to do unaligned accesses
 * from linux/include/asm-alpha/unaligned.h
 */

/*
 * EGCS 1.1 knows about arbitrary unaligned loads.  Define some
 * packed structures to talk about such things with.
 */

struct __una_u64 { unsigned long  x __attribute__((packed)); };
struct __una_u32 { unsigned int   x __attribute__((packed)); };
struct __una_u16 { unsigned short x __attribute__((packed)); };

/*
 * Elemental unaligned loads 
 */
/* let's try making these things static */

static __inline__ unsigned long ldq_u(unsigned long * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	const struct __una_u64 *ptr = (const struct __una_u64 *) r11;
	return ptr->x;
#else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extql %0,%2,%0\n\t"
		"extqh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(7+(char *) r11)));
	return r1 | r2;
#endif
}

static __inline__ unsigned long ldl_u(unsigned int * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	const struct __una_u32 *ptr = (const struct __una_u32 *) r11;
	return ptr->x;
#else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extll %0,%2,%0\n\t"
		"extlh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(3+(char *) r11)));
	return r1 | r2;
#endif
}

static __inline__ unsigned long ldw_u(unsigned short * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	const struct __una_u16 *ptr = (const struct __una_u16 *) r11;
	return ptr->x;
#else
	unsigned long r1,r2;
	__asm__("ldq_u %0,%3\n\t"
		"ldq_u %1,%4\n\t"
		"extwl %0,%2,%0\n\t"
		"extwh %1,%2,%1"
		:"=&r" (r1), "=&r" (r2)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(const unsigned long *)(1+(char *) r11)));
	return r1 | r2;
#endif
}

/*
 * Elemental unaligned stores 
 */

static __inline__ void stq_u(unsigned long r5, unsigned long * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	struct __una_u64 *ptr = (struct __una_u64 *) r11;
	ptr->x = r5;
#else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"insqh %6,%7,%5\n\t"
		"insql %6,%7,%4\n\t"
		"mskqh %3,%7,%3\n\t"
		"mskql %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(7+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#endif
}

static __inline__ void stl_u(unsigned long r5, unsigned int * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	struct __una_u32 *ptr = (struct __una_u32 *) r11;
	ptr->x = r5;
#else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"inslh %6,%7,%5\n\t"
		"insll %6,%7,%4\n\t"
		"msklh %3,%7,%3\n\t"
		"mskll %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(3+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#endif
}

static __inline__ void stw_u(unsigned long r5, unsigned short * r11)
{
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 91
	struct __una_u16 *ptr = (struct __una_u16 *) r11;
	ptr->x = r5;
#else
	unsigned long r1,r2,r3,r4;

	__asm__("ldq_u %3,%1\n\t"
		"ldq_u %2,%0\n\t"
		"inswh %6,%7,%5\n\t"
		"inswl %6,%7,%4\n\t"
		"mskwh %3,%7,%3\n\t"
		"mskwl %2,%7,%2\n\t"
		"bis %3,%5,%3\n\t"
		"bis %2,%4,%2\n\t"
		"stq_u %3,%1\n\t"
		"stq_u %2,%0"
		:"=m" (*r11),
		 "=m" (*(unsigned long *)(1+(char *) r11)),
		 "=&r" (r1), "=&r" (r2), "=&r" (r3), "=&r" (r4)
		:"r" (r5), "r" (r11));
#endif
}

#define mem_barrier()        __asm__ __volatile__("mb"  : : : "memory")
#ifdef __ELF__
#define write_mem_barrier()  __asm__ __volatile__("wmb" : : : "memory")
#else  /*  ECOFF gas 2.6 doesn't know "wmb" :-(  */
#define write_mem_barrier()  mem_barrier()
#endif


#elif defined(linux) && defined(__ia64__) 
#define inline __inline__
 
#include <stdlib.h>
#include <asm/types.h>
#include <asm/system.h>
#include <sys/io.h>
#include <asm/unaligned.h>
  
#define ldq_u(p)        __uldq(p)
#define ldl_u(p)        __uldl(p)
#define ldw_u(p)        __uldw(p) 
#define stq_u(v,p)      __ustq(v,p)
#define stl_u(v,p)      __ustl(v,p)
#define stw_u(v,p)      __ustw(v,p)
  
#define mem_barrier()        __asm__ __volatile__ ("mf" ::: "memory")
#define write_mem_barrier()  __asm__ __volatile__ ("mf" ::: "memory")

#undef outb
#undef outw
#undef outl
 
#define outb(a,b)	_outb(b,a)
#define outw(a,b)	_outw(b,a)
#define outl(a,b)	_outl(b,a) 

#elif (defined(linux) || defined(Lynx)) && defined(__sparc__)

#if !defined(Lynx)
#ifndef ASI_PL
#define ASI_PL 0x88
#endif

static __inline__ void outb(unsigned long port, unsigned char val)
{
	__asm__ __volatile__("stba %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ void outw(unsigned long port, unsigned short val)
{
	__asm__ __volatile__("stha %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ void outl(unsigned long port, unsigned int val)
{
	__asm__ __volatile__("sta %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ unsigned int inb(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lduba [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned int inw(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lduha [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
	return ret;
}

static __inline__ unsigned int inl(unsigned long port)
{
	unsigned int ret;
	__asm__ __volatile__("lda [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
	return ret;
}

#endif	/* !Lynx */

#include <string.h>

static __inline__ unsigned long ldq_u(unsigned long *p)
{
	unsigned long ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
}

static __inline__ unsigned long ldl_u(unsigned int *p)
{
	unsigned int ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
}

static __inline__ unsigned long ldw_u(unsigned short *p)
{
	unsigned short ret;
	memmove(&ret, p, sizeof(*p));
	return ret;
}

static __inline__ void stq_u(unsigned long val, unsigned long *p)
{
	unsigned long tmp = val;
	memmove(p, &tmp, sizeof(*p));
}

static __inline__ void stl_u(unsigned long val, unsigned int *p)
{
	unsigned int tmp = val;
	memmove(p, &tmp, sizeof(*p));
}

static __inline__ void stw_u(unsigned long val, unsigned short *p)
{
	unsigned short tmp = val;
	memmove(p, &tmp, sizeof(*p));
}

#define mem_barrier()         /* XXX: nop for now */
#define write_mem_barrier()   /* XXX: nop for now */

#elif defined(__mips__) || defined(__arm32__)

unsigned int IOPortBase;  /* Memory mapped I/O port area */

static __inline__ void
outb(unsigned short port, unsigned char val)
{
	*(volatile unsigned char*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
	*(volatile unsigned short*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
	*(volatile unsigned long*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ unsigned int
inb(unsigned short port)
{
	return(*(volatile unsigned char*)(((unsigned short)(port))+IOPortBase));
}

static __inline__ unsigned int
inw(unsigned short port)
{
	return(*(volatile unsigned short*)(((unsigned short)(port))+IOPortBase));
}

static __inline__ unsigned int
inl(unsigned short port)
{
	return(*(volatile unsigned long*)(((unsigned short)(port))+IOPortBase));
}


#if defined(__mips__)
static __inline__ unsigned long ldq_u(unsigned long * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(3+(char *) r11)));
	return r1;
}

static __inline__ unsigned long ldl_u(unsigned int * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(3+(char *) r11)));
	return r1;
}

static __inline__ unsigned long ldw_u(unsigned short * r11)
{
	unsigned long r1;
	__asm__("lwr %0,%2\n\t"
		"lwl %0,%3\n\t"
		:"=&r" (r1)
		:"r" (r11),
		 "m" (*r11),
		 "m" (*(unsigned long *)(1+(char *) r11)));
	return r1;
}

#define stq_u(v,p)	stl_u(v,p)
#define stl_u(v,p)	(*(unsigned char *)(p)) = (v); \
			(*(unsigned char *)(p)+1) = ((v) >> 8);  \
			(*(unsigned char *)(p)+2) = ((v) >> 16); \
			(*(unsigned char *)(p)+3) = ((v) >> 24)

#define stw_u(v,p)	(*(unsigned char *)(p)) = (v); \
			(*(unsigned char *)(p)+1) = ((v) >> 8)

#define mem_barrier()   /* NOP */
#endif /* __mips__ */

#if defined(__arm32__)
#define ldq_u(p)	(*((unsigned long  *)(p)))
#define ldl_u(p)	(*((unsigned int   *)(p)))
#define ldw_u(p)	(*((unsigned short *)(p)))
#define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#define mem_barrier()	/* NOP */
#define write_mem_barrier()	/* NOP */
#endif /* __arm32__ */

#elif (defined(Lynx) || defined(linux)) && defined(__powerpc__)

extern volatile unsigned char *ioBase;

#define eieio()		__asm__ __volatile__ ("eieio")

static __inline__ void
outb(unsigned short port, unsigned char value)
{
	*((volatile unsigned char *)(ioBase + port)) = value; eieio();
}

static __inline__ void
outw(unsigned short port, unsigned short value)
{
	__asm__ __volatile__ (
		"sthbrx %0,%1,%2\n\t"
		"eieio"
                  : 
		  : "r" (value), "b" (ioBase), "r" (port) 
		  : "memory"
        );
}

static __inline__ void
outl(unsigned short port, unsigned int value)
{
	__asm__ __volatile__ (
		"stwbrx %0,%1,%2\n\t"
		"eieio"
                  : 
		  : "r" (value), "b" (ioBase), "r" (port) 
		  : "memory"
        );
}

static __inline__ unsigned int
inb(unsigned short port)
{
	unsigned char val;

	val = *((volatile unsigned char *)(ioBase + port));
	eieio();
	return(val);
}

static __inline__ unsigned int
inw(unsigned short port)
{
	register unsigned short val;

        __asm__ ("lhbrx %0,%1,%2\n\t"
        	 "eieio"
                  : "=r" (val) 
                  : "b" (ioBase), "r" (port)
        );
	return(val);
}

static __inline__ unsigned int
inl(unsigned short port)
{
	register unsigned long val;

        __asm__ ("lwbrx %0,%1,%2\n\t"
        	 "eieio"
                  : "=r" (val) 
                  : "b" (ioBase), "r" (port)
        );
	return(val);
}

#define ldq_u(p)	ldl_u(p)
#define ldl_u(p)	((*(unsigned char *)(p))	| \
			(*((unsigned char *)(p)+1)<<8)	| \
			(*((unsigned char *)(p)+2)<<16)	| \
			(*((unsigned char *)(p)+3)<<24))
#define ldw_u(p)	((*(unsigned char *)(p)) | \
			(*((unsigned char *)(p)+1)<<8))

#define stq_u(v,p)	stl_u(v,p)
#define stl_u(v,p)	(*(unsigned char *)(p)) = (v); \
			(*((unsigned char *)(p)+1)) = ((v) >> 8);  \
			(*((unsigned char *)(p)+2)) = ((v) >> 16); \
			(*((unsigned char *)(p)+3)) = ((v) >> 24)
#define stw_u(v,p)	(*(unsigned char *)(p)) = (v); \
			(*((unsigned char *)(p)+1)) = ((v) >> 8)

#define mem_barrier()		eieio()
#define write_mem_barrier()	eieio()

#else /* ix86 */

#define ldq_u(p)	(*((unsigned long  *)(p)))
#define ldl_u(p)	(*((unsigned int   *)(p)))
#define ldw_u(p)	(*((unsigned short *)(p)))
#define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#define mem_barrier()   /* NOP */
#define write_mem_barrier()   /* NOP */

#if !defined(FAKEIT) && !defined(__mc68000__)
#ifdef GCCUSESGAS

/*
 * If gcc uses gas rather than the native assembler, the syntax of these
 * inlines has to be different.		DHD
 */
#ifndef PC98

static __inline__ void
outb(unsigned short port, unsigned char val)
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}


static __inline__ void
outw(unsigned short port, unsigned short val)
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(unsigned short port)
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
   unsigned short ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#else /* PC98 */

static __inline__ void
_outb(unsigned short port, unsigned char val)
{
     __asm__ __volatile__("outb %0,%1" ::"a" (val), "d" (port));
}

static __inline__ void
_outw(unsigned short port, unsigned short val)
{
     __asm__ __volatile__("outw %0,%1" ::"a" (val), "d" (port));
}
 
static __inline__ void
_outl(unsigned short port, unsigned int val)
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}


static __inline__ unsigned int
_inb(unsigned short port)
{
     unsigned char ret;
     __asm__ __volatile__("inb %1,%0" :
                          "=a" (ret) :
                          "d" (port));
     return ret;
}

static __inline__ unsigned int
_inw(unsigned short port)
{
     unsigned char ret;
     __asm__ __volatile__("inw %1,%0" :
                          "=a" (ret) :
                          "d" (port));
     return ret;
}

static __inline__ unsigned int
_inl(unsigned short port)
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}


#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB) || defined(PC98_GA968)
#define PW_PORT 0x600
extern short chipID;
extern void *mmioBase;
extern unsigned short _port_tbl[];
#define	port_convert(x)	_port_tbl[(unsigned short)x]
#endif 

#if defined(PC98_WAB) ||  defined(PC98_GANB_WAP)
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE0;
     return port;
}
#endif /* PC98_WAB || PC98_GANB_WAP */
 
#if defined(PC98_WABEP)
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port &= 0x7f; /* Mask 0000 0000 0111 1111 */
     port |= 0x0f00;
     return port;
}
#endif /* PC98_WABEP */

#ifdef PC98_WSNA
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE2;
     return port;
}
#endif /* PC98_WSNA */

#ifdef PC98_NKVNEC
#ifdef	PC98_NEC_CIRRUS2
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x0050;
     return port;
}
#else
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x00a0;
     return port;
}
#endif /* PC98_NEC_CIRRUS2 */
#endif /* PC98_NKVNEC */

#if defined(PC98_TGUI)
extern void *mmioBase;
#endif

static __inline__ void
outb(unsigned short port, unsigned char val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned char *)((char *)mmioBase+(port)) = (unsigned char)(val);
#else
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned short *)((char *)mmioBase+(port)) = (unsigned short)(val);
#else
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned int *)((char *)mmioBase+(port)) = (unsigned int)(val);
#else
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ unsigned int
inb(unsigned short port)
{
   unsigned char ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned char *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
   unsigned short ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned short *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
   unsigned int ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned int *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

#endif /* PC98 */

#else	/* GCCUSESGAS */

static __inline__ void
outb(unsigned short port, unsigned char val)
{
  __asm__ __volatile__("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
  __asm__ __volatile__("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
  __asm__ __volatile__("out%L0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(unsigned short port)
{
  unsigned char ret;
  __asm__ __volatile__("in%B0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
inw(unsigned short port)
{
  unsigned short ret;
  __asm__ __volatile__("in%W0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
inl(unsigned short port)
{
  unsigned int ret;
  __asm__ __volatile__("in%L0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

#endif /* GCCUSESGAS */

#else /* !defined(FAKEIT) && !defined(__mc68000__) */

static __inline__ void
outb(unsigned short port, unsigned char val)
{
}

static __inline__ void
outw(unsigned short port, unsigned short val)
{
}

static __inline__ void
outl(unsigned short port, unsigned int val)
{
}

static __inline__ unsigned int
inb(unsigned short port)
{
  return 0;
}

static __inline__ unsigned int
inw(unsigned short port)
{
  return 0;
}

static __inline__ unsigned int
inl(unsigned short port)
{
  return 0;
}

#endif /* FAKEIT */

#endif /* ix86 */

#elif defined(__powerpc__) /* && !__GNUC__ */
/*
 * NON-GCC PowerPC - Presumed to be PowerMAX OS for now
 */
# ifndef PowerMAX_OS
# error - Non-gcc PowerPC and !PowerMAXOS ???
# endif

#define PPCIO_DEBUG  0
#define PPCIO_INLINE 1
#define USE_ABS_MACRO 1
/*
 * Use compiler intrinsics to access certain PPC machine instructions
 */
#define eieio() 	      __inst_eieio()
#define stw_brx(val,base,ndx) __inst_sthbrx(val,base,ndx)
#define stl_brx(val,base,ndx) __inst_stwbrx(val,base,ndx)
#define ldw_brx(base,ndx)     __inst_lhbrx(base,ndx)
#define ldl_brx(base,ndx)     __inst_lwbrx(base,ndx)

#define ldq_u(p)	(*((unsigned long long  *)(p)))
#define ldl_u(p)	(*((unsigned long   *)(p)))
#define ldw_u(p)	(*((unsigned short *)(p)))
#define stq_u(v,p)	(*(unsigned long long *)(p)) = (v)
#define stl_u(v,p)	(*(unsigned long  *)(p)) = (v)
#define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#define mem_barrier()         eieio()
#define write_mem_barrier()   eieio()

extern volatile unsigned char *ioBase;

#if !defined(abs) && defined(USE_ABS_MACRO)
#define abs(x) ((x) >= 0 ? (x) : -(x))
#endif

#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl

#if PPCIO_DEBUG

extern void debug_outb(unsigned int a, unsigned char b, int line, char *file); 
extern void debug_outw(unsigned int a, unsigned short w, int line, char *file); 
extern void debug_outl(unsigned int a, unsigned long l, int line, char *file); 
extern unsigned char debug_inb(unsigned int a, int line, char *file); 
extern unsigned short debug_inw(unsigned int a, int line, char *file); 
extern unsigned long debug_inl(unsigned int a, int line, char *file); 

#define outb(a,b) debug_outb(a,b, __LINE__, __FILE__)
#define outw(a,w) debug_outw(a,w, __LINE__, __FILE__)
#define outl(a,l) debug_outl(a,l, __LINE__, __FILE__)
#define inb(a)    debug_inb(a, __LINE__, __FILE__)
#define inw(a)    debug_inw(a, __LINE__, __FILE__)
#define inl(a)    debug_inl(a, __LINE__, __FILE__)

#else /* !PPCIO_DEBUG */

extern unsigned char  inb(unsigned int a);
extern unsigned short inw(unsigned int a);
extern unsigned long  inl(unsigned int a);

# if PPCIO_INLINE

#define outb(a,b) (*((volatile unsigned char *)(ioBase + (a))) = (b), eieio())
#define outw(a,w) (stw_brx((w),ioBase,(a)), eieio())
#define outl(a,l) (stl_brx((l),ioBase,(a)), eieio())

# else /* !PPCIO_INLINE */

extern void outb(unsigned int a, unsigned char b);
extern void outw(unsigned int a, unsigned char w);
extern void outl(unsigned int a, unsigned char l);

# endif /* PPCIO_INLINE */

#endif /* !PPCIO_DEBUG */

#else /* !GNUC && !PPC */
#if !defined(AMOEBA) && !defined(MINIX) && !defined(QNX4)
# if defined(__STDC__) && (__STDC__ == 1)
#  ifndef asm
#   define asm __asm
#  endif
# endif
# ifdef SVR4
#if 0
#  include <sys/types.h>
#endif
#  ifndef __HIGHC__
#   ifndef __USLC__
#    define __USLC__
#   endif
#  endif
# endif
# ifndef PC98
#  ifndef SCO325
#   if defined(USL)
#    if defined(IN_MODULE)
#     /* avoid including <sys/types.h> for <sys/inline.h> on UnixWare */
#     define ushort unsigned short
#     define ushort_t unsigned short
#     define ulong unsigned long
#     define ulong_t unsigned long
#     define uint_t unsigned int
#     define uchar_t unsigned char
#    else
#     include <sys/types.h>
#    endif /* IN_MODULE */
#   endif /* USL */
#   include <sys/inline.h>
#  else
#   include "scoasm.h"
#  endif
# else
#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB) || defined(PC98_GA968)
#define PW_PORT 0x600
extern short chipID;
extern void *mmioBase;
extern unsigned short _port_tbl[];
#define	port_convert(x)	_port_tbl[(unsigned short)x]
#endif 

asm     void _outl(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	outl	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movl    val, %eax
	outl	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	outl	(%dx)
%mem	port,val;
	movw	port, %dx
	movl    val, %eax
	outl	(%dx)
}

asm	void _outw(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	data16
	outl	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movw	val, %ax
	data16
	outl	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	data16
	outl	(%dx)
%mem	port,val;
	movw	port, %dx
	movw	val, %ax
	data16
	outl	(%dx)
}

asm	void _outb(port,val)
{
%reg	port,val;
	movl	port, %edx
	movl	val, %eax
	outb	(%dx)
%reg	port; mem	val;
	movl	port, %edx
	movb	val, %al
	outb	(%dx)
%mem	port; reg	val;
	movw	port, %dx
	movl	val, %eax
	outb	(%dx)
%mem	port,val;
	movw	port, %dx
	movb	val, %al
	outb	(%dx)
}

asm     int _inl(port)
{
%reg	port;
	movl	port, %edx
	inl	(%dx)
%mem	port;
	movw	port, %dx
	inl	(%dx)
}

asm	int _inw(port)
{
%reg	port;
	subl    %eax, %eax
	movl	port, %edx
	data16
	inl	(%dx)
%mem	port;
	subl    %eax, %eax
	movw	port, %dx
	data16
	inl	(%dx)
}

asm	int _inb(port)
{
%reg	port;
	subl    %eax, %eax
	movl	port, %edx
	inb	(%dx)
%mem	port;
	subl    %eax, %eax
	movw	port, %dx
	inb	(%dx)
}

#if defined(PC98_WAB) ||  defined(PC98_GANB_WAP)
static unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE0;
     return port;
}
#endif /* PC98_WAB || PC98_GANB_WAP */

#if defined(PC98_WABEP)
static unsigned short
port_convert(unsigned short port)
{
     port &= 0x7f; /* Mask 0000 0000 0111 1111 */
     port |= 0x0f00;
     return port;
}
#endif /* PC98_WABEP */

#ifdef PC98_WSNA
static unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE2;
     return port;
}
#endif /* PC98_WSNA */

#ifdef PC98_NKVNEC
#ifdef	PC98_NEC_CIRRUS2
static unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x0050;
     return port;
}
#else
static unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x00a0;
     return port;
}
#endif /* PC98_NEC_CIRRUS2 */
#endif /* PC98_NKVNEC */

static void outl(port,val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   *(volatile unsigned int *)((char *)mmioBase+(port)) = (unsigned int)(val);
#else
   _outl(port,val);
#endif
}

static void outw(port,val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   *(volatile unsigned short *)((char *)mmioBase+(port)) = (unsigned short)(val);
#else
   _outw(port,val);
#endif
}

static void outb(port,val)
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   *(volatile unsigned char *)((char *)mmioBase+(port)) = (unsigned char)(val);
#else
   _outb(port,val);
#endif
}

static int inl(port)
{
   unsigned int ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   ret =*(volatile unsigned int *)((char *)mmioBase+(port));
#else
   ret = _inl(port);
#endif
   return ret;
}

static int inw(port)
{
   unsigned short ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   ret =*(volatile unsigned short *)((char *)mmioBase+(port));
#else
   ret = _inw(port);
#endif
   return ret;
}

static int inb(port)
{
   unsigned char ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)
   ret =*(volatile unsigned char *)((char *)mmioBase+(port));
#else
   ret = _inb(port);
#endif
   return ret;
}


# endif /* PC98 */
# if !defined(__HIGHC__) && !defined(SCO325)
#  pragma asm partial_optimization outl
#  pragma asm partial_optimization outw
#  pragma asm partial_optimization outb
#  pragma asm partial_optimization inl
#  pragma asm partial_optimization inw
#  pragma asm partial_optimization inb
# endif
#endif
#define ldq_u(p)	(*((unsigned long  *)(p)))
#define ldl_u(p)	(*((unsigned int   *)(p)))
#define ldw_u(p)	(*((unsigned short *)(p)))
#define stq_u(v,p)	(*(unsigned long  *)(p)) = (v)
#define stl_u(v,p)	(*(unsigned int   *)(p)) = (v)
#define stw_u(v,p)	(*(unsigned short *)(p)) = (v)
#define mem_barrier()   /* NOP */
#define write_mem_barrier()   /* NOP */
#endif /* __GNUC__ */

#if defined(QNX4)
#include <sys/types.h>
extern unsigned  inb(unsigned port);
extern unsigned  inw(unsigned port);
extern unsigned  inl(unsigned port);
extern void outb(unsigned port, unsigned val);
extern void outw(unsigned port, unsigned val);
extern void outl(unsigned port, unsigned val);
#define ldq_u(p)        (*((unsigned long  *)(p)))
#define ldl_u(p)        (*((unsigned int   *)(p)))
#define ldw_u(p)        (*((unsigned short *)(p)))
#undef stq_u
#define stq_u(v,p)      ((unsigned long  *)(p)) = (v)
#undef stl_u
#define stl_u(v,p)      ((unsigned int   *)(p)) = (v)
#undef stw_u
#define stw_u(v,p)      ((unsigned short *)(p)) = (v)
#define mem_barrier()   /* NOP */
#define write_mem_barrier()   /* NOP */
#endif /* QNX4 */

#if defined(IODEBUG) && defined(__GNUC__)
#undef inb
#undef inw
#undef inl
#undef outb
#undef outw
#undef outl
#define inb(a) __extension__ ({unsigned char __c=RealInb(a); ErrorF("inb(0x%03x) = 0x%02x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})
#define inw(a) __extension__ ({unsigned short __c=RealInw(a); ErrorF("inw(0x%03x) = 0x%04x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})
#define inl(a) __extension__ ({unsigned long __c=RealInl(a); ErrorF("inl(0x%03x) = 0x%08x\t@ line %4d, file %s\n", a, __c, __LINE__, __FILE__);__c;})

#define outb(a,b) (ErrorF("outb(0x%03x, 0x%02x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutb(a,b))
#define outw(a,b) (ErrorF("outw(0x%03x, 0x%04x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutw(a,b))
#define outl(a,b) (ErrorF("outl(0x%03x, 0x%08x)\t@ line %4d, file %s\n", a, b, __LINE__, __FILE__),RealOutl(a,b))
#endif

/*
 * This header sometimes gets included where is isn't needed, and on some
 * OSs this causes problems because the following functions generate
 * references to inb() and outb() which can't be resolved.  Defining
 * NO_COMPILER_H_EXTRAS avoids this problem.
 */

#ifndef NO_COMPILER_H_EXTRAS
/*
 *-----------------------------------------------------------------------
 * Port manipulation convenience functions
 *-----------------------------------------------------------------------
 */

#ifndef __GNUC__
#ifdef __HIGHC__
#define __inline__ _Inline
#else
#define __inline__ /**/
#endif
#endif

/*
 * rdinx - read the indexed byte port 'port', index 'ind', and return its value
 */
static __inline__ unsigned char 
rdinx(unsigned short port, unsigned char ind)
{
	if (port == 0x3C0)		/* reset attribute flip-flop */
		(void) inb(0x3DA);
	outb(port, ind);
	return(inb(port+1));
}

/*
 * wrinx - write 'val' to port 'port', index 'ind'
 */
static __inline__ void 
wrinx(unsigned short port, unsigned char ind, unsigned char val)
{
	outb(port, ind);
	outb(port+1, val);
}

/*
 * modinx - in register 'port', index 'ind', set the bits in 'mask' as in 'new';
 *	    the other bits are unchanged.
 */
static __inline__ void
modinx(unsigned short port, unsigned char ind, 
       unsigned char mask, unsigned char new)
{
	unsigned char tmp;

	tmp = (rdinx(port, ind) & ~mask) | (new & mask);
	wrinx(port, ind, tmp);
}

/*
 * tstrg - returns true iff the bits in 'mask' of register 'port' are
 *	   readable & writable.
 */

static __inline__ int
testrg(unsigned short port, unsigned char mask)
{
	unsigned char old, new1, new2;

	old = inb(port);
	outb(port, old & ~mask);
	new1 = inb(port) & mask;
	outb(port, old | mask);
	new2 = inb(port) & mask;
	outb(port, old);
	return((new1 == 0) && (new2 == mask));
}

/*
 * testinx2 - returns true iff the bits in 'mask' of register 'port', index
 *	      'ind' are readable & writable.
 */
static __inline__ int
testinx2(unsigned short port, unsigned char ind, unsigned char mask)
{
	unsigned char old, new1, new2;

	old = rdinx(port, ind);
	wrinx(port, ind, old & ~mask);
	new1 = rdinx(port, ind) & mask;
	wrinx(port, ind, old | mask);
	new2 = rdinx(port, ind) & mask;
	wrinx(port, ind, old);
	return((new1 == 0) && (new2 == mask));
}

/*
 * testinx - returns true iff all bits of register 'port', index 'ind' are 
 *     	     readable & writable.
 */
static __inline__ int
testinx(unsigned short port, unsigned char ind)
{
	return(testinx2(port, ind, 0xFF));
}
#endif /* NO_COMPILER_H_EXTRAS */

#endif /* NO_INLINE */

/* Some macros to hide the system dependencies for MMIO accesses */
#ifdef __alpha__
extern void (*xf86WriteMmio8)(int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmio16)(int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmio32)(int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmioNB8)(int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmioNB16)(int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmioNB32)(int Value, void *Base, unsigned long Offset);
extern int  (*xf86ReadMmio8)(void *Base, unsigned long Offset);
extern int  (*xf86ReadMmio16)(void *Base, unsigned long Offset);
extern int  (*xf86ReadMmio32)(void *Base, unsigned long Offset);

#define MMIO_IN8(base, offset) (*xf86ReadMmio8)(base, offset)
#define MMIO_IN16(base, offset) (*xf86ReadMmio16)(base, offset)
# if defined (JENSEN_SUPPORT)
#define MMIO_IN32(base, offset) (*xf86ReadMmio32)(base, offset)
#define MMIO_OUT32(base, offset, val) (*xf86WriteMmio32)(val, base, offset)
#define MMIO_ONB32(base, offset, val) (*xf86WriteMmioNB32)(val, base, offset)
# else
#define MMIO_IN32(base, offset) \
                   *(volatile CARD32 *)(((CARD8*)(base)) + (offset))
#define MMIO_OUT32(base, offset, val) do { \
                   *(volatile CARD32 *)(((CARD8*)(base)) + (offset)) = (val);\
                   write_mem_barrier();\
                  } while (0)
#define MMIO_ONB32(base, offset, val) \
                   *(volatile CARD32 *)(((CARD8*)(base)) + (offset)) = (val)
# endif
#define MMIO_OUT8(base, offset, val) (*xf86WriteMmio8)(val, base, offset)
#define MMIO_OUT16(base, offset, val) (*xf86WriteMmio16)(val, base, offset)
#define MMIO_ONB8(base, offset, val) (*xf86WriteMmioNB8)(val, base, offset)
#define MMIO_ONB16(base, offset, val) (*xf86WriteMmioNB16)(val, base, offset)
#else /* !__alpha__ */
#define MMIO_IN8(base, offset) *(volatile CARD8 *)(((CARD8*)(base)) + (offset))
#define MMIO_IN16(base, offset) *(volatile CARD16 *)(((CARD8*)(base)) + (offset))
#define MMIO_IN32(base, offset) *(volatile CARD32 *)(((CARD8*)(base)) + (offset))
#define MMIO_OUT8(base, offset, val) *(volatile CARD8 *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT16(base, offset, val) *(volatile CARD16 *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT32(base, offset, val) *(volatile CARD32 *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_ONB8(base, offset, val) MMIO_OUT8(base, offset, val) 
#define MMIO_ONB16(base, offset, val) MMIO_OUT16(base, offset, val) 
#define MMIO_ONB32(base, offset, val) MMIO_OUT32(base, offset, val) 
#endif /* __alpha__ */

/*
 * With Intel, the version in os-support/misc/SlowBcopy.s is used.
 * This avoids port I/O during the copy (which causes problems with
 * some hardware).
 */
#ifdef __alpha__
#define slowbcopy_tobus(src,dst,count) xf86SlowBCopyToBus(src,dst,count)
#define slowbcopy_frombus(src,dst,count) xf86SlowBCopyFromBus(src,dst,count)
#else /* __alpha__ */
#define slowbcopy_tobus(src,dst,count) xf86SlowBcopy(src,dst,count)
#define slowbcopy_frombus(src,dst,count) xf86SlowBcopy(src,dst,count)
#endif /* __alpha__ */

#endif /* _COMPILER_H */
