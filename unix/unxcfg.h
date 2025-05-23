/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------
    Unix specific configuration section:
  ---------------------------------------------------------------------------*/

#ifndef __unxcfg_h
#  define __unxcfg_h

/* Enable large file support. These variables must be defined before any
   system headers are included! */

#  define _FILE_OFFSET_BITS 64   /* select default interface as 64 bit */
#  define _LARGE_FILES           /* only for AIX? */


#  include <sys/types.h>          /* off_t, time_t, dev_t, ... */
#  include <sys/param.h>
#  include <sys/stat.h>
#  include <fcntl.h>            /* O_BINARY for open() w/o CR/LF translation */
#  include <time.h>
#  include <unistd.h>
#  include <utime.h>

typedef off_t zoff_t;

typedef struct stat z_stat;

#  if (!defined(HAVE_STRNICMP) & !defined(NO_STRNICMP))
#    define NO_STRNICMP
#  endif
#  ifndef DATE_FORMAT
#    define DATE_FORMAT DF_MDY    /* GRR:  customize with locale.h somehow? */
#  endif
#  define lenEOL          1
#  ifdef EBCDIC
#    define PutNativeEOL  *q++ = '\n';
#  else
#    define PutNativeEOL  *q++ = native(LF);
#  endif
#  define SCREENSIZE(ttrows, ttcols)  screensize(ttrows, ttcols)
#  define SCREENWIDTH     80
#  define SCREENLWRAP     1
#  define USE_EF_UT_TIME
#  if (!defined(NO_LCHOWN) || !defined(NO_LCHMOD))
#    define SET_SYMLINK_ATTRIBS
#  endif
#  define SET_DIR_ATTRIB
#  if (!defined(NOTIMESTAMP) && !defined(TIMESTAMP))   /* GRR 970513 */
#    define TIMESTAMP
#  endif
#  define RESTORE_UIDGID

/* Static variables that we have to add to Uz_Globs: */
#  define SYSTEM_SPECIFIC_GLOBALS \
    int created_dir, renamed_fullpath;\
    char *rootpath, *buildpath, *end;\
    const char *wildname;\
    char *dirname, matchname[FILNAMSIZ];\
    int rootlen, have_dirname, dirnamelen, notfirstcall;\
    void *wild_dir;

/* created_dir, and renamed_fullpath are used by both mapname() and    */
/*    checkdir().                                                      */
/* rootlen, rootpath, buildpath and end are used by checkdir().        */
/* wild_dir, dirname, wildname, matchname[], dirnamelen, have_dirname, */
/*    and notfirstcall are used by do_wild().                          */


#  define MAX_CP_NAME (25 + 1)
   
#  ifdef _ISO_INTERN
#    undef _ISO_INTERN
#  endif
#  define _ISO_INTERN(str1) iso_intern(str1)

#  ifdef _OEM_INTERN
#    undef _OEM_INTERN
#  endif
#  ifndef IZ_OEM2ISO_ARRAY
#    define IZ_OEM2ISO_ARRAY
#  endif
#  define _OEM_INTERN(str1) oem_intern(str1)

void iso_intern(char *);
void oem_intern(char *);
void init_conversion_charsets(void);

#endif /* !__unxcfg_h */
