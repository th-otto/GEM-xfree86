/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/elfloader.c,v 1.23 1999/12/13 23:56:38 robin Exp $ */

/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef QNX
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>

#ifdef DBMALLOC
#include <debug/malloc.h>
#define Xalloc(size) malloc(size)
#define Xcalloc(size) calloc(1,(size))
#define Xfree(size) free(size)
#endif

#include "Xos.h"
#include "os.h"
#include "elf.h"

#include "sym.h"
#include "loader.h"

/*
#ifndef LDTEST
#define ELFDEBUG ErrorF
#endif
*/

#if defined (__alpha__)
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym Elf_Sym;
typedef Elf64_Rel Elf_Rel;
typedef Elf64_Rela Elf_Rela;
typedef Elf64_Addr Elf_Addr;
#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_TYPE ELF64_ST_TYPE
#define ELF_R_SYM ELF64_R_SYM
#define ELF_R_TYPE ELF64_R_TYPE
/*
 * The GOT is allocated dynamically. We need to keep a list of entries that
 * have already been added to the GOT. 
 *
 */
typedef struct _elf_GOT {
	Elf_Rela   *rel;
        int	offset;
	struct _elf_GOT *next;
} ELFGotRec, *ELFGotPtr;

#else
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Rela Elf_Rela;
typedef Elf32_Addr Elf_Addr;
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_TYPE ELF32_ST_TYPE
#define ELF_R_SYM ELF32_R_SYM
#define ELF_R_TYPE ELF32_R_TYPE
#endif

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef	struct {
	int	handle;
	int	module;
	int	fd;
	loader_funcs	*funcs;
	Elf_Ehdr 	*header;/* file header */
	int	numsh;
	Elf_Shdr	*sections;/* Address of the section header table */
	int	secsize;	/* size of the section table */
	unsigned char	**saddr;/* Start addresss of the section pointer table */
	unsigned char	*shstraddr; /* Start address of the section header string table */
	int	shstrndx;	/* index of the section header string table */
	int	shstrsize;	/* size of the section header string table */
	unsigned char *straddr;	/* Start address of the string table */
	int	strndx;		/* index of the string table */
	int	strsize;	/* size of the string table */
	unsigned char *text;	/* Start address of the .text section */
	int	txtndx;		/* index of the .text section */
	int	txtsize;	/* size of the .text section */
	unsigned char *data;	/* Start address of the .data section */
	int	datndx;		/* index of the .data section */
	int	datsize;	/* size of the .data section */
	unsigned char *data1;	/* Start address of the .data1 section */
	int	dat1ndx;	/* index of the .data1 section */
	int	dat1size;	/* size of the .data1 section */
	unsigned char *sdata;	/* Start address of the .sdata section */
	int	sdatndx;	/* index of the .sdata section */
	int	sdatsize;	/* size of the .sdata section */
	unsigned char *bss;	/* Start address of the .bss section */
	int	bssndx;		/* index of the .bss section */
	int	bsssize;	/* size of the .bss section */
	unsigned char *sbss;	/* Start address of the .sbss section */
	int	sbssndx;        /* index of the .sbss section */
	int	sbsssize;	/* size of the .sbss section */
	unsigned char *rodata;	/* Start address of the .rodata section */
	int	rodatndx;	/* index of the .rodata section */
	int	rodatsize;	/* size of the .rodata section */
	unsigned char *rodata1;	/* Start address of the .rodata section */
	int	rodat1ndx;	/* index of the .rodata section */
	int	rodat1size;	/* size of the .rodata section */
#if defined(__alpha__)
	unsigned char *got;     /* Start address of the .got section */
	ELFGotPtr got_entries;  /* List of entries in the .got section */
	int     gotndx;         /* index of the .got section */
	int     gotsize;        /* size of the .got section */
#endif
	Elf_Sym *symtab;	/* Start address of the .symtab section */
	int	symndx;		/* index of the .symtab section */
	int	symsize;	/* size of the .symtab section */
	unsigned char *reltext;	/* Start address of the .rel.text section */
	int	reltxtndx;	/* index of the .rel.text section */
	int	reltxtsize;	/* size of the .rel.text section */
	unsigned char *reldata;	/* Start address of the .rel.data section */
	int	reldatndx;	/* index of the .rel.data section */
	int	reldatsize;	/* size of the .rel.data section */
	unsigned char *relsdata;/* Start address of the .rel.sdata section */
	int	relsdatndx;	/* index of the .rel.sdata section */
	int	relsdatsize;	/* size of the .rel.sdata section */
	unsigned char *relrodata;/* Start address of the .rel.rodata section */
	int	relrodatndx;	/* index of the .rel.rodata section */
	int	relrodatsize;	/* size of the .rel.rodata section */
	unsigned char *common;	/* Start address of the SHN_COMMON space */
	int	comsize;	/* size of the SHN_COMMON space */
	}	ELFModuleRec, *ELFModulePtr;

/*
 * If a relocation is unable to be satisfied, then put it on a list
 * to try later after more modules have been loaded.
 */
typedef struct _elf_reloc {
#if defined(i386)
	Elf_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
	Elf_Rela	*rel;
#endif
	ELFModulePtr	file;
	unsigned char	*secp;
	struct _elf_reloc	*next;
} ELFRelocRec;

/*
 * symbols with a st_shndx of COMMON need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */
typedef struct _elf_COMMON {
	Elf_Sym	   *sym;
	struct _elf_COMMON *next;
} ELFCommonRec;

static	ELFCommonPtr listCOMMON=NULL;

/* Prototypes for static functions */
static int ELFhashCleanOut(void *, itemPtr);
static char *ElfGetStringIndex(ELFModulePtr, int, int);
static char *ElfGetString(ELFModulePtr, int);
static char *ElfGetSectionName(ELFModulePtr, int);
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
static ELFRelocPtr ElfDelayRelocation(ELFModulePtr, unsigned char *, Elf_Rela *);
#else
static ELFRelocPtr ElfDelayRelocation(ELFModulePtr, unsigned char *, Elf_Rel *);
#endif
static ELFCommonPtr ElfAddCOMMON(Elf_Sym *);
static LOOKUP *ElfCreateCOMMON(ELFModulePtr);
static char *ElfGetSymbolNameIndex(ELFModulePtr, int, int);
static char *ElfGetSymbolName(ELFModulePtr, int);
static Elf_Addr ElfGetSymbolValue(ELFModulePtr, int);
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
static ELFRelocPtr Elf_RelocateEntry(ELFModulePtr, unsigned char *, Elf_Rela *, int);
#else
static ELFRelocPtr Elf_RelocateEntry(ELFModulePtr, unsigned char *, Elf_Rel *, int);
#endif
static ELFRelocPtr ELFCollectRelocations(ELFModulePtr, int);
static LOOKUP *ELF_GetSymbols(ELFModulePtr);
static void ELFCollectSections(ELFModulePtr);
#if defined(__alpha__)
static void ElfAddGOT(ELFModulePtr, Elf_Rela *);
void ELFCreateGOT(ELFModulePtr);
#endif

/*
 * Utility Functions
 */


static int
ELFhashCleanOut(voidptr, item)
void *voidptr;
itemPtr item ;
{
    ELFModulePtr module = (ELFModulePtr) voidptr;
    return (module->handle == item->handle);
}

/*
 * Manage listResolv
 */
static ELFRelocPtr
ElfDelayRelocation(elffile,secp,rel)
ELFModulePtr	elffile;
unsigned char	*secp;
#if defined(i386)
Elf_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
Elf_Rela	*rel;
#endif
{
    ELFRelocPtr	reloc;

    if ((reloc = xf86loadermalloc(sizeof(ELFRelocRec))) == NULL) {
	ErrorF( "ElfDelayRelocation() Unable to allocate memory!!!!\n" );
	return 0;
    }
    reloc->file=elffile;
    reloc->secp=secp;
    reloc->rel=rel;
    reloc->next=0;
#ifdef ELFDEBUG
    ELFDEBUG("ElfDelayRelocation %lx: file %lx, sec %lx, r_offset 0x%x, r_info 0x%x", reloc, elffile, secp, rel->r_offset, rel->r_info);
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
    ELFDEBUG(", r_addend 0x%x", rel->r_addend);
#endif
    ELFDEBUG("\n" );
#endif
    return reloc;
}

/*
 * Manage listCOMMON
 */
static ELFCommonPtr
ElfAddCOMMON(sym)
Elf_Sym	*sym;
{
    ELFCommonPtr common;

    if ((common = xf86loadermalloc(sizeof(ELFCommonRec))) == NULL) {
	ErrorF( "ElfAddCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }
    common->sym=sym;
    common->next=0;
    return common;
}

static LOOKUP *
ElfCreateCOMMON(elffile)
ELFModulePtr	elffile;
{
    int	numsyms=0,size=0,l=0;
    int	offset=0;
    LOOKUP	*lookup;
    ELFCommonPtr common;

    if (listCOMMON == NULL)
	return NULL;

    for (common = listCOMMON; common; common = common->next) {
	size+=common->sym->st_size;
#if defined(__alpha__)
	size = (size+7)&~0x7;
#endif
	numsyms++;
    }

#ifdef ELFDEBUG
    ELFDEBUG("ElfCreateCOMMON() %d entries (%d bytes) of COMMON data\n",
	     numsyms, size );
#endif

    if((lookup = xf86loadermalloc((numsyms+1)*sizeof(LOOKUP))) == NULL) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }

    elffile->comsize=size;
    if((elffile->common = xf86loadercalloc(1,size)) == NULL) {
	ErrorF( "ElfCreateCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }

    if (DebuggerPresent)
    {
	ldrCommons = xf86loadermalloc(numsyms*sizeof(LDRCommon));
	nCommons = numsyms;
    }
    /* Traverse the common list and create a lookup table with all the
     * common symbols.  Destroy the common list in the process.
     * See also ResolveSymbols.
     */
    while(listCOMMON) {
	common=listCOMMON;
	/* this is xstrdup because is should be more efficient. it is freed
	 * with xf86loaderfree
	 */
	lookup[l].symName = xf86loaderstrdup(ElfGetString(elffile,common->sym->st_name));
	lookup[l].offset = (funcptr)(elffile->common + offset);
#ifdef ELFDEBUG
	ELFDEBUG("Adding common %lx %s\n", lookup[l].offset, lookup[l].symName );
#endif
	
	/* Record the symbol address for gdb */
	if (DebuggerPresent && ldrCommons)
	{
	     ldrCommons[l].addr = (void *)lookup[l].offset;
	     ldrCommons[l].name = lookup[l].symName;
	     ldrCommons[l].namelen = strlen(lookup[l].symName);
	}
	listCOMMON=common->next;
	offset+=common->sym->st_size;
#if defined(__alpha__)
	offset = (offset+7)&~0x7;  
#endif
	xf86loaderfree(common);
	l++;
    }
    /* listCOMMON == 0 */
    lookup[l].symName=NULL; /* Terminate the list. */
    return lookup;
}


/*
 * String Table
 */
static char *
ElfGetStringIndex(file, offset, index)
ELFModulePtr	file;
int offset;
int index;
{
    if( !offset || !index )
        return "";

    return (char *)(file->saddr[index]+offset);
}

static char *
ElfGetString(file, offset)
ELFModulePtr	file;
int offset;
{
    return ElfGetStringIndex( file, offset, file->strndx );
}

static char *
ElfGetSectionName(file, offset)
ELFModulePtr	file;
int offset;
{
    return (char *)(file->shstraddr+offset);
}



/*
 * Symbol Table
 */

/*
 * Get symbol name
 */
static char *
ElfGetSymbolNameIndex(elffile, index, secndx)
ELFModulePtr	elffile;
int index;
int secndx;
{
    Elf_Sym	*syms;

#ifdef ELFDEBUG
    ELFDEBUG("ElfGetSymbolNameIndex(%x,%x) ",index, secndx );
#endif

    syms=(Elf_Sym *)elffile->saddr[secndx];

#ifdef ELFDEBUG
    ELFDEBUG("%s ",ElfGetString(elffile, syms[index].st_name));
    ELFDEBUG("%x %x ",ELF_ST_BIND(syms[index].st_info),
	     ELF_ST_TYPE(syms[index].st_info));
    ELFDEBUG("%lx\n",syms[index].st_value);
#endif

    return ElfGetString(elffile,syms[index].st_name );
}

static char *
ElfGetSymbolName(elffile, index)
ELFModulePtr	elffile;
int index;
{
    char	*name,*symname;
    symname=ElfGetSymbolNameIndex( elffile, index, elffile->symndx );
    if( symname == NULL )
	return NULL;
   
    name=xf86loadermalloc(strlen(symname)+1);
    if (!name)
	FatalError("ELFGetSymbolName: Out of memory\n");

    strcpy(name,symname);

    return name;
}

static Elf_Addr
ElfGetSymbolValue(elffile, index)
ELFModulePtr	elffile;
int index;
{
    Elf_Sym	*syms;
    Elf_Addr symval=0;	/* value of the indicated symbol */
    char *symname = NULL;		/* name of symbol in relocation */
    itemPtr symbol = NULL;		/* name/value of symbol */

    syms=(Elf_Sym *)elffile->saddr[elffile->symndx];

    switch( ELF_ST_TYPE(syms[index].st_info) )
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	case STT_FUNC:
	    switch( ELF_ST_BIND(syms[index].st_info) )
		{
		case STB_LOCAL:
		    symval=(Elf_Addr)(
					elffile->saddr[syms[index].st_shndx]+
					syms[index].st_value);
		    break;
		case STB_GLOBAL:
		case STB_WEAK: /* STB_WEAK seems like a hack to cover for
					some other problem */
		    symname=
			ElfGetString(elffile,syms[index].st_name);
		    symbol = LoaderHashFind(symname);
		    if( symbol == 0 ) {
			return 0;
			}
		    symval=(Elf_Addr)symbol->address;
		    break;
		default:
		    symval=0;
		    ErrorF(
			   "ElfGetSymbolValue(), unhandled symbol scope %x\n",
			   ELF_ST_BIND(syms[index].st_info) );
		    break;
		}
#ifdef ELFDEBUG
	    ELFDEBUG( "%x\t", symbol );
	    ELFDEBUG( "%lx\t", symval );
	    ELFDEBUG( "%s\n", symname ? symname : "NULL");
#endif
	    break;
	case STT_SECTION:
	    symval=(Elf_Addr)elffile->saddr[syms[index].st_shndx];
#ifdef ELFDEBUG
	    ELFDEBUG( "ST_SECTION %lx\n", symval );
#endif
	    break;
	case STT_FILE:
	case STT_LOPROC:
	case STT_HIPROC:
	default:
	    symval=0;
	    ErrorF( "ElfGetSymbolValue(), unhandled symbol type %x\n",
		    ELF_ST_TYPE(syms[index].st_info) );
	    break;
	}
    return symval;
}

#if defined(__powerpc__)
/*
 * This function returns the address of a pseudo PLT routine which can
 * be used to compute a function offset. This is needed because loaded
 * modules have an offset from the .text section of greater than 24 bits.
 * The code generated makes the assumption that all function entry points
 * will be within a 24 bit offset (non-PIC code).
 */
static Elf_Addr
ElfGetPltAddr(elffile, index)
ELFModulePtr	elffile;
int index;
{
    Elf_Sym	*syms;
    Elf_Addr symval=0;	/* value of the indicated symbol */
    char *symname = NULL;	/* name of symbol in relocation */
    itemPtr symbol;		/* name/value of symbol */

    syms=(Elf_Sym *)elffile->saddr[elffile->symndx];

    switch( ELF_ST_TYPE(syms[index].st_info) )
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	case STT_FUNC:
	    switch( ELF_ST_BIND(syms[index].st_info) )
		{
		case STB_GLOBAL:
		    symname=
			ElfGetString(elffile,syms[index].st_name);
		    symbol=LoaderHashFind(symname);
		    if( symbol == 0 )
			return 0;
/*
 * Here we are building up a pseudo Plt function that can make a call to
 * a function that has an offset greater than 24 bits. The following code
 * is being used to implement this.

     1  00000000                                .extern realfunc
     2  00000000                                .global pltfunc
     3  00000000                        pltfunc:
     4  00000000  3d 80 00 00                   lis     r12,hi16(realfunc)
     5  00000004  61 8c 00 00                   ori     r12,r12,lo16(realfunc)
     6  00000008  7d 89 03 a6                   mtctr   r12
     7  0000000c  4e 80 04 20                   bctr

 */

		    symbol->code.plt[0]=0x3d80; /* lis     r12 */
		    symbol->code.plt[1]=(((Elf_Addr)symbol->address)&0xffff0000)>>16;
		    symbol->code.plt[2]=0x618c; /* ori     r12,r12 */
		    symbol->code.plt[3]=(((Elf_Addr)symbol->address)&0xffff);
		    symbol->code.plt[4]=0x7d89; /* mtcr    r12 */
		    symbol->code.plt[5]=0x03a6;
		    symbol->code.plt[6]=0x4e80; /* bctr */
		    symbol->code.plt[7]=0x0420;
		    symbol->address=(char *)&symbol->code.plt[0];
		    symval=(Elf_Addr)symbol->address;
		    ppc_flush_icache(&symbol->code.plt[0]);
		    ppc_flush_icache(&symbol->code.plt[6]);
		    break;
		default:
		    symval=0;
		    ErrorF(
			   "ElfGetPltAddr(), unhandled symbol scope %x\n",
			   ELF_ST_BIND(syms[index].st_info) );
		    break;
		}
#ifdef ELFDEBUG
	    ELFDEBUG( "ElfGetPlt: symbol=%x\t", symbol );
	    ELFDEBUG( "newval=%x\t", symval );
	    ELFDEBUG( "name=\"%s\"\n", symname ? symname : "NULL");
#endif
	    break;
	case STT_SECTION:
	case STT_FILE:
	case STT_LOPROC:
	case STT_HIPROC:
	default:
	    symval=0;
	    ErrorF( "ElfGetPltAddr(), Unexpected symbol type %x",
		    ELF_ST_TYPE(syms[index].st_info) );
	    ErrorF( "for a Plt request\n" );
	    break;
	}
    return symval;
}
#endif /* __powerpc__ */

#if defined(__alpha__)
/*
 * Manage GOT Entries
 */
static void
ElfAddGOT(elffile,rel)
ELFModulePtr	elffile;
Elf_Rela	*rel;
{
    ELFGotPtr gotent;

#ifdef ELFDEBUG
    {
    Elf_Sym *sym;
    char *namestr;

    sym=(Elf_Sym *)&(elffile->symtab[ELF_R_SYM(rel->r_info)]);
    if( sym->st_name) {
	ELFDEBUG("ElfAddGOT: Adding GOT entry for %s\n", 
	    namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	xf86loaderfree(namestr);
	}
    else
	ELFDEBUG("ElfAddGOT: Adding GOT entry for %s\n", 
	   ElfGetSectionName(elffile,elffile->sections[sym->st_shndx].sh_name));
    }
#endif

    for (gotent=elffile->got_entries;gotent;gotent=gotent->next) {
	if ( ELF_R_SYM(gotent->rel->r_info) == ELF_R_SYM(rel->r_info) &&
	     gotent->rel->r_addend == rel->r_addend )
		break;
    }

    if( gotent ) {
#ifdef ELFDEBUG
	ELFDEBUG("Entry already present in GOT\n");
#endif
	return;
    }

    if ((gotent = xf86loadermalloc(sizeof(ELFGotRec))) == NULL) {
	ErrorF( "ElfAddGOT() Unable to allocate memory!!!!\n" );
	return;
    }

#ifdef ELFDEBUG
    ELFDEBUG("Entry added with offset %x\n",elffile->gotsize);
#endif
    gotent->rel=rel;
    gotent->offset=elffile->gotsize;
    gotent->next=elffile->got_entries;
    elffile->got_entries=gotent;
    elffile->gotsize+=8;
    return ;
}

void
ELFCreateGOT(elffile)
ELFModulePtr	elffile;
{
#ifdef ELFDEBUG
    ELFDEBUG( "ELFCreateGOT: %x entries in the GOT\n", elffile->gotsize/8 );
#endif

#if ELFDEBUG
    /*
     * Hmmm. Someone is getting here without any got entries, but they
     * may still have R_ALPHA_GPDISP relocations against the got.
     */
    if( elffile->gotsize == 0 ) 
	ELFDEBUG( "Module %s doesn't have any GOT entries!\n",
		_LoaderModuleToName(elffile->module) );
#endif
    if( elffile->gotsize == 0 ) elffile->gotsize=8;

    if ((elffile->got = xf86loadermalloc(elffile->gotsize)) == NULL) {
	ErrorF( "ELFCreateGOT() Unable to allocate memory!!!!\n" );
	return;
    }
    elffile->sections[elffile->gotndx].sh_size=elffile->gotsize;
#ifdef ELFDEBUG
    ELFDEBUG( "ELFCreateGOT: GOT address %lx\n", elffile->got );
#endif

    return;
}
#endif

/*
 * Fix all of the relocations for the given section.
 * If the argument 'force' is non-zero, then the relocation will be
 * made even if the symbol can't be found (by substituting
 * LoaderDefaultFunc) otherwise, the relocation will be deferred.
 */
static ELFRelocPtr
Elf_RelocateEntry(elffile, secp, rel, force)
ELFModulePtr	elffile;
unsigned char *secp;	/* Begining of the target section */
#if defined(i386)
Elf_Rel	*rel;
#endif
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
Elf_Rela	*rel;
#endif
int		force;
{
    unsigned int *dest32;	/* address of the 32 bit place being modified */
#if defined(__powerpc__) || defined(__mc68000__) || defined(__sparc__)
    unsigned short *dest16;	/* address of the 16 bit place being modified */
#endif
#if defined(__sparc__)
    unsigned char *dest8;	/* address of the 8 bit place being modified */
#endif
#if defined(__alpha__)
    unsigned int *dest32h;	/* address of the high 32 bit place being modified */
    unsigned long *dest64;
#if 0				/* XXX unused */
    unsigned long *gp=(unsigned long *)elffile->got+0x8000;	/*
								 * location of the got table */
#endif
#endif
    Elf_Addr symval;	/* value of the indicated symbol */

#ifdef ELFDEBUG
#if defined(i386)
    ELFDEBUG( "%lx %d %d\n", rel->r_offset,
	      ELF_R_SYM(rel->r_info),ELF_R_TYPE(rel->r_info) );
#endif
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
    ELFDEBUG( "%x %d %d %x\n", rel->r_offset,
	      ELF_R_SYM(rel->r_info),ELF_R_TYPE(rel->r_info),
	      rel->r_addend );
#endif
#endif
#if defined(__alpha__)
    if (ELF_R_SYM(rel->r_info) && ELF_R_TYPE(rel->r_info) != R_ALPHA_GPDISP) {
#else
    if (ELF_R_SYM(rel->r_info)) {
#endif
	symval = ElfGetSymbolValue(elffile, ELF_R_SYM(rel->r_info));
	if (symval == 0) {
	    if (force) {
		symval = (Elf_Addr) &LoaderDefaultFunc;
	    } else {
#ifdef ELFDEBUG
		char *namestr;
		namestr = ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info));
		ELFDEBUG("***Unable to resolve symbol %s\n", namestr);
		xf86loaderfree(namestr);
#endif
		return ElfDelayRelocation(elffile,secp,rel);
	    }
	}
    }

    switch( ELF_R_TYPE(rel->r_info) )
	{
#if defined(i386)
	case R_386_32:
	    dest32=(unsigned int *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_386_32\t");
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8lx\t", *dest32 );
#endif
	    *dest32=symval+(*dest32); /* S + A */
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8lx\n", *dest32 );
#endif
	    break;
	case R_386_PC32:
	    dest32=(unsigned int *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    {
	    char *namestr;
	    ELFDEBUG( "R_386_PC32 %s\t",
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    xf86loaderfree(namestr);
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8lx\t", *dest32 );
	    }
#endif

	    *dest32=symval+(*dest32)-(Elf_Addr)dest32; /* S + A - P */

#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8lx\n", *dest32 );
#endif

		break;
#endif /* i386 */
#if defined(__alpha__)
	case R_ALPHA_NONE:
	case R_ALPHA_LITUSE:
	  break;
	  
	case R_ALPHA_REFQUAD:
	    dest64=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,
				     ELF_R_SYM(rel->r_info));
#ifdef ELFDEBUG
	    ELFDEBUG( "R_ALPHA_REFQUAD\t");
	    ELFDEBUG( "dest64=%lx\t", dest64 );
	    ELFDEBUG( "*dest64=%8.8lx\t", *dest64 );
#endif
	    *dest64=symval+rel->r_addend+(*dest64); /* S + A + P */
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest64=%8.8lx\n", *dest64 );
#endif
	  break;
	  
	case R_ALPHA_GPREL32:
	    {
	    dest64=(unsigned long *)(secp+rel->r_offset);
	    dest32=(unsigned int *)dest64;

#ifdef ELFDEBUG
	    {
	    char *namestr;
	    ELFDEBUG( "R_ALPHA_GPREL32 %s\t", 
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    xf86loaderfree(namestr);
	    ELFDEBUG( "secp=%lx\t", secp );
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "dest32=%lx\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    }
#endif
	    symval += rel->r_addend;
	    symval = ((unsigned char *)symval)-((unsigned char *)elffile->got);
#ifdef ELFDEBUG
	    ELFDEBUG( "symval=%lx\t", symval );
#endif
	    if( (symval&0xffffffff00000000) != 0x0000000000000000 &&
	        (symval&0xffffffff00000000) != 0xffffffff00000000 ) {
		FatalError("R_ALPHA_GPREL32 symval-got is too large for %s\n",
			ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)));
	    }

	    *dest32=symval;
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%x\n", *dest32 );
#endif
	    break;
	    }
	case R_ALPHA_LITERAL:
	    {
	    ELFGotPtr gotent;
	    dest32=(unsigned int *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    {
	    char *namestr;
	    ELFDEBUG( "R_ALPHA_LITERAL %s\t", 
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    xf86loaderfree(namestr);
	    ELFDEBUG( "secp=%lx\t", secp );
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "dest32=%lx\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    }
#endif

	    for (gotent=elffile->got_entries;gotent;gotent=gotent->next) {
		if ( ELF_R_SYM(gotent->rel->r_info) == ELF_R_SYM(rel->r_info) &&
	     	gotent->rel->r_addend == rel->r_addend )
			break;
	    }

	    /* Set the address in the GOT */
	    if( gotent ) {
		*(unsigned long *)(elffile->got+gotent->offset) =
							symval+rel->r_addend;
#ifdef ELFDEBUG
		ELFDEBUG("Setting gotent[%x]=%lx\t",
				gotent->offset, symval+rel->r_addend);
#endif
		if ((gotent->offset & 0xffff0000) != 0)
		    FatalError("\nR_ALPHA_LITERAL offset %x too large\n",
			       gotent->offset);
		(*dest32)|=(gotent->offset);	/* The address part is always 0 */
		}
	    else	{
		unsigned long val;
		
		/* S + A - P >> 2 */
		val=((symval+(rel->r_addend)-(Elf_Addr)dest32));
#ifdef ELFDEBUG
		ELFDEBUG("S+A-P=%x\t",val);
#endif
		if( (val & 0xffff0000) != 0xffff0000 &&
		    (val & 0xffff0000) != 0x00000000 )  {
		    ErrorF("\nR_ALPHA_LITERAL offset %x too large\n", val);
			break;
		}
		val &= 0x0000ffff;
		(*dest32)|=(val);	/* The address part is always 0 */
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif

	    break;
	    }
	  
	case R_ALPHA_GPDISP:
	    {
	    long offset;

	    dest32h=(unsigned int *)(secp+rel->r_offset);
	    dest32=(unsigned int *)((secp+rel->r_offset)+rel->r_addend);

#ifdef ELFDEBUG
	    {
	    char *namestr;
	    ELFDEBUG( "R_ALPHA_GPDISP %s\t", 
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    xf86loaderfree(namestr);
	    ELFDEBUG( "secp=%lx\t", secp );
	    ELFDEBUG( "got=%lx\t", elffile->got );
	    ELFDEBUG( "gp=%lx\t", gp );
	    ELFDEBUG( "dest32=%lx\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    ELFDEBUG( "dest32h=%lx\t", dest32h );
	    ELFDEBUG( "*dest32h=%8.8x\t", *dest32h );
	    }
#endif
	    if ((*dest32h >> 26) != 9 || (*dest32 >> 26) != 8) {
	        char *namestr;
	        ErrorF( "***Bad instructions in relocating %s\n",
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	        xf86loaderfree(namestr);
	    }

	    symval = (*dest32h & 0xffff) << 16 | (*dest32 & 0xffff);
	    symval = (symval ^ 0x80008000) - 0x80008000;
	    
	    offset = ((unsigned char *)elffile->got - (unsigned char *)dest32h);
#ifdef ELFDEBUG
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "got-dest32=%lx\t", offset );
#endif

	    if( (offset >= 0x7fff8000L) || (offset < -0x80000000L) ) {
		FatalError( "Offset overflow for R_ALPHA_GPDISP\n");
	    }

	    symval += (unsigned long)offset;
#ifdef ELFDEBUG
	    ELFDEBUG( "symval=%lx\t", symval );
#endif
	    *dest32=(*dest32&0xffff0000) | (symval&0xffff);
	    *dest32h=(*dest32h&0xffff0000)|
			(((symval>>16)+((symval>>15)&1))&0xffff);
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    ELFDEBUG( "*dest32h=%8.8x\n", *dest32h );
#endif
	  break;
	  }
	  
	case R_ALPHA_HINT:
	    dest32=(unsigned int *)((secp+rel->r_offset)+rel->r_addend);
#ifdef ELFDEBUG
	    {
	    char *namestr;
	    ELFDEBUG( "R_ALPHA_HINT %s\t", 
			namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    xf86loaderfree(namestr);
	    ELFDEBUG( "secp=%lx\t", secp );
	    ELFDEBUG( "symval=%lx\t", symval );
	    ELFDEBUG( "dest32=%lx\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    }
#endif

#ifdef ELFDEBUG
	    ELFDEBUG( "symval=%lx\t", symval );
#endif
	    symval -= (Elf_Addr)(((unsigned char *)dest32)+4);
	    if (symval % 4 ) {
		ErrorF( "R_ALPHA_HINT bad alignment of offset\n");
		}
	    symval=symval>>2;

#ifdef ELFDEBUG
	    ELFDEBUG( "symval=%lx\t", symval );
#endif

	    if( symval & 0xffff8000 ) {
#ifdef ELFDEBUG
		ELFDEBUG("R_ALPHA_HINT symval too large\n" );
#endif
	    }

	    *dest32 = (*dest32&~0x3fff) | (symval&0x3fff);

#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	  break;
	  
#endif /* alpha */
#if defined(__mc68000__)
	case R_68K_32:
		dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
ELFDEBUG( "R_68K_32\t", dest32 );
ELFDEBUG( "dest32=%x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		*dest32=symval+(*dest32); /* S + A */
#ifdef ELFDEBUG
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		break;
	case R_68K_PC32:
		dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
char *namestr;
ELFDEBUG( "R_68K_PC32 %s\t",
		  namestr=ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
xf86loaderfree(namestr);
ELFDEBUG( "secp=%x\t", secp );
ELFDEBUG( "symval=%x\t", symval );
ELFDEBUG( "dest32=%x\t", dest32 );
ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif

		*dest32=symval+(*dest32)-(Elf_Addr)dest32; /* S + A - P */

#ifdef ELFDEBUG
ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif

		break;
#endif /* __mc68000__ */
#if defined(__powerpc__)
#if defined(PowerMAX_OS)
	case R_PPC_DISP24: /* 11 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_DISP24 %s\t", ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif

	    {
		unsigned long val;
		
		/* S + A - P >> 2 */
		val=((symval+(rel->r_addend)-(Elf_Addr)dest32));
#ifdef ELFDEBUG
		ELFDEBUG("S+A-P=%x\t",val);
#endif
		val = val>>2;
		if( (val & 0x3f000000) != 0x3f000000 &&
		    (val & 0x3f000000) != 0x00000000 )  {
#ifdef ELFDEBUG
		    ELFDEBUG("R_PPC_DISP24 offset %x too large\n", val<<2);
#endif
		    symval = ElfGetPltAddr(elffile,ELF_R_SYM(rel->r_info));
		    val=((symval+(rel->r_addend)-(Elf_Addr)dest32));
#ifdef ELFDEBUG
		    ELFDEBUG("PLT offset is %x\n", val);
#endif
		    val=val>>2;
		    if( (val & 0x3f000000) != 0x3f000000 &&
		         (val & 0x3f000000) != 0x00000000 )
			   FatalError("R_PPC_DISP24 PLT offset %x too large\n", val<<2);
		}
		val &= 0x00ffffff;
		(*dest32)|=(val<<2);	/* The address part is always 0 */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16HU: /* 31 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);

#endif
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16HU\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		/* S + A */
		val=((symval+(rel->r_addend))&0xffff0000)>>16;
#ifdef ELFDEBUG
		ELFDEBUG("uhi16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_32: /* 32 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_32\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		/* S + A */
		val=symval+(rel->r_addend);
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
#endif
		*dest32=val; /* S + A */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_32UA: /* 33 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_32UA\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		unsigned char *dest8 = (unsigned char *)dest32;
		/* S + A */
		val=symval+(rel->r_addend);
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
#endif
		*dest8++=(val&0xff000000)>>24;
		*dest8++=(val&0x00ff0000)>>16;
		*dest8++=(val&0x0000ff00)>> 8;
		*dest8++=(val&0x000000ff);
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16H: /* 34 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16H\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symbol=%s\t", ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		unsigned short loval;
		/* S + A */
		val=((symval+(rel->r_addend))&0xffff0000)>>16;
		loval=(symval+(rel->r_addend))&0xffff;
		if( loval & 0x8000 ) {
		    /*
		     * This is hi16(), instead of uhi16(). Because of this,
		     * if the lo16() will produce a negative offset, then
		     * we have to increment this part of the address to get
		     * the correct final result.
		     */
		    val++;
		}
#ifdef ELFDEBUG
		ELFDEBUG("hi16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_16L: /* 35 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_16L\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned short val;
		/* S + A */
		val=(symval+(rel->r_addend))&0xffff;
#ifdef ELFDEBUG
		ELFDEBUG("lo16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
#else
 /* Linux PPC */
	case R_PPC_ADDR32: /* 1 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
	    symval=ElfGetSymbolValue(elffile,ELF_R_SYM(rel->r_info));
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_ADDR32\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		/* S + A */
		val=symval+(rel->r_addend);
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
#endif
		*dest32=val; /* S + A */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_ADDR16_LO: /* 4 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_ADDR16_LO\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
#endif
	    {
		unsigned short val;
		/* S + A */
		val=(symval+(rel->r_addend))&0xffff;
#ifdef ELFDEBUG
		ELFDEBUG("lo16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_ADDR16_HA: /* 6 */
	    dest16=(unsigned short *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    dest32=(unsigned long *)(dest16-1);
#endif
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_ADDR16_HA\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest16=%x\t", dest16 );
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
#endif
	    {
		unsigned short val;
		unsigned short loval;
		/* S + A */
		val=((symval+(rel->r_addend))&0xffff0000)>>16;
		loval=(symval+(rel->r_addend))&0xffff;
		if( loval & 0x8000 ) {
		    /*
		     * This is hi16(), instead of uhi16(). Because of this,
		     * if the lo16() will produce a negative offset, then
		     * we have to increment this part of the address to get
		     * the correct final result.
		     */
		    val++;
		}
#ifdef ELFDEBUG
		ELFDEBUG("hi16(S+A)=%x\t",val);
#endif
		*dest16=val; /* S + A */
		ppc_flush_icache(dest16);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_REL24: /* 10 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_REL24 %s\t", ElfGetSymbolName(elffile,ELF_R_SYM(rel->r_info)) );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "dest32=%x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif

	    {
		unsigned long val;
		
		/* S + A - P >> 2 */
		val=((symval+(rel->r_addend)-(Elf_Addr)dest32));
#ifdef ELFDEBUG
		ELFDEBUG("S+A-P=%x\t",val);
#endif
		val = val>>2;
		if( (val & 0x3f000000) != 0x3f000000 &&
		    (val & 0x3f000000) != 0x00000000 )  {
#ifdef ELFDEBUG
		    ELFDEBUG("R_PPC_REL24 offset %x too large\n", val<<2);
#endif
		    symval = ElfGetPltAddr(elffile,ELF_R_SYM(rel->r_info));
		    val=((symval+(rel->r_addend)-(Elf_Addr)dest32));
#ifdef ELFDEBUG
		    ELFDEBUG("PLT offset is %x\n", val);
#endif
		    val=val>>2;
		    if( (val & 0x3f000000) != 0x3f000000 &&
		         (val & 0x3f000000) != 0x00000000 )
			   FatalError("R_PPC_REL24 PLT offset %x too large\n", val<<2);
		}
		val &= 0x00ffffff;
		(*dest32)|=(val<<2);	/* The address part is always 0 */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PPC_REL32: /* 26 */
	    dest32=(unsigned long *)(secp+rel->r_offset);
#ifdef ELFDEBUG
	    ELFDEBUG( "R_PPC_REL32\t" );
	    ELFDEBUG( "secp=%x\t", secp );
	    ELFDEBUG( "symval=%x\t", symval );
	    ELFDEBUG( "r_addend=%x\t", rel->r_addend );
	    ELFDEBUG( "dest32=%8.8x\t", dest32 );
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    {
		unsigned long val;
		/* S + A - P */
		val=symval+(rel->r_addend);
		val-=*dest32;
#ifdef ELFDEBUG
		ELFDEBUG("S+A=%x\t",val);
		ELFDEBUG("S+A-P=%x\t",val+(*dest32)-(Elf_Addr)dest32);
#endif
	        *dest32=val+(*dest32)-(Elf_Addr)dest32; /* S + A - P */
		ppc_flush_icache(dest32);
	    }
#ifdef ELFDEBUG
	    ELFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
#endif /* PowerMAX_OS */
#endif /* __powerpc__ */
#ifdef __sparc__
	case R_SPARC_RELATIVE:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		*dest32 += (unsigned int)secp + rel->r_addend;
		break;

	case R_SPARC_GLOB_DAT:
	case R_SPARC_32:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest32 = symval;
		break;

	case R_SPARC_JMP_SLOT:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		/* Before we change it the PLT entry looks like:
		 *
		 * pltent:	sethi	%hi(rela_plt_offset), %g1
		 *		b,a	PLT0
		 *		nop
		 *
		 * We change it into:
		 *
		 * pltent:	sethi	%hi(rela_plt_offset), %g1
		 *		sethi	%hi(symval), %g1
		 *		jmp	%g1 + %lo(symval), %g0
		 */
		symval += rel->r_addend;
		dest32[2] = 0x81c06000 | (symval & 0x3ff);
		__asm __volatile("flush %0 + 0x8" : : "r" (dest32));
		dest32[1] = 0x03000000 | (symval >> 10);
		__asm __volatile("flush %0 + 0x4" : : "r" (dest32));
		break;

	case R_SPARC_8:
		dest8 = (unsigned char *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest8 = symval;
		break;

	case R_SPARC_16:
		dest16 = (unsigned short *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest16 = symval;
		break;

	case R_SPARC_DISP8:
		dest8 = (unsigned char *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest8 = (symval - (Elf32_Addr) dest8);
		break;

	case R_SPARC_DISP16:
		dest16 = (unsigned short *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest16 = (symval - (Elf32_Addr) dest16);
		break;

	case R_SPARC_DISP32:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest32 = (symval - (Elf32_Addr) dest32);
		break;

	case R_SPARC_LO10:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest32 = (*dest32 & ~0x3ff) | (symval & 0x3ff);
		break;

	case R_SPARC_WDISP30:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest32 = ((*dest32 & 0xc0000000) |
			   ((symval - (Elf32_Addr) dest32) >> 2));
		break;

	case R_SPARC_HI22:
		dest32 = (unsigned int *)(secp + rel->r_offset);
		symval += rel->r_addend;
		*dest32 = (*dest32 & 0xffc00000) | (symval >> 10);
		break;

	case R_SPARC_NONE:
		break;

	case R_SPARC_COPY:
		/* Fix your code...  I'd rather dish out an error here
		 * so people will not link together PIC and non-PIC
		 * code into a final driver object file.
		 */
		ErrorF("Elf_RelocateEntry()  Copy relocs not supported on Sparc.\n");
		break;
#endif
	default:
	    ErrorF(
		   "Elf_RelocateEntry() Unsupported relocation type %d\n",
		   ELF_R_TYPE(rel->r_info) );
	    break;
	}
    return 0;
}

static ELFRelocPtr
ELFCollectRelocations(elffile, index)
ELFModulePtr	elffile;
int	index; /* The section to use as relocation data */
{
    int	i, numrel;
    Elf_Shdr	*sect=&(elffile->sections[index]);
#if defined(i386)
    Elf_Rel	*rel=(Elf_Rel *)elffile->saddr[index];
#endif
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
    Elf_Rela	*rel=(Elf_Rela *)elffile->saddr[index];
#endif
    Elf_Sym	*syms;
    unsigned char *secp;	/* Begining of the target section */
    ELFRelocPtr reloc_head = NULL;
    ELFRelocPtr tmp;

    secp=(unsigned char *)elffile->saddr[sect->sh_info];
    syms = (Elf_Sym *) elffile->saddr[elffile->symndx];

    numrel=sect->sh_size/sect->sh_entsize;

    for(i=0; i<numrel; i++ ) {
#if defined(__alpha__)
	if( ELF_R_TYPE(rel[i].r_info) == R_ALPHA_LITERAL) {
	    ElfAddGOT(elffile,&rel[i]);
	    }   
#endif
	tmp = ElfDelayRelocation(elffile,secp,&(rel[i]));
	tmp->next = reloc_head;
	reloc_head = tmp;
    }

    return reloc_head;
}

/*
 * ELF_GetSymbols()
 *
 * add the symbols to the symbol table maintained by the loader.
 */

static LOOKUP *
ELF_GetSymbols(elffile)
ELFModulePtr	elffile;
{
    Elf_Sym	*syms;
    Elf_Shdr	*sect;
    int		i, l, numsyms;
    LOOKUP	*lookup, *lookup_common, *p;
    ELFCommonPtr tmp;

    syms=elffile->symtab;
    sect=&(elffile->sections[elffile->symndx]);
    numsyms=sect->sh_size/sect->sh_entsize;

    if ((lookup = xf86loadermalloc((numsyms+1)*sizeof(LOOKUP))) == NULL)
	return 0;

    for(i=0,l=0; i<numsyms; i++)
	{
#ifdef ELFDEBUG
	    ELFDEBUG("value=%lx\tsize=%lx\tBIND=%x\tTYPE=%x\tndx=%x\t%s\n",
		     syms[i].st_value,syms[i].st_size,
		     ELF_ST_BIND(syms[i].st_info),ELF_ST_TYPE(syms[i].st_info),
		     syms[i].st_shndx,ElfGetString(elffile,syms[i].st_name) );
#endif

	    if( ELF_ST_BIND(syms[i].st_info) == STB_LOCAL )
		/* Don't add static symbols to the symbol table */
		continue;

	    switch( ELF_ST_TYPE(syms[i].st_info) )
		{
		case STT_OBJECT:
		case STT_FUNC:
		case STT_SECTION:
		case STT_NOTYPE:
		    switch(syms[i].st_shndx)
			{
			case SHN_ABS:
			    ErrorF("ELF_GetSymbols() Don't know how to handle SHN_ABS\n" );
			    break;
			case SHN_COMMON:
#ifdef ELFDEBUG
			    ELFDEBUG("Adding COMMON space for %s\n",
				     ElfGetString(elffile,syms[i].st_name) );
#endif
			    if (!LoaderHashFind(ElfGetString(elffile,
							syms[i].st_name))) {
				tmp = ElfAddCOMMON(&(syms[i]));
				if (tmp) {
				    tmp->next = listCOMMON;
				    listCOMMON = tmp;
				}
			    }			    
			    break;
			case SHN_UNDEF:
			    /*
			     * UNDEF will get resolved later, so the value
			     * doesn't really matter here.
			     */
			    /* since we don't know the value don't advertise the symbol */
			    break;
			default:
			    lookup[l].symName=xf86loaderstrdup(ElfGetString(elffile,syms[i].st_name));
			    lookup[l].offset=(funcptr)
					(elffile->saddr[syms[i].st_shndx]+
					syms[i].st_value);
#ifdef ELFDEBUG
			    ELFDEBUG("Adding symbol %lx %s\n",
				     lookup[l].offset, lookup[l].symName );
#endif
			    l++;
			    break;
			}
		    break;
		case STT_FILE:
		case STT_LOPROC:
		case STT_HIPROC:
		    /* Skip this type */
#ifdef ELFDEBUG
		    ELFDEBUG("Skipping TYPE %d %s\n",
			     ELF_ST_TYPE(syms[i].st_info),
			     ElfGetString(elffile,syms[i].st_name));
#endif
		    break;
		default:
		    ErrorF("ELF_GetSymbols(): Unepected symbol type %d\n",
			   ELF_ST_TYPE(syms[i].st_info) );
		    break;
		}
	}

    lookup[l].symName=NULL; /* Terminate the list */

    lookup_common = ElfCreateCOMMON(elffile);
    if (lookup_common) {
	for (i = 0, p = lookup_common; p->symName; i++, p++)
	    ;
	memcpy(&(lookup[l]), lookup_common, i * sizeof (LOOKUP));

	xf86loaderfree(lookup_common);
	l += i;
	lookup[l].symName = NULL;
    }


/*
 * Remove the ELF symbols that will show up in every object module.
 */
    for (i = 0, p = lookup; p->symName; i++, p++) {
	while (!strcmp(lookup[i].symName, ".text")
	       || !strcmp(lookup[i].symName, ".data")
	       || !strcmp(lookup[i].symName, ".bss")
	       || !strcmp(lookup[i].symName, ".comment")
	       || !strcmp(lookup[i].symName, ".note")
	       ) {
	    memmove(&(lookup[i]), &(lookup[i+1]), (l-- - i) * sizeof (LOOKUP));
	}
    }
    return lookup;
}

#define SecOffset(index) elffile->sections[index].sh_offset
#define SecSize(index) elffile->sections[index].sh_size

/*
 * ELFCollectSections
 *
 * Do the work required to load each section into memory.
 */
static void
ELFCollectSections(elffile)
ELFModulePtr	elffile;
{
    int	i;

/*
 * Find and identify all of the Sections
 */

#ifdef ELFDEBUG
    ELFDEBUG("%d sections\n", elffile->numsh );
#endif

    for( i=1; i<elffile->numsh; i++) {
#ifdef ELFDEBUG
	ELFDEBUG("%d %s\n", i, ElfGetSectionName(elffile, elffile->sections[i].sh_name) );
#endif
	/* .text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".text" ) == 0 ) {
	    elffile->text=_LoaderFileToMem(elffile->fd,
					   SecOffset(i),SecSize(i),".text");
	    elffile->saddr[i]=elffile->text;
	    elffile->txtndx=i;
	    elffile->txtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".text starts at %lx\n", elffile->text );
#endif
	    continue;
	}
	/* .data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".data" ) == 0 ) {
	    elffile->data=_LoaderFileToMem(elffile->fd,
					   SecOffset(i),SecSize(i),".data");
	    elffile->saddr[i]=elffile->data;
	    elffile->datndx=i;
	    elffile->datsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".data starts at %lx\n", elffile->data );
#endif
		continue;
		}
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
							".data1" ) == 0 ) {
		elffile->data1=_LoaderFileToMem(elffile->fd,
					SecOffset(i),SecSize(i),".data1");
		elffile->saddr[i]=elffile->data1;
		elffile->dat1ndx=i;
		elffile->dat1size=SecSize(i);
#ifdef ELFDEBUG
ELFDEBUG(".data1 starts at %lx\n", elffile->data1 );
#endif
		continue;
		}
	/* .sdata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".sdata" ) == 0 ) {
	    elffile->sdata=_LoaderFileToMem(elffile->fd,
					     SecOffset(i),SecSize(i),".sdata");
	    elffile->saddr[i]=elffile->sdata;
	    elffile->sdatndx=i;
	    elffile->sdatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".sdata starts at %lx\n", elffile->sdata );
#endif
		continue;
		}
	/* .bss */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".bss" ) == 0 ) {
	    if( SecSize(i) )
		elffile->bss = xf86loadercalloc(1, SecSize(i));
	    else
		elffile->bss=NULL;
	    elffile->saddr[i]=elffile->bss;
	    elffile->bssndx=i;
	    elffile->bsssize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".bss starts at %lx\n", elffile->bss );
#endif
	    continue;
	}
	/* .sbss */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".sbss" ) == 0 ) {
	    if( SecSize(i) )
		elffile->sbss = xf86loadercalloc(1, SecSize(i));
	    else
		elffile->sbss=NULL;
	    elffile->saddr[i]=elffile->sbss;
	    elffile->sbssndx=i;
	    elffile->sbsssize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".sbss starts at %lx\n", elffile->sbss );
#endif
	    continue;
	}
	/* .rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rodata" ) == 0 ) {
	    elffile->rodata=_LoaderFileToMem(elffile->fd,
					     SecOffset(i),SecSize(i),".rodata");
	    elffile->saddr[i]=elffile->rodata;
	    elffile->rodatndx=i;
	    elffile->rodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rodata starts at %lx\n", elffile->rodata );
#endif
		continue;
		}
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
							".rodata1" ) == 0 ) {
		elffile->rodata1=_LoaderFileToMem(elffile->fd,
					SecOffset(i),SecSize(i),".rodata1");
		elffile->saddr[i]=elffile->rodata1;
		elffile->rodat1ndx=i;
		elffile->rodat1size=SecSize(i);
#ifdef ELFDEBUG
ELFDEBUG(".rodata1 starts at %lx\n", elffile->rodata1 );
#endif
		continue;
		}
	/* .symtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".symtab" ) == 0 ) {
	    elffile->symtab=(Elf_Sym *)_LoaderFileToMem(elffile->fd,
							  SecOffset(i),SecSize(i),".symtab");
	    elffile->saddr[i]=(unsigned char *)elffile->symtab;
	    elffile->symndx=i;
	    elffile->symsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".symtab starts at %lx\n", elffile->symtab );
#endif
	    continue;
	}
	/* .strtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".strtab" ) == 0 ) {
	    elffile->straddr=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".strtab");
	    elffile->saddr[i]=(unsigned char *)elffile->straddr;
	    elffile->strndx=i;
	    elffile->strsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".strtab starts at %lx\n", elffile->straddr );
#endif
		continue;
		}
#if defined(i386) || defined(__alpha__)
	/* .rel.text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.text" ) == 0 ) {
	    elffile->reltext=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rel.text");
	    elffile->saddr[i]=(unsigned char *)elffile->reltext;
	    elffile->reltxtndx=i;
	    elffile->reltxtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.text starts at %lx\n", elffile->reltext );
#endif
	    continue;
	}
	/* .rel.data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.data" ) == 0 ) {
	    elffile->reldata=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rel.data");
	    elffile->saddr[i]=(unsigned char *)elffile->reldata;
	    elffile->reldatndx=i;
	    elffile->reldatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.data starts at %lx\n", elffile->reldata );
#endif
	    continue;
	}
	/* .rel.rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.rodata" ) == 0 ) {
	    elffile->relrodata=_LoaderFileToMem(elffile->fd,
						SecOffset(i),SecSize(i),".rel.rodata");
	    elffile->saddr[i]=(unsigned char *)elffile->relrodata;
	    elffile->relrodatndx=i;
	    elffile->relrodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rel.rodata starts at %lx\n", elffile->relrodata );
#endif
		continue;
		}
#endif /* i386/alpha */
#if defined(__powerpc__) || defined(__mc68000__) || defined(__alpha__) || defined(__sparc__)
	/* .rela.text */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.text" ) == 0 ) {
	    elffile->reltext=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rela.text");
	    elffile->saddr[i]=(unsigned char *)elffile->reltext;
	    elffile->reltxtndx=i;
	    elffile->reltxtsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.text starts at %x\n", elffile->reltext );
#endif
	    continue;
	}
	/* .rela.data */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.data" ) == 0 ) {
	    elffile->reldata=_LoaderFileToMem(elffile->fd,
					      SecOffset(i),SecSize(i),".rela.data");
	    elffile->saddr[i]=(unsigned char *)elffile->reldata;
	    elffile->reldatndx=i;
	    elffile->reldatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.data starts at %x\n", elffile->reldata );
#endif
	    continue;
	}
	/* .rela.sdata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.sdata" ) == 0 ) {
	    elffile->relsdata=_LoaderFileToMem(elffile->fd,
						SecOffset(i),SecSize(i),".rela.sdata");
	    elffile->saddr[i]=(unsigned char *)elffile->relsdata;
	    elffile->relsdatndx=i;
	    elffile->relsdatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.sdata starts at %x\n", elffile->relsdata );
#endif
	    continue;
	}
	/* .rela.rodata */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.rodata" ) == 0 ) {
	    elffile->relrodata=_LoaderFileToMem(elffile->fd,
						SecOffset(i),SecSize(i),".rela.rodata");
	    elffile->saddr[i]=(unsigned char *)elffile->relrodata;
	    elffile->relrodatndx=i;
	    elffile->relrodatsize=SecSize(i);
#ifdef ELFDEBUG
	    ELFDEBUG(".rela.rodata starts at %x\n", elffile->relrodata );
#endif
	    continue;
	}
#endif /* __powerpc__ || __mc68000__ */
	/* .shstrtab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".shstrtab" ) == 0 ) {
	    continue;
	}
	/* .comment */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".comment" ) == 0 ) {
	    continue;
	}
	/* .debug */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".debug" ) == 0 ) {
	    continue;
	}
	/* .line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".line" ) == 0 ) {
	    continue;
	}
	/* .note */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".note" ) == 0 ) {
	    continue;
	}
	/* .rel.debug */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.debug" ) == 0 ) {
	    continue;
	}
	/* .rel.line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.line" ) == 0 ) {
	    continue;
	}
	/* .stab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".stab" ) == 0 ) {
	    continue;
	}
	/* .rel.stab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rel.stab" ) == 0 ) {
	    continue;
	}
	/* .rela.stab */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.stab" ) == 0 ) {
	    continue;
	}
	/* .stabstr */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".stabstr" ) == 0 ) {
	    continue;
	}
#if defined(__powerpc__) || defined(__mc68000__)
	/* .rela.tdesc */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.tdesc" ) == 0 ) {
	    continue;
	}
	/* .rela.debug_line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".rela.debug_line" ) == 0 ) {
	    continue;
	}
	/* .tdesc */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".tdesc" ) == 0 ) {
	    continue;
	}
	/* .debug_line */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".debug_line" ) == 0 ) {
	    continue;
	}
	/* $0001300 */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   "$0001300" ) == 0 ) {
	    continue;
	}
#endif
#if defined(__alpha__)
	/* .got */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".got" ) == 0 ) {
	    continue;
	elffile->got=NULL;
	elffile->gotsize=0;
	elffile->got_entries=NULL;
	}
	/* .mdebug */
	if( strcmp(ElfGetSectionName(elffile, elffile->sections[i].sh_name),
		   ".mdebug" ) == 0 ) {
	    continue;
	}
#endif
	ErrorF("Not loading %s\n",
	       ElfGetSectionName(elffile, elffile->sections[i].sh_name) );
    }
}

/*
 * Public API for the ELF implementation of the loader.
 */
void *
ELFLoadModule(modrec, elffd, ppLookup)
loaderPtr	modrec;
int	elffd;
LOOKUP **ppLookup;
{
    ELFModulePtr elffile;
    Elf_Ehdr   *header;
    ELFRelocPtr  elf_reloc, tail;
    void	*v;
    LDRModulePtr elfmod;

    ldrCommons = 0;
    nCommons = 0;

    if ((elffile = xf86loadercalloc(1, sizeof(ELFModuleRec))) == NULL) {
	ErrorF( "Unable to allocate ELFModuleRec\n" );
	return NULL;
    }

    elffile->handle=modrec->handle;
    elffile->module=modrec->module;
    elffile->fd=elffd;
    v=elffile->funcs=modrec->funcs;

/*
 *  Get the ELF header
 */
    elffile->header=(Elf_Ehdr*)_LoaderFileToMem(elffd,0,sizeof(Elf_Ehdr),"header");
    header=(Elf_Ehdr *)elffile->header;

/*
 * Get the section table
 */
    elffile->numsh=header->e_shnum;
    elffile->secsize=(header->e_shentsize*header->e_shnum);
    elffile->sections=(Elf_Shdr *)_LoaderFileToMem(elffd,header->e_shoff,
						     elffile->secsize, "sections");
#if defined(__alpha__)
    /*
     * Need to allocate space for the .got section which will be
     * fabricated later
     */
    elffile->gotndx=header->e_shnum;
    header->e_shnum++;
    elffile->numsh=header->e_shnum;
    elffile->secsize=(header->e_shentsize*header->e_shnum);
    elffile->sections=xf86loaderrealloc(elffile->sections,elffile->secsize);
#endif
    elffile->saddr=xf86loadercalloc(elffile->numsh, sizeof(unsigned char *));

#if defined(__alpha__)
    /*
     * Manually fill in the entry for the .got section so ELFCollectSections()
     * will be able to find it.
     */
    elffile->sections[elffile->gotndx].sh_name=SecSize(header->e_shstrndx)+1;
    elffile->sections[elffile->gotndx].sh_type=SHT_PROGBITS;
    elffile->sections[elffile->gotndx].sh_flags=SHF_WRITE|SHF_ALLOC;
    elffile->sections[elffile->gotndx].sh_size=0;
    elffile->sections[elffile->gotndx].sh_addralign=8;
    /* Add room to copy ".got", and maintain alignment */
    SecSize(header->e_shstrndx)+=8;
#endif

/*
 * Get the section header string table
 */
    elffile->shstrsize = SecSize(header->e_shstrndx);
    elffile->shstraddr = _LoaderFileToMem(elffd,SecOffset(header->e_shstrndx),
					  SecSize(header->e_shstrndx),".shstrtab");
    elffile->shstrndx = header->e_shstrndx;
#if defined(__alpha__)
    /*
     * Add the string for the .got section
     */
    strcpy((char*)(elffile->shstraddr+elffile->sections[elffile->gotndx].sh_name),
	   ".got");
#endif

/*
 * Load the rest of the desired sections
 */
    ELFCollectSections(elffile);

    if( elffile->straddr == NULL || elffile->strsize == 0 ) {
	ErrorF("No symbols found in this module\n");
	ELFUnloadModule(elffile);
	return NULL;
    }

/*
 * add symbols
 */
    *ppLookup = ELF_GetSymbols(elffile);

/*
 * Do relocations
 */
    if (elffile->reltxtndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->reltxtndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }
    if (elffile->reldatndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->reldatndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }
    if (elffile->relrodatndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->relrodatndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }
    if (elffile->relsdatndx) {
	elf_reloc = ELFCollectRelocations(elffile,elffile->relsdatndx);
	if (elf_reloc) {
	    for (tail = elf_reloc; tail->next; tail = tail->next)
		;
	    tail->next = _LoaderGetRelocations(v)->elf_reloc;
	    _LoaderGetRelocations(v)->elf_reloc = elf_reloc;
	}
    }

#if defined(__alpha__)
    ELFCreateGOT(elffile);
#endif

    /* Record info for gdb - if we can't allocate the loader record fail
       silently (the user will find out soon enough that there's no VM left */
    if ((elfmod = xf86loadercalloc(1, sizeof(LDRModuleRec))) != NULL) {
	elfmod->name = strdup(modrec->name);
	elfmod->namelen = strlen(modrec->name);
	elfmod->version = 1;
	elfmod->text = elffile->text;
	elfmod->data = elffile->data;
	elfmod->rodata = elffile->rodata;
	elfmod->bss = elffile->bss;
	elfmod->next = ModList;
	elfmod->commons = ldrCommons;
	elfmod->commonslen = nCommons;

	ModList = elfmod;

	/* Tell GDB something interesting happened */
	_loader_debug_state();
    }
    return (void *)elffile;
}

void
ELFResolveSymbols(mod)
void *mod;
{
    ELFRelocPtr newlist, p, tmp;

    /* Try to relocate everything.  Build a new list containing entries
     * which we failed to relocate.  Destroy the old list in the process.
     */
    newlist = 0;
    for (p = _LoaderGetRelocations(mod)->elf_reloc; p; ) {
#ifdef ELFDEBUG
	ErrorF("ResolveSymbols: file %lx, sec %lx, r_offset 0x%x, r_info 0x%lx\n",
	       p->file, p->secp, p->rel->r_offset, p->rel->r_info);
#endif
	tmp = Elf_RelocateEntry(p->file, p->secp, p->rel, FALSE);
	if (tmp) {
	    /* Failed to relocate.  Keep it in the list. */
	    tmp->next = newlist;
	    newlist = tmp;
	}
	tmp = p;
	p = p->next;
	xf86loaderfree(tmp);
    }
    _LoaderGetRelocations(mod)->elf_reloc=newlist;
}

int
ELFCheckForUnresolved(mod)
void	*mod;
{
    ELFRelocPtr	erel;
    char	*name;
    int flag, fatalsym=0;

    if ((erel = _LoaderGetRelocations(mod)->elf_reloc) == NULL)
	return 0;

    while( erel ) {
	Elf_RelocateEntry(erel->file, erel->secp, erel->rel, TRUE);
	name = ElfGetSymbolName(erel->file, ELF_R_SYM(erel->rel->r_info));
	flag = _LoaderHandleUnresolved(
	    name, _LoaderHandleToName(erel->file->handle));
	if(flag) fatalsym = 1;
	xf86loaderfree(name);
	erel=erel->next;
    }
    return fatalsym;
}

void
ELFUnloadModule(modptr)
void *modptr;
{
    ELFModulePtr elffile = (ELFModulePtr)modptr;
    ELFRelocPtr  relptr, reltptr, *brelptr;

/*
 * Delete any unresolved relocations
 */

    relptr=_LoaderGetRelocations(elffile->funcs)->elf_reloc;
    brelptr=&(_LoaderGetRelocations(elffile->funcs)->elf_reloc);

    while(relptr) {
	if( relptr->file == elffile ) {
	    *brelptr=relptr->next;	/* take it out of the list */
	    reltptr=relptr;		/* save pointer to this node */
	    relptr=relptr->next;	/* advance the pointer */
	    xf86loaderfree(reltptr);		/* free the node */
	}
	else {
	    brelptr=&(relptr->next);
	    relptr=relptr->next;	/* advance the pointer */
	    }
    }

/*
 * Delete any symbols in the symbols table.
 */

    LoaderHashTraverse((void *)elffile, ELFhashCleanOut);

/*
 * Free the sections that were allocated.
 */
#define CheckandFree(ptr,size)  if(ptr) _LoaderFreeFileMem((ptr),(size))

    CheckandFree(elffile->straddr,elffile->strsize);
    CheckandFree(elffile->symtab,elffile->symsize);
    CheckandFree(elffile->text,elffile->txtsize);
    CheckandFree(elffile->data,elffile->datsize);
    CheckandFree(elffile->data1,elffile->dat1size);
    CheckandFree(elffile->sdata,elffile->sdatsize);
    CheckandFree(elffile->bss,elffile->bsssize);
    CheckandFree(elffile->rodata,elffile->rodatsize);
    CheckandFree(elffile->reltext,elffile->reltxtsize);
    CheckandFree(elffile->reldata,elffile->reldatsize);
    CheckandFree(elffile->relrodata,elffile->relrodatsize);
    CheckandFree(elffile->relsdata,elffile->relsdatsize);
#if defined(__alpha__)
    CheckandFree(elffile->got,elffile->gotsize);
#endif
    if( elffile->common )
	xf86loaderfree(elffile->common);
/*
 * Free the section table, section pointer array, and section names
 */
    _LoaderFreeFileMem(elffile->sections,elffile->secsize);
    xf86loaderfree(elffile->saddr);
    _LoaderFreeFileMem(elffile->header,sizeof(Elf_Ehdr));
    _LoaderFreeFileMem(elffile->shstraddr,elffile->shstrsize);

/*
 * Free the ELFModuleRec
 */
    xf86loaderfree(elffile);

    return;
}

char *
ELFAddressToSection(void *modptr, unsigned long address)
{
    ELFModulePtr elffile = (ELFModulePtr)modptr;
    int i;

    for( i=1; i<elffile->numsh; i++) {
	if( address >= (unsigned long)elffile->saddr[i] &&
	    address <= (unsigned long)elffile->saddr[i]+SecSize(i) ) {
		return ElfGetSectionName(elffile, elffile->sections[i].sh_name);
		}
	}
return NULL;
}

