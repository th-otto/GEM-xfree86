
static __inline void TAG(triangle)( GLcontext *ctx, GLuint e0,
				  GLuint e1, GLuint e2, GLuint pv )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint vertsize = mmesa->vertsize;
   mgaUI32 *wv = mgaAllocVertexDwords( mmesa, 3 * vertsize);

   struct vertex_buffer *VB = ctx->VB;
   mgaVertexPtr mgaVB = MGA_DRIVER_DATA(VB)->verts;
   const mgaVertex *v[3];
   int i, j;

#if (IND & MGA_OFFSET_BIT)
   GLfloat offset = ctx->Polygon.OffsetUnits * 1.0/0x10000;
#endif

#if (IND & (MGA_FLAT_BIT|MGA_TWOSIDE_BIT))
   mgaUI32 c[3];

   c[2] = c[1] = c[0] = *(mgaUI32 *)&mgaVB[pv].warp2.color; 
#endif

   (void) VB;

   v[0] = &mgaVB[e0];  
   v[1] = &mgaVB[e1];  
   v[2] = &mgaVB[e2];  


#if (IND & (MGA_TWOSIDE_BIT|MGA_OFFSET_BIT)) 
   {
      GLfloat ex = v[0]->warp1.x - v[2]->warp1.x;
      GLfloat ey = v[0]->warp1.y - v[2]->warp1.y;
      GLfloat fx = v[1]->warp1.x - v[2]->warp1.x;
      GLfloat fy = v[1]->warp1.y - v[2]->warp1.y;
      GLfloat cc = ex*fy-ey*fx;
	 
#if (IND & MGA_TWOSIDE_BIT) 
      {
	 GLuint facing = (cc>0.0) ^ ctx->Polygon.FrontBit;
	 GLubyte (*vbcolor)[4] = VB->Color[facing]->data;
	 if (IND & MGA_FLAT_BIT) {
	    MGA_COLOR((char *)&c[0],vbcolor[pv]);
	    c[2] = c[1] = c[0];
	 } else {
	    MGA_COLOR((char *)&c[0],vbcolor[e0]);
	    MGA_COLOR((char *)&c[1],vbcolor[e1]);
	    MGA_COLOR((char *)&c[2],vbcolor[e2]);
	 }
      }
#endif
      
#if (IND & MGA_OFFSET_BIT) 
      {
	 if (cc * cc > 1e-16) {
	    GLfloat factor = ctx->Polygon.OffsetFactor;
	    GLfloat ez = v[0]->warp1.z - v[2]->warp1.z;
	    GLfloat fz = v[1]->warp1.z - v[2]->warp1.z;
	    GLfloat a = ey*fz-ez*fy;
	    GLfloat b = ez*fx-ex*fz;
	    GLfloat ic = 1.0 / cc;
	    GLfloat ac = a * ic;
	    GLfloat bc = b * ic;
	    if (ac<0.0F)  ac = -ac;
	    if (bc<0.0F)  bc = -bc;
	    offset += MAX2( ac, bc ) * factor;
	 }
      }
#endif
   }
#endif  

   mgaglx.c_triangles++;

   for (j = 0 ; j < 3 ; j++, wv += vertsize) {

      for (i = 0 ; i < vertsize ; i++)
	 wv[i] = v[j]->ui[i];

#if (IND & (MGA_FLAT_BIT|MGA_TWOSIDE_BIT))
      wv[4] = c[j];		/* color is the fifth element... */
#endif
#if (IND & MGA_OFFSET_BIT)
      *(float *)&wv[2] = v[j]->warp1.z + offset;
#endif
   }
}



static void TAG(quad)( GLcontext *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3, 
		       GLuint pv )
{
   TAG(triangle)( ctx, v0, v1, v3, pv );
   TAG(triangle)( ctx, v1, v2, v3, pv );
}


#if ((IND & ~MGA_FLAT_BIT) == 0)

static void TAG(line)( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaVertexPtr mgaVB = MGA_DRIVER_DATA(ctx->VB)->verts;
   float width = ctx->Line.Width;

   if (IND & MGA_FLAT_BIT) {
      mgaVertex tmp0 = mgaVB[v0];
      mgaVertex tmp1 = mgaVB[v1];
      *(int *)&tmp0.warp1.color = *(int *)&mgaVB[pv].warp1.color;
      *(int *)&tmp1.warp1.color = *(int *)&mgaVB[pv].warp1.color;
      mga_draw_line( mmesa, &tmp0, &tmp1, width );
   }
   else
      mga_draw_line( mmesa, &mgaVB[v0], &mgaVB[v1], width );
}


static void TAG(points)( GLcontext *ctx, GLuint first, GLuint last )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   struct vertex_buffer *VB = ctx->VB;
   mgaVertexPtr mgaVB = MGA_DRIVER_DATA(VB)->verts;
   GLfloat sz = ctx->Point.Size * .5;
   int i;

   for(i=first;i<=last;i++) 
      if(VB->ClipMask[i]==0) 
	 mga_draw_point( mmesa, &mgaVB[i], sz );
}

#endif


static void TAG(init)( void )
{
   tri_tab[IND] = TAG(triangle);
   quad_tab[IND] = TAG(quad);

#if ((IND & ~MGA_FLAT_BIT) == 0) 
   line_tab[IND] = TAG(line);
   points_tab[IND] = TAG(points);
#endif
}


#undef IND
#undef TAG
