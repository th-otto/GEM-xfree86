
static void TAG(WriteRGBASpan)( const GLcontext *ctx, 
				GLuint n, GLint x, GLint y, 
				const GLubyte rgba[][4], 
				const GLubyte mask[] )
{
   GLuint x1,n1;
   LOCAL_VARS;

   y = Y_FLIP(y);

   if (DBG) fprintf(stderr, "WriteRGBASpan\n");

   HW_CLIPLOOP() 
      {
	 GLuint i = 0;
	 CLIPSPAN(x,y,n,x1,n1,i);
	 if (mask)
	 {
	    for (i=0;i<n1;i++,x1++)
	       if (mask[i])
		  WRITE_RGBA( x1, y, 
			      rgba[i][0], rgba[i][1], 
			      rgba[i][2], rgba[i][3] );
	 }
	 else
	 {
	    for (i=0;i<n1;i++,x1++)
	       WRITE_RGBA( x1, y, 
			   rgba[i][0], rgba[i][1], 
			   rgba[i][2], rgba[i][3] );
	 }
      }
   HW_ENDCLIPLOOP();
}

static void TAG(WriteRGBSpan)( const GLcontext *ctx,
			       GLuint n, GLint x, GLint y,
			       const GLubyte rgb[][3], 
			       const GLubyte mask[] )
{
   GLuint x1,n1;
   LOCAL_VARS;

   y = Y_FLIP(y);


   if (DBG) fprintf(stderr, "WriteRGBSpan\n");

   HW_CLIPLOOP() 
      {
	 GLuint i = 0;
	 CLIPSPAN(x,y,n,x1,n1,i);

	 if (mask)
	 {
	    for (;i<n1;i++,x1++)
	       if (mask[i])
		  WRITE_RGBA( x1, y, rgb[i][0], rgb[i][1], rgb[i][2], 0 );
	 }
	 else
	 {
	    for (;i<n1;i++,x1++)
	       WRITE_RGBA( x1, y, rgb[i][0], rgb[i][1], rgb[i][2], 0 );
	 }
      }
   HW_ENDCLIPLOOP();
}

static void TAG(WriteRGBAPixels)( const GLcontext *ctx,
			       GLuint n, 
			       const GLint x[], 
			       const GLint y[],
			       const GLubyte rgba[][4], 
			       const GLubyte mask[] )
{
   GLuint i;
   LOCAL_VARS;

   if (DBG) fprintf(stderr, "WriteRGBAPixels\n");

   HW_CLIPLOOP()
      {
	 for (i=0;i<n;i++)
	 {
	    if (mask[i]) {
	       const int fy = Y_FLIP(y[i]);
	       if (CLIPPIXEL(x[i],fy))
		  WRITE_RGBA( x[i], fy, 
			      rgba[i][0], rgba[i][1], 
			      rgba[i][2], rgba[i][3] );
	    }
	 }
      }
   HW_ENDCLIPLOOP();
}


static void TAG(WriteMonoRGBASpan)( const GLcontext *ctx,	
				    GLuint n, GLint x, GLint y, 
				    const GLubyte mask[] )
{
   GLuint x1,n1;
   LOCAL_VARS;
   INIT_MONO_PIXEL(p);

   y = Y_FLIP( y );

   if (DBG) fprintf(stderr, "WriteMonoRGBASpan\n");

   HW_CLIPLOOP() 
      {
	 GLuint i = 0;
	 CLIPSPAN(x,y,n,x1,n1,i);
	 for (;i<n1;i++,x1++)
	    if (mask[i])
	       WRITE_PIXEL( x1, y, p );
      }
   HW_ENDCLIPLOOP();
}


static void TAG(WriteMonoRGBAPixels)( const GLcontext *ctx,
				      GLuint n, 
				      const GLint x[], const GLint y[],
				      const GLubyte mask[] ) 
{
   GLuint i;
   LOCAL_VARS;
   INIT_MONO_PIXEL(p);

   if (DBG) fprintf(stderr, "WriteMonoRGBAPixels\n");

   HW_CLIPLOOP()
      {
	 for (i=0;i<n;i++)
	    if (mask[i]) {
	       int fy = Y_FLIP(y[i]);
	       if (CLIPPIXEL( x[i], fy ))
		  WRITE_PIXEL( x[i], fy, p );
	    }
      }
   HW_ENDCLIPLOOP();
}



/*
 * Read a horizontal span of color pixels.
 */
static void TAG(ReadRGBASpan)( const GLcontext *ctx,
			       GLuint n, GLint x, GLint y,
			       GLubyte rgba[][4])
{
   GLuint x1,n1;
   LOCAL_VARS;

   y = Y_FLIP(y);

   if (DBG) fprintf(stderr, "ReadRGBASpan\n");

   HW_CLIPLOOP() 
      {
	 GLuint i = 0;
	 CLIPSPAN(x,y,n,x1,n1,i);
	 for (;i<n1;i++)
	    READ_RGBA( rgba[i], x1+i, y );
      }
   HW_ENDCLIPLOOP();
}

static void TAG(ReadRGBAPixels)( const GLcontext *ctx,
				 GLuint n, const GLint x[], const GLint y[],
				 GLubyte rgba[][4], const GLubyte mask[] )
{
   GLuint i;
   LOCAL_VARS;

   if (DBG) fprintf(stderr, "ReadRGBAPixels\n");
 
   HW_CLIPLOOP()
      {
	 for (i=0;i<n;i++)
	    if (mask[i]) {
	       int fy = Y_FLIP( y[i] );
	       if (CLIPPIXEL( x[i], fy ))
		  READ_RGBA( rgba[i], x[i], fy );
	    }
      }
   HW_ENDCLIPLOOP();
}




#undef WRITE_PIXEL
#undef WRITE_RGBA
#undef READ_RGBA
#undef TAG
