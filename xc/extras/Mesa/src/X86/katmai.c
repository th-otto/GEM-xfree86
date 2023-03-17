/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * PentiumIII-SIMD (SSE) optimizations contributed by
 * Andre Werthmann <wertmann@cs.uni-potsdam.de>
 */

#if defined(USE_KATMAI_ASM)
#include "katmai.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "context.h"
#include "types.h"
#include "xform.h"
#include "vertices.h"

#ifdef DEBUG
#include "debug_xform.h"
#endif




#define XFORM_ARGS      GLvector4f *to_vec,             \
                        const GLmatrix *mat,            \
                        const GLvector4f *from_vec,     \
                        const GLubyte *mask,            \
                        const GLubyte flag



#define DECLARE_XFORM_GROUP(pfx, v, masked) \
 extern void gl_##pfx##_transform_points##v##_general_##masked(XFORM_ARGS);    \
 extern void gl_##pfx##_transform_points##v##_identity_##masked(XFORM_ARGS);   \
 extern void gl_##pfx##_transform_points##v##_3d_no_rot_##masked(XFORM_ARGS);  \
 extern void gl_##pfx##_transform_points##v##_perspective_##masked(XFORM_ARGS);\
 extern void gl_##pfx##_transform_points##v##_2d_##masked(XFORM_ARGS);         \
 extern void gl_##pfx##_transform_points##v##_2d_no_rot_##masked(XFORM_ARGS);  \
 extern void gl_##pfx##_transform_points##v##_3d_##masked(XFORM_ARGS);



#define ASSIGN_XFORM_GROUP( pfx, cma, vsize, masked )           \
 gl_transform_tab[cma][vsize][MATRIX_GENERAL]                   \
  = gl_##pfx##_transform_points##vsize##_general_##masked;      \
 gl_transform_tab[cma][vsize][MATRIX_IDENTITY]                  \
  = gl_##pfx##_transform_points##vsize##_identity_##masked;     \
 gl_transform_tab[cma][vsize][MATRIX_3D_NO_ROT]                 \
  = gl_##pfx##_transform_points##vsize##_3d_no_rot_##masked;    \
 gl_transform_tab[cma][vsize][MATRIX_PERSPECTIVE]               \
  = gl_##pfx##_transform_points##vsize##_perspective_##masked;  \
 gl_transform_tab[cma][vsize][MATRIX_2D]                        \
  = gl_##pfx##_transform_points##vsize##_2d_##masked;           \
 gl_transform_tab[cma][vsize][MATRIX_2D_NO_ROT]                 \
  = gl_##pfx##_transform_points##vsize##_2d_no_rot_##masked;    \
 gl_transform_tab[cma][vsize][MATRIX_3D]                        \
  = gl_##pfx##_transform_points##vsize##_3d_##masked;




#define NORM_ARGS       const GLmatrix *mat,        \
                        GLfloat scale,              \
                        const GLvector3f *in,       \
                        const GLfloat *lengths,     \
                        const GLubyte mask[],       \
                        GLvector3f *dest 



#define DECLARE_NORM_GROUP(pfx, masked)                                        \
 extern void gl_##pfx##_rescale_normals_##masked## (NORM_ARGS);                \
 extern void gl_##pfx##_normalize_normals_##masked## (NORM_ARGS);              \
 extern void gl_##pfx##_transform_normals_##masked## (NORM_ARGS);              \
 extern void gl_##pfx##_transform_normals_no_rot_##masked## (NORM_ARGS);       \
 extern void gl_##pfx##_transform_rescale_normals_##masked## (NORM_ARGS);      \
 extern void gl_##pfx##_transform_rescale_normals_no_rot_##masked## (NORM_ARGS); \
 extern void gl_##pfx##_transform_normalize_normals_##masked## (NORM_ARGS);    \
 extern void gl_##pfx##_transform_normalize_normals_no_rot_##masked## (NORM_ARGS);



#define ASSIGN_NORM_GROUP( pfx, cma, masked )                                 \
   gl_normal_tab[NORM_RESCALE][cma]   =                                       \
      gl_##pfx##_rescale_normals_##masked##;                                  \
   gl_normal_tab[NORM_NORMALIZE][cma] =                                       \
      gl_##pfx##_normalize_normals_##masked##;                                \
   gl_normal_tab[NORM_TRANSFORM][cma] =                                       \
      gl_##pfx##_transform_normals_##masked##;                                \
   gl_normal_tab[NORM_TRANSFORM_NO_ROT][cma] =                                \
      gl_##pfx##_transform_normals_no_rot_##masked##;                         \
   gl_normal_tab[NORM_TRANSFORM | NORM_RESCALE][cma] =                        \
      gl_##pfx##_transform_rescale_normals_##masked##;                        \
   gl_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_RESCALE][cma] =                 \
      gl_##pfx##_transform_rescale_normals_no_rot_##masked##;                 \
   gl_normal_tab[NORM_TRANSFORM | NORM_NORMALIZE][cma] =                      \
      gl_##pfx##_transform_normalize_normals_##masked##;                      \
   gl_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_NORMALIZE][cma] =               \
      gl_##pfx##_transform_normalize_normals_no_rot_##masked##;


extern void gl_katmai_project_vertices( GLfloat *first,
					GLfloat *last,
					const GLfloat *m,
					GLuint stride );

extern void gl_katmai_project_clipped_vertices( GLfloat *first,
						GLfloat *last,
						const GLfloat *m,
						GLuint stride,
						const GLubyte *clipmask );

extern void gl_v16_katmai_general_xform( GLfloat *first_vert,
					const GLfloat *m,
					const GLfloat *src,
					GLuint src_stride,
					GLuint count );



void gl_init_katmai_asm_transforms (void)
{
  extern void gl_katmai_transform_rescale_normals_raw(NORM_ARGS);
  extern void gl_katmai_transform_rescale_normals_no_rot_raw(NORM_ARGS);
  extern void gl_katmai_transform_normals_no_rot_raw(NORM_ARGS);
  
  /* Some functions are not written in SSE-assembly, because the fpu ones are faster */
  extern void gl_katmai_transform_points4_general_raw(XFORM_ARGS);
  extern void gl_katmai_transform_points4_general_masked(XFORM_ARGS);
  extern void gl_katmai_transform_points4_identity_masked(XFORM_ARGS);
  extern void gl_katmai_transform_points4_3d_no_rot_masked(XFORM_ARGS);
  extern void gl_katmai_transform_points4_3d_raw(XFORM_ARGS);
  extern void gl_katmai_transform_points4_3d_masked(XFORM_ARGS);


  DECLARE_XFORM_GROUP( katmai, 1, raw )
  DECLARE_XFORM_GROUP( katmai, 1, masked )

  DECLARE_XFORM_GROUP( katmai, 2, raw )
  DECLARE_XFORM_GROUP( katmai, 2, masked )

  DECLARE_XFORM_GROUP( katmai, 3, raw )
  DECLARE_XFORM_GROUP( katmai, 3, masked )


  gl_normal_tab[NORM_TRANSFORM | NORM_RESCALE][0]=gl_katmai_transform_rescale_normals_raw;
  gl_normal_tab[NORM_TRANSFORM_NO_ROT | NORM_RESCALE][0]=gl_katmai_transform_rescale_normals_no_rot_raw; 
  gl_normal_tab[NORM_TRANSFORM_NO_ROT][0]=gl_katmai_transform_normals_no_rot_raw;


  gl_transform_tab[0][4][MATRIX_GENERAL]=gl_katmai_transform_points4_general_raw;
  gl_transform_tab[CULL_MASK_ACTIVE][4][MATRIX_GENERAL]=gl_katmai_transform_points4_general_masked;

  gl_transform_tab[CULL_MASK_ACTIVE][4][MATRIX_IDENTITY]=gl_katmai_transform_points4_identity_masked;

  gl_transform_tab[CULL_MASK_ACTIVE][4][MATRIX_3D_NO_ROT]=gl_katmai_transform_points4_3d_no_rot_masked;

  gl_transform_tab[0][4][MATRIX_3D]=gl_katmai_transform_points4_3d_raw;
  gl_transform_tab[CULL_MASK_ACTIVE][4][MATRIX_3D]=gl_katmai_transform_points4_3d_masked;


  ASSIGN_XFORM_GROUP( katmai, 0, 1, raw )
  ASSIGN_XFORM_GROUP( katmai, CULL_MASK_ACTIVE, 1, masked )

  ASSIGN_XFORM_GROUP( katmai, 0, 2, raw )
  ASSIGN_XFORM_GROUP( katmai, CULL_MASK_ACTIVE, 2, masked )

  ASSIGN_XFORM_GROUP( katmai, 0, 3, raw )
  ASSIGN_XFORM_GROUP( katmai, CULL_MASK_ACTIVE, 3, masked )


  /* TODO ! (some parts of it) */

  /*
   DECLARE_NORM_GROUP( katmai, raw )

   ASSIGN_NORM_GROUP( katmai, 0, raw )
  */

#ifdef DEBUG
  gl_test_all_transform_functions("Katmai!");
  gl_test_all_normal_transform_functions("Katmai!");
#endif
   
   /* done */

   /* Hook in some stuff for vertices.c.
    */
   gl_xform_points3_v16_general = gl_v16_katmai_general_xform;
   
   /* test, if it works correctly ! */
   gl_project_v16 = gl_katmai_project_vertices;
   
   /* test, if it works correctly ! */
   gl_project_clipped_v16 = gl_katmai_project_clipped_vertices;
}

#endif
