/*
Copyright (c) 1998 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* $XFree86: xc/lib/font/fontfile/encparse.c,v 1.12 1999/08/21 13:48:02 dawes Exp $ */

/* Parser for encoding files */

/* This code is ASCII-dependent */

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "fntfilio.h"
#include "fntfilst.h"

#include "fontenc.h"
#include "fontencI.h"

#define MAXALIASES 20

#define EOF_TOKEN -1
#define ERROR_TOKEN -2
#define EOL_TOKEN 0
#define NUMBER_TOKEN 1
#define KEYWORD_TOKEN 2

#define EOF_LINE -1
#define ERROR_LINE -2
#define STARTENCODING_LINE 1
#define STARTMAPPING_LINE 2
#define ENDMAPPING_LINE 3
#define CODE_LINE 4
#define CODE_RANGE_LINE 5
#define CODE_UNDEFINE_LINE 6
#define NAME_LINE 7
#define SIZE_LINE 8
#define ALIAS_LINE 9

/* Return from lexer */
#define MAXKEYWORDLEN 100

static long number_value;
static char keyword_value[MAXKEYWORDLEN+1];

static long value1, value2, value3;

/* Lexer code */

/* Skip to the beginning of new line */
static void
skipEndOfLine(FontFilePtr f, int c)
{
  if(c==0)
    c==FontFileGetc(f);
  
  for(;;)
    if(c<=0 || c=='\n')
      return;
    else
      c=FontFileGetc(f);
}

/* Get a number; we're at the first digit. */
static long 
getnum(FontFilePtr f, int c, int *cp)
{
  long n=0;
  int base=10;

  /* look for `0' or `0x' prefix */
  if(c=='0') {
    c=FontFileGetc(f);
    base=8;
    if(c=='x' || c=='X') {
      base=16;
      c=FontFileGetc(f);
    }
  }

  /* accumulate digits */
  for(;;) {
    if('0'<=c && c<='9') {
      n*=base; n+=c-'0';
    } else if('a'<=c && c<='f') {
      n*=base; n+=c-'a'+10;
    } else if('A'<=c && c<='F') {
      n*=base; n+=c-'A'+10;
    } else
      break;
    c=FontFileGetc(f);
  }

  *cp=c; return n;
}
 
/* Skip to beginning of new line; return 1 if only whitespace was found. */
static int
endOfLine(FontFilePtr f, int c)
{
  if(c==0)
    c=FontFileGetc(f);

  for(;;) {
    if(c<=0 || c=='\n')
      return 1;
    else if(c=='#') {
      skipEndOfLine(f,c);
      return 1;
    }
    else if(!isspace(c)) {
      skipEndOfLine(f,c);
      return 0;
    }
    c=FontFileGetc(f);
  }
}

/* Get a token; we're at first char */
static int
gettoken(FontFilePtr f, int c, int *cp)
{
  char *p;

  if(c<=0)
    c=FontFileGetc(f);

  if(c<=0) {
    return EOF_TOKEN;
  }

  while(isspace(c) && c!='\n')
    c=FontFileGetc(f);

  if(c=='\n') {
    return EOL_TOKEN;
  } else if(c=='#') {
    skipEndOfLine(f,c);
    return EOL_TOKEN;
  } else if(isdigit(c)) {
    number_value=getnum(f,c,cp);
    return NUMBER_TOKEN;
  } else if(isalpha(c) || c=='/' || c=='_' || c=='-' || c=='.') {
    p=keyword_value;
    *p++=c;
    while(p-keyword_value<MAXKEYWORDLEN) {
      c=FontFileGetc(f);
      if(c<=0 || isspace(c) || c=='#')
        break;
      *p++=c;
    }
    *cp=c;
    *p='\0';
    return KEYWORD_TOKEN;
  } else {
    *cp=c;
    return ERROR_TOKEN;
  }
}

/* Parse a line.
 * Always skips to the beginning of a new line, even if an error occurs */
static int
getnextline(FontFilePtr f)
{
  int c, token, i;
  c=FontFileGetc(f);
  if(c<=0)
    return EOF_LINE;

retry:
  token=gettoken(f,c,&c);

  switch(token) {
  case EOF_TOKEN:
    return EOF_LINE;
  case EOL_TOKEN:
    /* empty line */
    c=FontFileGetc(f);
    goto retry;
  case NUMBER_TOKEN:
    value1=number_value;
    token=gettoken(f,c,&c);
    switch(token) {
    case NUMBER_TOKEN:
      value2=number_value;
      token=gettoken(f,c,&c);
      switch(token) {
      case NUMBER_TOKEN:
        value3=number_value;
        return CODE_RANGE_LINE;
      case EOL_TOKEN:
        return CODE_LINE;
      default:
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    case KEYWORD_TOKEN:
      if(!endOfLine(f,c))
        return ERROR_LINE;
      else
        return NAME_LINE;
    default:
      skipEndOfLine(f,c);
      return ERROR_LINE;
    }
  case KEYWORD_TOKEN:
    if(!strcasecmp(keyword_value, "STARTENCODING")) {
      token=gettoken(f,c,&c);
      if(token==KEYWORD_TOKEN) {
        if(endOfLine(f,c))
          return STARTENCODING_LINE;
        else
          return ERROR_LINE;
      } else {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "ALIAS")) {
      token=gettoken(f,c,&c);
      if(token==KEYWORD_TOKEN) {
        if(endOfLine(f,c))
          return ALIAS_LINE;
        else
          return ERROR_LINE;
      } else {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "SIZE")) {
      token=gettoken(f,c,&c);
      if(token==NUMBER_TOKEN) {
        value1=number_value;
        token=gettoken(f,c,&c);
        switch(token) {
        case NUMBER_TOKEN:
          value2=number_value;
          return SIZE_LINE;
        case EOL_TOKEN:
          value2=0;
          return SIZE_LINE;
        default:
          skipEndOfLine(f,c);
          return ERROR_LINE;
        }
      } else {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "STARTMAPPING")) {
      keyword_value[0]=0;
      value1=0; value1=0;
      /* read up to three tokens, the first being a keyword */
      for(i=0; i<3; i++) {
        token=gettoken(f,c,&c);
        if(token==EOL_TOKEN) {
          if(i>0)
            return STARTMAPPING_LINE;
          else
            return ERROR_LINE;
        } else if(token==KEYWORD_TOKEN) {
          if(i!=0) {
            skipEndOfLine(f,c);
            return ERROR_LINE;
          }
        } else if(token==NUMBER_TOKEN) {
          if(i==1) value1=number_value;
          else if(i==2) value2=number_value;
          else {
            skipEndOfLine(f,c);
            return ERROR_LINE;
          }
        } else {
          skipEndOfLine(f,c);
          return ERROR_LINE;
        }
      }
      if(!endOfLine(f,c))
         return ERROR_LINE;
      else {
        return STARTMAPPING_LINE;
      }
    } else if(!strcasecmp(keyword_value, "UNDEFINE")) {
      token=gettoken(f,c,&c);
      if(token!=NUMBER_TOKEN) {
        skipEndOfLine(f,c);
        return ERROR_LINE;
      }
      value1=number_value;
      token=gettoken(f,c,&c);
      if(token==EOL_TOKEN) {
        value2=value1;
        return CODE_UNDEFINE_LINE;
      } else if(token==NUMBER_TOKEN) {
        value2=number_value;
        if(endOfLine(f,c)) {
          return CODE_UNDEFINE_LINE;
        } else
          return ERROR_LINE;
      }
    } else if(!strcasecmp(keyword_value, "ENDENCODING")) {
      if(endOfLine(f,c))
        return EOF_LINE;
      else
        return ERROR_LINE;
    } else if(!strcasecmp(keyword_value, "ENDMAPPING")) {
      if(endOfLine(f,c))
        return ENDMAPPING_LINE;
      else
        return ERROR_LINE;
    } else {
      skipEndOfLine(f,c);
      return ERROR_LINE;
    }
  default:
    return ERROR_LINE;
  }
}

static void 
install_mapping(struct font_encoding *encoding,
                struct font_encoding_mapping *mapping)
{
  struct font_encoding_mapping *m;

  if(encoding->mappings==NULL)
    encoding->mappings=mapping;
  else {
    m=encoding->mappings;
    while(m->next!=NULL)
      m=m->next;
    m->next=mapping;
  }
  mapping->next=NULL;
}

static int
setCode(unsigned from, unsigned to, unsigned row_size,
        unsigned *first, unsigned *last,
        unsigned *encsize, unsigned short **enc)
{
  unsigned index, i;
  unsigned short *newenc;

  if(from>0xFFFF)
    return 0;                   /* success */

  if(row_size==0)
    index=from;
  else {
    if((value1&0xFF)>=row_size)
      return 0;                 /* ignore out-of-range mappings */
    index=(from>>8)*row_size+(from&0xFF);
  }

  /* Optimize away useless identity mappings.  This is only expected
   * to be useful with linear encodings. */
  if(index==to && (index<*first || index>*last))
    return 0;
  if(*encsize==0) {
    *encsize=index<256?256:0x10000;
    if((*enc=
        (unsigned short*)xalloc(*encsize*sizeof(unsigned short)))==NULL) {
      encsize=0;
      return 1;
    }
  } else if(*encsize<=index) {
    *encsize=0x10000;
    if((newenc=(unsigned short*)xrealloc(enc, *encsize))==NULL)
      return 1;
    *enc=newenc;
  }
  if(*first>*last) {
    *first=*last=index;
  }
  if(index<*first) {
    for(i=index; i<*first; i++)
      (*enc)[i]=i;
    *first=index;
  }
  if(index>*last) {
    for(i=*last+1; i<=index; i++)
      (*enc)[i]=i;
    *last=index;
  }
  (*enc)[index]=to;
  return 0;
}

/* Parser.  If headerOnly is true, we're only interested in the
 * encodings's header. */
static struct font_encoding*
parseEncodingFile(FontFilePtr f, int headerOnly)
{
  int line;

  unsigned short *enc=NULL;
  char **nam=NULL, **newnam;
  unsigned i, first=0xFFFF, last=0, encsize=0, namsize=0;
  struct font_encoding *encoding=NULL;
  struct font_encoding_mapping *mapping=NULL;
  struct font_encoding_simple_mapping *sm;
  struct font_encoding_simple_naming *sn;
  char *aliases[MAXALIASES];
  int numaliases=0;

/* no_encoding: */
  line=getnextline(f);
  switch(line) {
  case EOF_LINE:
    goto error;
  case STARTENCODING_LINE:
    if((encoding=
        (struct font_encoding*)xalloc(sizeof(struct font_encoding)))
       ==NULL)
      goto error;
    if((encoding->name=(char*)xalloc(strlen(keyword_value)+1))==NULL)
      goto error;
    strcpy(encoding->name, keyword_value);
    encoding->size=256;
    encoding->row_size=0;
    encoding->mappings=NULL;
    encoding->next=NULL;
    goto no_mapping;
  default:
    goto error;
  }

no_mapping:
  line=getnextline(f);
  switch(line) {
  case EOF_LINE: goto done;
  case ALIAS_LINE:
    if(numaliases<MAXALIASES) {
      if((aliases[numaliases]=(char*)xalloc(strlen(keyword_value)+1))
         ==NULL)
        goto error;
      strcpy(aliases[numaliases], keyword_value);
      numaliases++;
    }
    goto no_mapping;
  case SIZE_LINE:
    encoding->size=value1;
    encoding->row_size=value2;
    goto no_mapping;
  case STARTMAPPING_LINE:
    if(headerOnly)
      goto done;
    if(!strcasecmp(keyword_value, "unicode")) {
      if((mapping=
          (struct font_encoding_mapping*)
          xalloc(sizeof(struct font_encoding_mapping)))
         ==NULL)
        goto error;
      mapping->type = FONT_ENCODING_UNICODE;
      mapping->pid = 0;
      mapping->eid = 0;
      mapping->recode=0;
      mapping->name=0;
      mapping->client_data=0;
      mapping->next=0;
    } else if(!strcasecmp(keyword_value, "cmap")) {
      if((mapping=
          (struct font_encoding_mapping*)
          xalloc(sizeof(struct font_encoding_mapping)))
         ==NULL)
        goto error;
      mapping->type = FONT_ENCODING_TRUETYPE;
      mapping->pid = value1;
      mapping->eid = value2;
      mapping->recode=0;
      mapping->name=0;
      mapping->client_data=0;
      mapping->next=0;
      goto mapping;
    } else if(!strcasecmp(keyword_value, "postscript")) {
      if((mapping=
          (struct font_encoding_mapping*)
          xalloc(sizeof(struct font_encoding_mapping)))
         ==NULL)
        goto error;
      mapping->type = FONT_ENCODING_POSTSCRIPT;
      mapping->pid = 0;
      mapping->eid = 0;
      mapping->recode=0;
      mapping->name=0;
      mapping->client_data=0;
      mapping->next=0;
      goto string_mapping;
    } else {                    /* unknown mapping type -- ignore */
      goto skipmapping;
    }
    goto mapping;
  default: goto no_mapping;     /* ignore unknown lines */
  }

skipmapping:
  line=getnextline(f);
  switch(line) {
  case ENDMAPPING_LINE:
    goto no_mapping;
  case EOF_LINE:
    goto error;
  default:
    goto skipmapping;
  }
    

mapping:
  line=getnextline(f);
  switch(line) {
  case EOF_LINE: goto error;
  case ENDMAPPING_LINE:
    mapping->recode=font_encoding_simple_recode;
    mapping->name=font_encoding_undefined_name;
    if((mapping->client_data=sm=
        (struct font_encoding_simple_mapping*)
        xalloc(sizeof(struct font_encoding_simple_mapping)))==NULL)
      goto error;
    sm->row_size=encoding->row_size;
    if(first<=last) {
      sm->first=first;
      sm->len=last-first+1;
      if((sm->map=
          (unsigned short*)xalloc(sm->len*sizeof(unsigned short)))
         ==NULL) {
        xfree(sm);
        mapping->client_data=sm=NULL;
        goto error;
      }
    } else {
      sm->first=0;
      sm->len=0;
      sm->map=0;
    }
    for(i=0; i<sm->len; i++)
      sm->map[i]=enc[first+i];
    install_mapping(encoding,mapping);
    mapping=0;
    first=0xFFFF; last=0;
    goto no_mapping;

  case CODE_LINE:
    if(setCode(value1, value2, encoding->row_size,
               &first, &last, &encsize, &enc))
      goto error;
    goto mapping;

  case CODE_RANGE_LINE:
    if(value1>0x10000)
      value1=0x10000;
    if(value2>0x10000)
      value2=0x10000;
    if(value2<value1)
      goto mapping;
    /* Do the last value first to avoid having to realloc() */
    if(setCode(value2, value3+(value2-value1), encoding->row_size,
               &first, &last, &encsize, &enc))
      goto error;
    for(i=value1; i<value2; i++) {
      if(setCode(i, value3+(i-value1), encoding->row_size,
                 &first, &last, &encsize, &enc))
        goto error;
    }
    goto mapping;
    
  case CODE_UNDEFINE_LINE:
    if(value1>0x10000)
      value1=0x10000;
    if(value2>0x10000)
      value2=0x10000;
    if(value2<value1)
      goto mapping;
    /* Do the last value first to avoid having to realloc() */
    if(setCode(value2, 0, encoding->row_size,
               &first, &last, &encsize, &enc))
      goto error;
    for(i=value1; i<value2; i++) {
      if(setCode(i, 0, encoding->row_size,
                 &first, &last, &encsize, &enc))
        goto error;
    }
    goto mapping;

  default: goto mapping;        /* ignore unknown lines */
  }

string_mapping:
  line=getnextline(f);
  switch(line) {
  case EOF_LINE: goto error;
  case ENDMAPPING_LINE:
    mapping->recode=font_encoding_undefined_recode;
    mapping->name=font_encoding_simple_name;
    if((mapping->client_data=sn=
        (struct font_encoding_simple_naming*)
        xalloc(sizeof(struct font_encoding_simple_naming)))==NULL)
      goto error;
    if(first>last) {
      xfree(sn);
      mapping->client_data=sn=NULL;
      goto error;
    }
    sn->first=first;
    sn->len=last-first+1;
    if((sn->map=
        (char**)xalloc(sn->len*sizeof(char*)))
       ==NULL) {
      xfree(sn);
      mapping->client_data=sn=NULL;
      goto error;
    }
    for(i=0; i<sn->len; i++)
      sn->map[i]=nam[first+i];
    install_mapping(encoding,mapping);
    mapping=0;
    first=0xFFFF; last=0;
    goto no_mapping;
  case NAME_LINE:
    if(value1>=0x10000) goto string_mapping;
    if(namsize==0) {
      namsize=value1<256?256:0x10000;
      if((nam=(char**)xalloc(namsize*sizeof(char*)))==NULL) {
        namsize=0;
        goto error;
      }
    } else if(namsize<=value1) {
      namsize=0x10000;
      if((newnam=(char**)xrealloc(nam, namsize))==NULL)
        goto error;
      nam=newnam;
    }
    if(first>last) {
      first=last=value1;
    }
    if(value1<first) {
      for(i=value1; i<first; i++)
        nam[i]=NULL;
      first=value1;
    }
    if(value1>last) {
      for(i=last+1; i<=value1; i++)
        nam[i]=NULL;
      last=value1;
    }
    if((nam[value1]=(char*)xalloc(strlen(keyword_value)+1))==NULL) {
      goto error;
    }
    strcpy(nam[value1], keyword_value);
    goto string_mapping;

  default: goto string_mapping; /* ignore unknown lines */
  }

done:
  if(encsize) xfree(enc); encsize=0;
  if(namsize) xfree(nam); namsize=0; /* don't free entries! */

  encoding->aliases=NULL;
  if(numaliases) {
    if((encoding->aliases=(char**)xalloc((numaliases+1)*sizeof(char*)))
       ==NULL)
      goto error;
    for(i=0; i<numaliases; i++)
      encoding->aliases[i]=aliases[i];
    encoding->aliases[numaliases]=NULL;
  }

  return encoding;

error:
  if(encsize) xfree(enc); encsize=0;
  if(namsize) {
    for(i=first; i<=last; i++)
      if(nam[i])
        xfree(nam[i]);
    xfree(nam);
    namsize=0;
  }
  if(mapping) {
    if(mapping->client_data) xfree(mapping->client_data);
    xfree(mapping);
  }
  if(encoding) {
    if(encoding->name) xfree(encoding->name);
    for(mapping=encoding->mappings; mapping; mapping=mapping->next) {
      if(mapping->client_data) xfree(mapping->client_data);
      xfree(mapping);
    }
    xfree(encoding);
  }
  for(i=0; i<numaliases; i++)
    xfree(aliases[i]);
  /* We don't need to free sn and sm as they handled locally in the body.*/
  return 0;
}

/* Public entrypoint */  
struct font_encoding* 
loadEncodingFile(const char *charset, const char *fontFileName)
{
  FontFilePtr f;
  FILE *file;
  struct font_encoding *encoding;
  const char *p;
  char dir[MAXFONTFILENAMELEN], buf[MAXFONTFILENAMELEN],
    file_name[MAXFONTFILENAMELEN], encoding_name[MAXFONTNAMELEN],
    *q, *lastslash;
  int count, n;

  for(p=fontFileName, q=dir, lastslash=NULL; *p; p++, q++) {
    *q=*p;
    if(*p=='/')
      lastslash=q+1;
  }

  if(!lastslash)
    lastslash=dir;

  *lastslash='\0';

  /* As we don't really expect to open encodings that often, we don't
   * take the trouble of caching encodings directories. */

  strcpy(buf, dir);
  strcat(buf, "encodings.dir");

  if((file=fopen(buf, "r"))==NULL) {
    return NULL;
  }

  count=fscanf(file, "%d\n", &n);
  if(count==EOF || count!=1) {
    fclose(file);
    return NULL;
  }

  encoding=NULL;
  for(;;) {
    if((count=fscanf(file, "%s %[^\n]\n", encoding_name, file_name))==EOF)
      break;
    if(count!=2)
      break;
    if(!strcasecmp(encoding_name, charset)) {
      if(file_name[0]!='/') {
        if(strlen(dir)+strlen(file_name)>=MAXFONTFILENAMELEN)
          return NULL;
        strcpy(buf, dir);
        strcat(buf, file_name);
      } else
        strcpy(buf,file_name);
      if((f=FontFileOpen(buf))==NULL) {
        return NULL;
      }
      encoding=parseEncodingFile(f, 0);
      FontFileClose(f);
      break;
    }
  }

  fclose(file);

  return encoding;
}

/* Return a NULL-terminated array of encoding names.  Note that this
 * file has incestuous knowledge of the allocations done by
 * parseEncodingFile. */

char **
identifyEncodingFile(const char *fileName)
{
  FontFilePtr f;
  struct font_encoding *encoding;
  char **names, **name, **alias;
  int numaliases;

  if((f=FontFileOpen(fileName))==NULL) {
    return NULL;
  }
  encoding=parseEncodingFile(f, 1);
  FontFileClose(f);

  if(!encoding)
    return NULL;

  numaliases=0;
  if(encoding->aliases)
    for(alias=encoding->aliases; *alias; alias++)
      numaliases++;

  if((names=(char**)xalloc((numaliases+2)*sizeof(char*)))==NULL)
    return NULL;

  name=names;
  *(name++)=encoding->name;
  if(numaliases>0)
    for(alias=encoding->aliases; *alias; alias++, name++)
      *name=*alias;

  *name=0;
  xfree(encoding->aliases);
  xfree(encoding);

  return names;
}

  
