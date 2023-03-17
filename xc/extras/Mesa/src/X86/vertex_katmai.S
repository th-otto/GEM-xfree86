#include "assyntax.h"

#define MAT_SY 20
#define MAT_SZ 40
#define MAT_TX 48
#define MAT_TY 52
#define MAT_TZ 56
	
	
	SEG_TEXT

#if !defined(NASM_ASSEMBLER) && !defined(MASM_ASSEMBLER)
#define LLBL(a) .L##a
#else
#define LLBL(a) a
#endif


/*
 * void gl_v16_katmai_general_xform (GLfloat *dest,
 *		        	     const GLfloat *m,
 *				     const GLfloat *src,
 *                                   GLuint src_stride,
 *				     GLuint count )
 *
 */

ALIGNTEXT16
GLOBL GLNAME( gl_v16_katmai_general_xform )
GLNAME( gl_v16_katmai_general_xform ):

	PUSH_L( EDI )
 	PUSH_L( ESI )

 	MOV_L( REGOFF(12, ESP), EAX )	/* f - destination */
 	MOV_L( REGOFF(16, ESP), ESI )	/* m - matrix */
 	MOV_L( REGOFF(20, ESP), EDX )	/* obj - source */
 	MOV_L( REGOFF(24, ESP), EDI )	/* obj_stride */
 	MOV_L( REGOFF(28, ESP), ECX )	/* count */

 	MOVUPS( REGOFF(0, ESI), XMM4 )	/* x14, x13, x12, x11 => xmm4 */
 	MOVUPS( REGOFF(16, ESI), XMM5 )	/* x24, x23, x22, x21 => xmm5 */
 	MOVUPS( REGOFF(32, ESI), XMM6 )	/* x34, x33, x32, x31 => xmm6 */
 	MOVUPS( REGOFF(48, ESI), XMM7 )	/* x44, x43, x42, x41 => xmm7 */

ALIGNTEXT32
LLBL( loop1 ):	

 	MOVSS( REGOFF(0, EDX), XMM0 )	/* s1 => xmm0 */
 	SHUFPS( CONST(0x0), XMM0, XMM0 )	/* xmm0 shuffle to other 96 bits */
 	MULPS( XMM4, XMM0 )		/* x14*s1, x13*s1, x12*s1, x11*s1 */

 	MOVSS( REGOFF(4, EDX), XMM1 )	/* s2 => xmm1 */
 	SHUFPS( CONST(0x0), XMM1, XMM1 )	/* xmm1 shuffle to other 96 bits */
 	MULPS( XMM5, XMM1 )		/* x24*s2, x23*s2, x22*s2, x21*s2 */

 	MOVSS( REGOFF(8, EDX), XMM2 )	/* s3 => xmm2 */
 	SHUFPS( CONST(0x0), XMM2, XMM2 )	/* xmm2 shuffle to other 96 bits */
 	MULPS( XMM6, XMM2 )		/* x34*s3, x33*s3, x32*s3, x31*s3 */

 	ADDPS( XMM1, XMM0 )		/* xmm0 + xmm1 => xmm0 */
 	ADDPS( XMM2, XMM0 )		/* xmm0 + xmm2 => xmm0 */
 	ADDPS( XMM7, XMM0 )		/* xmm0 + xmm7 => xmm0 */

 	MOVUPS( XMM0, REGOFF(0, EAX) ) 	/* xmm0 => dest( f11, f12, f13, f14 ) */

 	ADD_L   ( CONST(64), EAX )	/* f + 64 bytes => f */
        ADD_L   ( EDI, EDX )		/* obj + obj_stride => obj */

        DEC_L   ( ECX )			/* count - 1 => count */
        JNE     ( LLBL(loop1) )		/* if not zero, go back to LABL(loop1) */

 	POP_L( ESI )
 	POP_L( EDI )

 	RET


/* void gl_katmai_project_vertices(GLfloat *first,
 *	              		   GLfloat *last,
 *	              		   const GLfloat *m,
 *	              		   GLuint stride )
 */


ALIGNTEXT16
GLOBL GLNAME( gl_katmai_project_vertices )
GLNAME( gl_katmai_project_vertices ):
	
	PUSH_L( EBP )

	MOV_L( REGOFF(8, ESP), ECX )	/* first_vert */
	MOV_L( REGOFF(12, ESP), EDX )	/* last_vert */
	MOV_L( REGOFF(16, ESP), EBP )	/* matrix */
	MOV_L( REGOFF(20, ESP), EAX )	/* stride */

	
ALIGNTEXT32
	MOVUPS( REGOFF(MAT_TX, EBP), XMM0 )	/* (x44), x43, x42, x41 => xmm0 */
	MOVSS( REGOFF(0, EBP), XMM1 )		/* -, -, -, x11 => xmm1 */
	UNPCKLPS( REGOFF(MAT_SY, EBP), XMM1 )	/* -, -, x22, x11 => xmm1 */
	SHUFPS( CONST(0x44), REGOFF(MAT_SZ, EBP), XMM1 )
						/* -, x33, x22, x11 => xmm1 */

	SUB_L( ECX, EDX )			/* last -= first */

LLBL( v16_katmai_pv_loop_start ):
	MOVUPS( REGOFF(0, ECX), XMM3 )		/* f[3], f[2], f[1], f[0] */

	MOVSS( REGOFF(12, ECX), XMM2 )		/* -, -, -, f[3] */
	SHUFPS( CONST(0x0), XMM2, XMM2 )	/* f[3], f[3], f[3], f[3] */
	RCPPS( XMM2, XMM2 )			/* 1/f[3], 1/f[3], 1/f[3], 1/f[3] */

	MULPS( XMM2, XMM1 )			/* -, x33*1/f[3]... */
	MULPS( XMM3, XMM1 )			/* -, x33*1/f[3]*f[2]... */
	ADDPS( XMM0, XMM1 )			/* -, x33*1/f[3]*f[2]+x43... */

	MOVUPS( XMM1, REGOFF(0, ECX) )		/* back to f */
	MOVSS( XMM2, REGOFF(12, ECX) )		/* 1/f[3] into f[3] ! */


	ADD_L( EAX, ECX )			/* f += stride */
	SUB_L( EAX, EDX )			/* (last-first)-stride */
	JA ( LLBL( v16_katmai_pv_loop_start ) )

	POP_L( EBP )
	RET



/* void gl_katmai_project_clipped_vertices(GLfloat *first,
 *	              		   	   GLfloat *last,
 *	              		   	   const GLfloat *m,
 *	              		   	   GLuint stride,
 *					   const GLubyte *clipmask );
 */


ALIGNTEXT16
GLOBL GLNAME( gl_katmai_project_clipped_vertices )
GLNAME( gl_katmai_project_clipped_vertices ):
	
	PUSH_L( EBP )
	PUSH_L( ESI )

	MOV_L( REGOFF(12, ESP), ECX )	/* first_vert */
	MOV_L( REGOFF(16, ESP), EDX )	/* last_vert */
	MOV_L( REGOFF(20, ESP), EBP )	/* matrix */
	MOV_L( REGOFF(24, ESP), EAX )	/* stride */
	MOV_L( REGOFF(28, ESP), ESI )	/* clip_mask */
	

	
ALIGNTEXT32

	MOVUPS( REGOFF(MAT_TX, EBP), XMM0 )	/* (x44), x43, x42, x41 => xmm0 */
	MOVSS( REGOFF(0, EBP), XMM1 )		/* -, -, -, x11 => xmm1 */
	UNPCKLPS( REGOFF(MAT_SY, EBP), XMM1 )	/* -, -, x22, x11 => xmm1 */
	SHUFPS( CONST(0x44), REGOFF(MAT_SZ, EBP), XMM1 )
						/* -, x33, x22, x11 => xmm1 */

LLBL( v16_katmai_pcv_loop_start ):
	CMP_B ( CONST(0), REGIND(ESI) )		/* clip_mask == 0 ? */
	JNE( LLBL( v16_katmai_pcv_skip ) )	/* no -> skip ! */

	MOVUPS( REGOFF(0, ECX), XMM3 )		/* f[3], f[2], f[1], f[0] */

	MOVSS( REGOFF(12, ECX), XMM2 )		/* -, -, -, f[3] */
	SHUFPS( CONST(0x0), XMM2, XMM2 )	/* f[3], f[3], f[3], f[3] */
	RCPPS( XMM2, XMM2 )			/* 1/f[3], 1/f[3], 1/f[3], 1/f[3] */

	MULPS( XMM2, XMM1 )			/* -, x33*1/f[3]... */
	MULPS( XMM3, XMM1 )			/* -, x33*1/f[3]*f[2]... */
	ADDPS( XMM0, XMM1 )			/* -, x33*1/f[3]*f[2]+x43... */

	MOVUPS( XMM1, REGOFF(0, ECX) )		/* back to f */
	MOVSS( XMM2, REGOFF(12, ECX) )		/* 1/f[3] into f[3] ! */

LLBL( v16_katmai_pcv_skip ):
	ADD_L( EAX, ECX )			/* f += stride */
	INC_L( ESI )				/* nect ClipMask */

	CMP_L( ECX, EDX )			/* p_first_vertex == p_last_vertex */
	JNE( LLBL( v16_katmai_pcv_loop_start ) )
						/* no -> go on with next vertex */

	POP_L( ESI )	
	POP_L( EBP )
	RET



