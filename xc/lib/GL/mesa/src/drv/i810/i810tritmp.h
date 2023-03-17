
static __inline__ void TAG(triangle)( GLcontext *ctx, GLuint e0,
				      GLuint e1, GLuint e2, GLuint pv )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   struct vertex_buffer *VB = ctx->VB;
   i810VertexPtr i810VB = I810_DRIVER_DATA(VB)->verts;
   const i810_vertex *v0 = &i810VB[e0].v;  
   const i810_vertex *v1 = &i810VB[e1].v;  
   const i810_vertex *v2 = &i810VB[e2].v;  

#if (IND & I810_OFFSET_BIT)
   GLfloat offset = ctx->Polygon.OffsetUnits * 1.0/0x10000;
#endif

#if (IND & (I810_FLAT_BIT|I810_TWOSIDE_BIT))
   int c0 = *(int *)&i810VB[pv].v.color; 
   int c1 = c0;
   int c2 = c0;
#endif


#if (IND & (I810_TWOSIDE_BIT|I810_OFFSET_BIT)) 
   {
      GLfloat ex = v0->x - v2->x;
      GLfloat ey = v0->y - v2->y;
      GLfloat fx = v1->x - v2->x;
      GLfloat fy = v1->y - v2->y;
      GLfloat c = ex*fy-ey*fx;
	 
#if (IND & I810_TWOSIDE_BIT) 
      {
	 GLuint facing = (c>0.0) ^ ctx->Polygon.FrontBit;
	 GLubyte (*vbcolor)[4] = VB->Color[facing]->data;
	 if (IND & I810_FLAT_BIT) {
	    I810_COLOR((char *)&c0,vbcolor[pv]);
	    c2 = c1 = c0;
	 } else {
	    I810_COLOR((char *)&c0,vbcolor[e0]);
	    I810_COLOR((char *)&c1,vbcolor[e1]);
	    I810_COLOR((char *)&c2,vbcolor[e2]);
	 }
      }
#endif
      
#if (IND & I810_OFFSET_BIT) 
      {
	 if (c * c > 1e-16) {
	    GLfloat factor = ctx->Polygon.OffsetFactor;
	    GLfloat ez = v0->z - v2->z;
	    GLfloat fz = v1->z - v2->z;
	    GLfloat a = ey*fz-ez*fy;
	    GLfloat b = ez*fx-ex*fz;
	    GLfloat ic = 1.0 / c;
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

   i810glx.c_triangles++;

   
   BEGIN_CLIP_LOOP(imesa)
      {
	 i810_vertex *wv = (i810_vertex *)i810AllocPrimitiveVerts( imesa, 30 );
	 wv[0] = *v0;
#if (IND & (I810_FLAT_BIT|I810_TWOSIDE_BIT))
	 *((int *)(&wv[0].color)) = c0;
#endif
#if (IND & I810_OFFSET_BIT)
	 wv[0].z = v0->z + offset;
#endif


	 wv[1] = *v1;
#if (IND & (I810_FLAT_BIT|I810_TWOSIDE_BIT))
	 *((int *)(&wv[1].color)) = c1;
#endif
#if (IND & I810_OFFSET_BIT)
	 wv[1].z = v1->z + offset;
#endif

	 wv[2] = *v2;
#if (IND & (I810_FLAT_BIT|I810_TWOSIDE_BIT))
	 *((int *)(&wv[2].color)) = c2;
#endif
#if (IND & I810_OFFSET_BIT)
	 wv[2].z = v2->z + offset;
#endif

	 FINISH_PRIM();
      }
   END_CLIP_LOOP(imesa);
}



static void TAG(quad)( GLcontext *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3, 
		       GLuint pv )
{
   TAG(triangle)( ctx, v0, v1, v3, pv );
   TAG(triangle)( ctx, v1, v2, v3, pv );
}


#if ((IND & ~I810_FLAT_BIT) == 0)

static void TAG(line)( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   i810VertexPtr i810VB = I810_DRIVER_DATA(ctx->VB)->verts;
   i810_vertex tmp0 = i810VB[v0].v;
   i810_vertex tmp1 = i810VB[v1].v;
   float width = ctx->Line.Width;

   if (IND & I810_FLAT_BIT) {
      *(int *)&tmp1.color = *(int *)&tmp0.color = 
	 *(int *)&i810VB[pv].v.color;
   }

   BEGIN_CLIP_LOOP(imesa)
      i810_draw_line( imesa, &tmp0, &tmp1, width );
   END_CLIP_LOOP(imesa);
}


static void TAG(points)( GLcontext *ctx, GLuint first, GLuint last )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   struct vertex_buffer *VB = ctx->VB;
   i810VertexPtr i810VB = I810_DRIVER_DATA(VB)->verts;
   GLfloat sz = ctx->Point.Size * .5;
   int i;

   /* Culling is disabled automatically via. the
    * ctx->Driver.ReducedPrimitiveChange() callback.  
    */
   
   BEGIN_CLIP_LOOP(imesa)
      for(i=first;i<=last;i++) {
	 if(VB->ClipMask[i]==0) {
	    i810_vertex *tmp = &i810VB[i].v;
	    i810_draw_point( imesa, tmp, sz );
	 }
      }
   END_CLIP_LOOP(imesa);
}

#endif


static void TAG(init)( void )
{
   tri_tab[IND] = TAG(triangle);
   quad_tab[IND] = TAG(quad);

#if ((IND & ~I810_FLAT_BIT) == 0) 
   line_tab[IND] = TAG(line);
   points_tab[IND] = TAG(points);
#endif
}


#undef IND
#undef TAG
