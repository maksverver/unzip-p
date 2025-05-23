/*---------------------------------------------------------------------------

  unzip.h (new)

  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  This header file contains the public macros and typedefs required by
  both the UnZip sources and by any application using the UnZip API.  If
  UNZIP_INTERNAL is defined, it includes unzpriv.h (containing includes,
  prototypes and extern variables used by the actual UnZip sources).

  ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
This is version 2009-Jan-02 of the Info-ZIP license.
The definitive version of this document should be available at
ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely and
a copy at http://www.info-zip.org/pub/infozip/license.html.


Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

For the purposes of this copyright and license, "Info-ZIP" is defined as
the following set of individuals:

   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
   Jean-loup Gailly, Hunter Goatley, Ed Gordon, Ian Gorman, Chris Herborth,
   Dirk Haase, Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz,
   David Kirschbaum, Johnny Lee, Onno van der Linden, Igor Mandrichenko,
   Steve P. Miller, Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs,
   Kai Uwe Rommel, Steve Salisbury, Dave Smith, Steven M. Schweda,
   Christian Spieler, Cosmin Truta, Antoine Verheijen, Paul von Behren,
   Rich Wales, Mike White.

This software is provided "as is," without warranty of any kind, express
or implied.  In no event shall Info-ZIP or its contributors be held liable
for any direct, indirect, incidental, special or consequential damages
arising out of the use of or inability to use this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the above disclaimer and the following restrictions:

    1. Redistributions of source code (in whole or in part) must retain
       the above copyright notice, definition, disclaimer, and this list
       of conditions.

    2. Redistributions in binary form (compiled executables and libraries)
       must reproduce the above copyright notice, definition, disclaimer,
       and this list of conditions in documentation and/or other materials
       provided with the distribution.  Additional documentation is not needed
       for executables where a command line license option provides these and
       a note regarding this option is in the executable's startup banner.  The
       sole exception to this condition is redistribution of a standard
       UnZipSFX binary (including SFXWiz) as part of a self-extracting archive;
       that is permitted without inclusion of this license, as long as the
       normal SFX banner has not been removed from the binary or disabled.

    3. Altered versions--including, but not limited to, ports to new operating
       systems, existing ports with new graphical interfaces, versions with
       modified or added functionality, and dynamic, shared, or static library
       versions not from Info-ZIP--must be plainly marked as such and must not
       be misrepresented as being the original source or, if binaries,
       compiled from the original source.  Such altered versions also must not
       be misrepresented as being Info-ZIP releases--including, but not
       limited to, labeling of the altered versions with the names "Info-ZIP"
       (or any variation thereof, including, but not limited to, different
       capitalizations), "Pocket UnZip," "WiZ" or "MacZip" without the
       explicit permission of Info-ZIP.  Such altered versions are further
       prohibited from misrepresentative use of the Zip-Bugs or Info-ZIP
       e-mail addresses or the Info-ZIP URL(s), such as to imply Info-ZIP
       will provide support for the altered versions.

    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
       "UnZipSFX," "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its
       own source and binary releases.
  ---------------------------------------------------------------------------*/

#ifndef __unzip_h   /* prevent multiple inclusions */
#  define __unzip_h

/*---------------------------------------------------------------------------
    Predefined, machine-specific macros.
  ---------------------------------------------------------------------------*/

#  ifdef __COMPILER_KCC__
#    include <c-env.h>
#  endif /* __COMPILER_KCC__ */

#  if (defined(linux) && !defined(LINUX))
#    define LINUX
#  endif


/*---------------------------------------------------------------------------
    Grab system-dependent definition of EXPENTRY for prototypes below.
  ---------------------------------------------------------------------------*/

#  if (defined(USE_UNZIP_LIB))
#    ifndef EXPENTRY
#      define UZ_EXP WINAPI
#    else
#      define UZ_EXP EXPENTRY
#    endif
#  endif

#  ifndef UZ_EXP
#    define UZ_EXP
#  endif

#  ifdef __cplusplus
extern "C" {
#  endif

/*---------------------------------------------------------------------------
    Public typedefs.
  ---------------------------------------------------------------------------*/

#  ifndef _IZ_TYPES_DEFINED
typedef unsigned char   uch;    /* code assumes unsigned bytes; these type-  */
typedef unsigned short  ush;    /*  defs replace byte/UWORD/ULONG (which are */
typedef unsigned long   ulg;    /*  predefined on some systems) & match zip  */
#    define _IZ_TYPES_DEFINED
#  endif /* !_IZ_TYPES_DEFINED */

/* InputFn is not yet used and is likely to change: */
typedef int   (UZ_EXP MsgFn)     (void *pG, uch *buf, ulg size, int flag);
typedef int   (UZ_EXP InputFn)   (void *pG, uch *buf, int *size, int flag);
typedef void  (UZ_EXP PauseFn)   (void *pG, const char *prompt, int flag);
typedef int   (UZ_EXP PasswdFn)  (void *pG, int *rcnt, char *pwbuf,
                                  int size, const char *zfn,
                                  const char *efn);
typedef int   (UZ_EXP StatCBFn)  (void *pG, int fnflag, const char *zfn,
                                  const char *efn, const void *details);
typedef void  (UZ_EXP UsrIniFn)  (void);

typedef struct _UzpBuffer {    /* rxstr */
    ulg   strlength;           /* length of string */
    char  *strptr;             /* pointer to string */
} UzpBuffer;

typedef struct _UzpInit {
    ulg structlen;             /* length of the struct being passed */

    /* GRR: can we assume that each of these is a 32-bit pointer?  if not,
     * does it matter? add "far" keyword to make sure? */
    MsgFn *msgfn;
    InputFn *inputfn;
    PauseFn *pausefn;
    UsrIniFn *userfn;          /* user init function to be called after */
                               /*  globals constructed and initialized */

    /* pointer to program's environment area or something? */
    /* hooks for performance testing? */
    /* hooks for extra unzip -v output? (detect CPU or other hardware?) */
    /* anything else?  let me (Greg) know... */
} UzpInit;

typedef struct _UzpCB {
    ulg structlen;             /* length of the struct being passed */
    /* GRR: can we assume that each of these is a 32-bit pointer?  if not,
     * does it matter? add "far" keyword to make sure? */
    MsgFn *msgfn;
    InputFn *inputfn;
    PauseFn *pausefn;
    PasswdFn *passwdfn;
    StatCBFn *statrepfn;
} UzpCB;

/* the collection of general UnZip option flags and option arguments */
typedef struct _UzpOpts {
#  ifndef FUNZIP
    char *exdir;        /* pointer to extraction root directory (-d option) */
    char *pwdarg;       /* pointer to command-line password (-P option) */
    int zipinfo_mode;   /* behave like ZipInfo or like normal UnZip? */
    int aflag;          /* -a: do ASCII-EBCDIC and/or end-of-line translation */
    int B_flag;         /* -B: back up existing files by renaming to *~##### */
    int cflag;          /* -c: output to stdout */
    int C_flag;         /* -C: match filenames case-insensitively */
    int D_flag;         /* -D: don't restore directory (-DD: any) timestamps */
    int fflag;          /* -f: "freshen" (extract only newer files) */
    int acorn_nfs_ext;  /* -F: RISC OS types & NFS filetype extensions */
    int hflag;          /* -h: header line (zipinfo) */
    int jflag;          /* -j: junk pathnames (unzip) */
    int K_flag;         /* -K: keep setuid/setgid/tacky permissions */
    int lflag;          /* -12slmv: listing format (zipinfo) */
    int L_flag;         /* -L: convert filenames from some OSes to lowercase */
    int overwrite_none; /* -n: never overwrite files (no prompting) */
    int overwrite_all;  /* -o: OK to overwrite files without prompting */
#  endif /* !FUNZIP */
    int qflag;          /* -q: produce a lot less output */
#  ifndef FUNZIP
    int tflag;          /* -t: test (unzip) or totals line (zipinfo) */
    int T_flag;         /* -T: timestamps (unzip) or dec. time fmt (zipinfo) */
    int uflag;          /* -u: "update" (extract only newer/brand-new files) */
    int U_flag;         /* -U: escape non-ASCII, -UU No Unicode paths */
    int vflag;          /* -v: (verbosely) list directory */
    int V_flag;         /* -V: don't strip VMS version numbers */
    int W_flag;         /* -W: wildcard '*' won't match '/' dir separator */
    int X_flag;         /* -X: restore owner/protection or UID/GID or ACLs */
    int zflag;          /* -z: display the zipfile comment (only, for unzip) */
    int ddotflag;       /* -:: don't skip over "../" path elements */
    int cflxflag;       /* -^: allow control chars in extracted filenames */
    int zipbomb;
#  endif /* !FUNZIP */
} UzpOpts;

/* intended to be a private struct: */
typedef struct _ver {
    uch major;              /* e.g., integer 5 */
    uch minor;              /* e.g., 2 */
    uch patchlevel;         /* e.g., 0 */
    uch not_used;
} _version_type;

typedef struct _UzpVer {
    ulg structlen;            /* length of the struct being passed */
    ulg flag;                 /* bit 0: is_beta   bit 1: uses_zlib */
    const char *betalevel;    /* e.g. "g BETA" or "" */
    const char *date;         /* e.g. "9 Oct 08" (beta) or "9 October 2008" */
    const char *zlib_version; /* e.g. "1.2.3" or NULL */
    _version_type unzip;      /* current UnZip version */
    _version_type zipinfo;    /* current ZipInfo version */
    _version_type os2dll;     /* OS2DLL version (retained for compatibility */
    _version_type windll;     /* WinDLL version (retained for compatibility */
    _version_type dllapimin;  /* last incompatible change of library API */
} UzpVer;

/* for Visual BASIC access to Windows DLLs: */
typedef struct _UzpVer2 {
    ulg structlen;            /* length of the struct being passed */
    ulg flag;                 /* bit 0: is_beta   bit 1: uses_zlib */
    char betalevel[10];       /* e.g. "g BETA" or "" */
    char date[20];            /* e.g. "9 Oct 08" (beta) or "9 October 2008" */
    char zlib_version[10];    /* e.g. "1.2.3" or NULL */
    _version_type unzip;      /* current UnZip version */
    _version_type zipinfo;    /* current ZipInfo version */
    _version_type os2dll;     /* OS2DLL version (retained for compatibility */
    _version_type windll;     /* WinDLL version (retained for compatibility */
    _version_type dllapimin;  /* last incompatible change of library API */
} UzpVer2;


typedef struct _Uzp_Siz64 {
    unsigned long lo32;
    unsigned long hi32;
} Uzp_Siz64;

typedef struct _Uzp_cdir_Rec {
    uch version_made_by[2];
    uch version_needed_to_extract[2];
    ush general_purpose_bit_flag;
    ush compression_method;
    ulg last_mod_dos_datetime;
    ulg crc32;
    Uzp_Siz64 csize;
    Uzp_Siz64 ucsize;
    ush filename_length;
    ush extra_field_length;
    ush file_comment_length;
    ush disk_number_start;
    ush internal_file_attributes;
    ulg external_file_attributes;
    Uzp_Siz64 relative_offset_local_header;
} Uzp_cdir_Rec;


#  define UZPINIT_LEN   sizeof(UzpInit)
#  define UZPVER_LEN    sizeof(UzpVer)
#  define cbList(func)  int (* UZ_EXP func)(char *filename, Uzp_cdir_Rec *crec)


/*---------------------------------------------------------------------------
    Return (and exit) values of the public UnZip API functions.
  ---------------------------------------------------------------------------*/

/* external return codes */
#  define PK_OK              0   /* no error */
#  define PK_COOL            0   /* no error */
#  define PK_WARN            1   /* warning error */
#  define PK_ERR             2   /* error in zipfile */
#  define PK_BADERR          3   /* severe error in zipfile */
#  define PK_MEM             4   /* insufficient memory (during initialization) */
#  define PK_MEM2            5   /* insufficient memory (password failure) */
#  define PK_MEM3            6   /* insufficient memory (file decompression) */
#  define PK_MEM4            7   /* insufficient memory (memory decompression) */
#  define PK_MEM5            8   /* insufficient memory (not yet used) */
#  define PK_NOZIP           9   /* zipfile not found */
#  define PK_PARAM          10   /* bad or illegal parameters specified */
#  define PK_FIND           11   /* no files found */
#  define PK_BOMB           12   /* likely zip bomb */
#  define PK_DISK           50   /* disk full */
#  define PK_EOF            51   /* unexpected EOF */

#  define IZ_CTRLC          80   /* user hit ^C to terminate */
#  define IZ_UNSUP          81   /* no files found: all unsup. compr/encrypt. */
#  define IZ_BADPWD         82   /* no files found: all had bad password */
#  define IZ_ERRBF          83   /* big-file archive, small-file program */

/* return codes of password fetches (negative = user abort; positive = error) */
#  define IZ_PW_ENTERED      0   /* got some password string; use/try it */
#  define IZ_PW_CANCEL      -1   /* no password available (for this entry) */
#  define IZ_PW_CANCELALL   -2   /* no password, skip any further pwd. request */
#  define IZ_PW_ERROR        5   /* = PK_MEM2 : failure (no mem, no tty, ...) */

/* flag values for status callback function */
#  define UZ_ST_START_EXTRACT     1       /* no details */
#  define UZ_ST_IN_PROGRESS       2       /* no details */
#  define UZ_ST_FINISH_MEMBER     3       /* 'details': extracted size */

/* return values of status callback function */
#  define UZ_ST_CONTINUE          0
#  define UZ_ST_BREAK             1


/*---------------------------------------------------------------------------
    Prototypes for public UnZip API (DLL) functions.
  ---------------------------------------------------------------------------*/

#  define  UzpMatch match

int      UZ_EXP UzpMessagePrnt      (void *pG, uch *buf, ulg size, int flag);
int      UZ_EXP UzpInput            (void *pG, uch *buf, int *size, int flag);
void     UZ_EXP UzpMorePause        (void *pG, const char *prompt, int flag);
int      UZ_EXP UzpPassword         (void *pG, int *rcnt, char *pwbuf,
                                     int size, const char *zfn,
                                     const char *efn);

#  ifdef __cplusplus
}
#  endif


/*---------------------------------------------------------------------------
    Remaining private stuff for UnZip compilation.
  ---------------------------------------------------------------------------*/

#  ifdef UNZIP_INTERNAL
#    include "unzpriv.h"
#  endif


#endif /* !__unzip_h */
