/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  unzip.c

  UnZip - a zipfile extraction utility.  See below for make instructions, or
  read the comments in Makefile and the various Contents files for more de-
  tailed explanations.  To report a bug, submit a *complete* description via
  //www.info-zip.org/zip-bug.html; include machine type, operating system and
  version, compiler and version, and reasonably detailed error messages or
  problem report.  To join Info-ZIP, see the instructions in README.

  UnZip 5.x is a greatly expanded and partially rewritten successor to 4.x,
  which in turn was almost a complete rewrite of version 3.x.  For a detailed
  revision history, see UnzpHist.zip at quest.jpl.nasa.gov.  For a list of
  the many (near infinite) contributors, see "CONTRIBS" in the UnZip source
  distribution.

  UnZip 6.0 adds support for archives larger than 4 GiB using the Zip64
  extensions as well as support for Unicode information embedded per the
  latest zip standard additions.

  ---------------------------------------------------------------------------

  [from original zipinfo.c]

  This program reads great gobs of totally nifty information, including the
  central directory stuff, from ZIP archives ("zipfiles" for short).  It
  started as just a testbed for fooling with zipfiles, but at this point it
  is actually a useful utility.  It also became the basis for the rewrite of
  UnZip (3.16 -> 4.0), using the central directory for processing rather than
  the individual (local) file headers.

  As of ZipInfo v2.0 and UnZip v5.1, the two programs are combined into one.
  If the executable is named "unzip" (or "unzip.exe", depending), it behaves
  like UnZip by default; if it is named "zipinfo" or "ii", it behaves like
  ZipInfo.  The ZipInfo behavior may also be triggered by use of unzip's -Z
  option; for example, "unzip -Z [zipinfo_options] archive.zip".

  Another dandy product from your buddies at Newtware!

  Author:  Greg Roelofs, newt@pobox.com, http://pobox.com/~newt/
           23 August 1990 -> April 1997

  ---------------------------------------------------------------------------

  Version:  unzip5??.{tar.Z | tar.gz | zip} for Unix, VMS, OS/2, MS-DOS, Amiga,
              Atari, Windows 3.x/95/NT/CE, Macintosh, Human68K, Acorn RISC OS,
              AtheOS, BeOS, SMS/QDOS, VM/CMS, MVS, AOS/VS, Tandem NSK, Theos
              and TOPS-20.

  Copyrights:  see accompanying file "LICENSE" in UnZip source distribution.
               (This software is free but NOT IN THE PUBLIC DOMAIN.)

  ---------------------------------------------------------------------------*/



#define __UNZIP_C       /* identifies this source module */
#define UNZIP_INTERNAL
#include "unzip.h"      /* includes, typedefs, macros, prototypes, etc. */
#include "crypt.h"
#include "unzvers.h"


/***************************/
/* Local type declarations */
/***************************/

#if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
typedef struct _sign_info
    {
        struct _sign_info *previous;
        void (*sighandler)(int);
        int sigtype;
    } savsigs_info;
#endif

/*******************/
/* Local Functions */
/*******************/

#if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
static int setsignalhandler (__GPRO__ savsigs_info **p_savedhandler_chain,
                                int signal_type, void (*newhandler)(int));
#endif
#ifndef SFX
static void  help_extended         (__GPRO);
static void  show_version_info     (__GPRO);
#endif


/*************/
/* Constants */
/*************/

#include "consts.h"  /* all constant global variables are in here */
                     /* (non-constant globals were moved to globals.c) */

/* constant local variables: */

#ifndef SFX
   static const char EnvUnZip[] = ENV_UNZIP;
   static const char EnvUnZip2[] = ENV_UNZIP2;
   static const char EnvZipInfo[] = ENV_ZIPINFO;
   static const char EnvZipInfo2[] = ENV_ZIPINFO2;
  static const char NoMemEnvArguments[] =
    "envargs:  cannot get memory for arguments";
  static const char CmdLineParamTooLong[] =
    "error:  command line parameter #%d exceeds internal size limit\n";
#endif /* !SFX */

#if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
  static const char CantSaveSigHandler[] =
    "error:  cannot save signal handler settings\n";
#endif

#if (!defined(SFX) || defined(SFX_EXDIR))
   static const char NotExtracting[] =
     "caution:  not extracting; -d ignored\n";
   static const char MustGiveExdir[] =
     "error:  must specify directory to which to extract with -d option\n";
   static const char OnlyOneExdir[] =
     "error:  -d option used more than once (only one exdir allowed)\n";
#endif

#if CRYPT
   static const char MustGivePasswd[] =
     "error:  must give decryption password with -P option\n";
#endif

#ifndef SFX
   static const char Zfirst[] =
   "error:  -Z must be first option for ZipInfo mode (check UNZIP variable?)\n";
#endif
static const char InvalidOptionsMsg[] = "error:\
  -fn or any combination of -c, -l, -p, -t, -u and -v options invalid\n";
static const char IgnoreOOptionMsg[] =
  "caution:  both -n and -o specified; ignoring -o\n";

/* usage() strings */
#ifndef SFX
   static const char Example3[] = "ReadMe";
   static const char Example2[] = " \
 unzip -p foo | more  => send contents of foo.zip via pipe into program more\n";

/* local1[]:  command options */
#  if defined(TIMESTAMP)
   static const char local1[] =
     "  -T  timestamp archive to latest";
#  else /* !TIMESTAMP */
   static const char local1[] = "";
#  endif /* ?TIMESTAMP */

/* local2[] and local3[]:  modifier options */
   static const char local2[] = " -X  restore UID/GID info";
#  ifdef MORE
   static const char local3[] = "\
  -K  keep setuid/setgid/tacky permissions   -M  pipe through \"more\" pager\n";
#  else
   static const char local3[] = "\
  -K  keep setuid/setgid/tacky permissions\n";
#  endif
#endif /* !SFX */

#ifndef NO_ZIPINFO
   static const char ZipInfoExample[] = "*, ?, [] (e.g., \"[a-j]*.zip\")";

static const char ZipInfoUsageLine1[] = "\
ZipInfo %d.%d%d%s of %s, by Greg Roelofs and the Info-ZIP group.\n\
\n\
List name, date/time, attribute, size, compression method, etc., about files\n\
in list (excluding those in xlist) contained in the specified .zip archive(s).\
\n\"file[.zip]\" may be a wildcard name containing %s.\n\n\
   usage:  zipinfo [-12smlvChMtTz] file[.zip] [list...] [-x xlist...]\n\
      or:  unzip %s-Z%s [-12smlvChMtTz] file[.zip] [list...] [-x xlist...]\n";

static const char ZipInfoUsageLine2[] = "\nmain\
 listing-format options:             -s  short Unix \"ls -l\" format (def.)\n\
  -1  filenames ONLY, one per line       -m  medium Unix \"ls -l\" format\n\
  -2  just filenames but allow -h/-t/-z  -l  long Unix \"ls -l\" format\n\
                                         -v  verbose, multi-page format\n";

static const char ZipInfoUsageLine3[] = "miscellaneous options:\n\
  -h  print header line       -t  print totals for listed files or for all\n\
  -z  print zipfile comment   -T  print file times in sortable decimal format\
\n  -C  be case-insensitive   %s\
  -x  exclude filenames that follow from listing\n\
  -O  CHARSET  specify a character encoding for DOS, Windows and OS/2 archives\n\
  -I  CHARSET  specify a character encoding for UNIX and other archives\n";
#  ifdef MORE
   static const char ZipInfoUsageLine4[] =
     "  -M  page output through built-in \"more\"\n";
#  else /* !MORE */
   static const char ZipInfoUsageLine4[] = "";
#  endif /* ?MORE */
#endif /* !NO_ZIPINFO */

#ifdef BETA
     static const char BetaVersion[] = "%s\
        THIS IS STILL A BETA VERSION OF UNZIP%s -- DO NOT DISTRIBUTE.\n\n";
#endif

#ifdef SFX
     static const char UnzipSFXBanner[] =
     "UnZipSFX %d.%d%d%s of %s, by Info-ZIP (http://www.info-zip.org).\n";
#  ifdef SFX_EXDIR
     static const char UnzipSFXOpts[] =
    "Valid options are -tfupcz and -d <exdir>; modifiers are -abjnoqCL%sV%s.\n";
#  else
     static const char UnzipSFXOpts[] =
       "Valid options are -tfupcz; modifiers are -abjnoqCL%sV%s.\n";
#  endif
#else /* !SFX */
   static const char CompileOptions[] =
     "UnZip special compilation options:\n";
   static const char CompileOptFormat[] = "        %s\n";
   static const char EnvOptions[] =
     "\nUnZip and ZipInfo environment options:\n";
   static const char EnvOptFormat[] = "%16s:  %.1024s\n";
   static const char None[] = "[none]";
   static const char AcornFtypeNFS[] = "ACORN_FTYPE_NFS";
#  ifdef ASM_CRC
     static const char AsmCRC[] = "ASM_CRC";
#  endif
#  ifdef ASM_INFLATECODES
     static const char AsmInflateCodes[] = "ASM_INFLATECODES";
#  endif
#  ifdef CHECK_VERSIONS
     static const char Check_Versions[] = "CHECK_VERSIONS";
#  endif
     static const char Copyright_Clean[] =
     "COPYRIGHT_CLEAN (PKZIP 0.9x unreducing method not supported)";
#  ifdef DEBUG
     static const char UDebug[] = "DEBUG";
#  endif
#  ifdef DEBUG_TIME
     static const char DebugTime[] = "DEBUG_TIME";
#  endif
#  ifdef LZW_CLEAN
     static const char LZW_Clean[] =
     "LZW_CLEAN (PKZIP/Zip 1.x unshrinking method not supported)";
#  endif
#  ifndef MORE
     static const char No_More[] = "NO_MORE";
#  endif
#  ifdef NO_ZIPINFO
     static const char No_ZipInfo[] = "NO_ZIPINFO";
#  endif
#  ifdef REENTRANT
     static const char Reentrant[] = "REENTRANT";
#  endif
#  ifdef REGARGS
     static const char RegArgs[] = "REGARGS";
#  endif
#  ifdef RETURN_CODES
     static const char Return_Codes[] = "RETURN_CODES";
#  endif
#  ifdef SET_DIR_ATTRIB
     static const char SetDirAttrib[] = "SET_DIR_ATTRIB";
#  endif
     static const char SymLinkSupport[] =
     "SYMLINKS (symbolic links supported, if RTL and file system permit)";
#  ifdef TIMESTAMP
     static const char TimeStamp[] = "TIMESTAMP";
#  endif
     static const char UnixBackup[] = "UNIXBACKUP";
     static const char Use_EF_UT_time[] = "USE_EF_UT_TIME";
#  ifndef LZW_CLEAN
     static const char Use_Unshrink[] =
     "USE_UNSHRINK (PKZIP/Zip 1.x unshrinking method supported)";
#  endif
#  ifdef USE_DEFLATE64
     static const char Use_Deflate64[] =
     "USE_DEFLATE64 (PKZIP 4.x Deflate64(tm) supported)";
#  endif
     /* direct native UTF-8 check AND charset transform via wchar_t */
     static const char Use_Unicode[] =
     "UNICODE_SUPPORT [wide-chars, char coding: %s] (handle UTF-8 paths)";
     static const char SysChUTF8[] = "UTF-8";
     static const char SysChOther[] = "other";
#  ifdef MULT_VOLUME
     static const char Use_MultiVol[] =
     "MULT_VOLUME (multi-volume archives supported)";
#  endif
     static const char Use_LFS[] =
     "LARGE_FILE_SUPPORT (large files over 2 GiB supported)";
     static const char Use_Zip64[] =
     "ZIP64_SUPPORT (archives using Zip64 for large files supported)";
#  ifdef USE_VFAT
     static const char Use_VFAT_support[] = "USE_VFAT";
#  endif
#  ifdef USE_ZLIB
     static const char UseZlib[] =
     "USE_ZLIB (compiled with version %s; using version %s)";
#  endif
#  ifdef USE_BZIP2
     static const char UseBZip2[] =
     "USE_BZIP2 (PKZIP 4.6+, using bzip2 lib version %s)";
#  endif
#  ifdef VMS_TEXT_CONV
     static const char VmsTextConv[] = "VMS_TEXT_CONV";
#  endif
     static const char WildStopAtDir[] = "WILD_STOP_AT_DIR";
#  if CRYPT
#    ifdef PASSWD_FROM_STDIN
       static const char PasswdStdin[] = "PASSWD_FROM_STDIN";
#    endif
     static const char Decryption[] =
       "        [decryption, version %d.%d%s of %s]\n";
     static const char CryptDate[] = CR_VERSION_DATE;
#  endif

   static const char UnzipUsageLine1[] = "\
UnZip %d.%d%d%s of %s, by Info-ZIP.  Maintained by C. Spieler.  Send\n\
bug reports using http://www.info-zip.org/zip-bug.html; see README for details.\
\n\n";
#  define UnzipUsageLine1v       UnzipUsageLine1

static const char UnzipUsageLine2v[] = "\
Latest sources and executables are at ftp://ftp.info-zip.org/pub/infozip/ ;\
\nsee ftp://ftp.info-zip.org/pub/infozip/UnZip.html for other sites.\
\n\n";

static const char UnzipUsageLine2[] = "\
Usage: unzip %s[-opts[modifiers]] file[.zip] [list] [-x xlist] [-d exdir]\n \
 Default action is to extract files in list, except those in xlist, to exdir;\n\
  file[.zip] may be a wildcard.  %s\n";

#  ifdef NO_ZIPINFO
#    define ZIPINFO_MODE_OPTION  ""
   static const char ZipInfoMode[] =
     "(ZipInfo mode is disabled in this version.)";
#  else
#    define ZIPINFO_MODE_OPTION  "[-Z] "
   static const char ZipInfoMode[] =
     "-Z => ZipInfo mode (\"unzip -Z\" for usage).";
#  endif /* ?NO_ZIPINFO */


static const char UnzipUsageLine3[] = "\n\
  -p  extract files to pipe, no messages     -l  list files (short format)\n\
  -f  freshen existing files, create none    -t  test compressed archive data\n\
  -u  update files, create if necessary      -z  display archive comment only\n\
  -v  list verbosely/show version info     %s\n\
  -x  exclude files that follow (in xlist)   -d  extract files into exdir\n";

/* There is not enough space on a standard 80x25 Windows console screen for
 * the additional line advertising the UTF-8 debugging options. This may
 * eventually also be the case for other ports. Probably, the -U option need
 * not be shown on the introductory screen at all. [Chr. Spieler, 2008-02-09]
 *
 * Likely, other advanced options should be moved to an extended help page and
 * the option to list that page put here.  [E. Gordon, 2008-3-16]
 */
static const char UnzipUsageLine4[] = "\
modifiers:\n\
  -n  never overwrite existing files         -q  quiet mode (-qq => quieter)\n\
  -o  overwrite files WITHOUT prompting      -a  auto-convert any text files\n\
  -j  junk paths (do not make directories)   -aa treat ALL files as text\n\
  -U  use escapes for all non-ASCII Unicode  -UU ignore any Unicode fields\n\
  -C  match filenames case-insensitively     -L  make (some) names \
lowercase\n %-42s  -V  retain VMS version numbers\n%s\
  -O  CHARSET  specify a character encoding for DOS, Windows and OS/2 archives\n\
  -I  CHARSET  specify a character encoding for UNIX and other archives\n\n";

static const char UnzipUsageLine5[] = "\
See \"unzip -hh\" or unzip.txt for more help.  Examples:\n\
  unzip data1 -x joe   => extract all files except joe from zipfile data1.zip\n\
%s\
  unzip -fo foo %-6s => quietly replace existing %s if archive file newer\n";
#endif /* ?SFX */





/*****************************/
/*  main() / UzpMain() stub  */
/*****************************/

int MAIN(argc, argv)   /* return PK-type error code (except under VMS) */
    int argc;
    char *argv[];
{
    int r;

    CONSTRUCTGLOBALS();
    r = unzip(__G__ argc, argv);
    DESTROYGLOBALS();
    RETURN(r);
}




/*******************************/
/*  Primary UnZip entry point  */
/*******************************/

int unzip(__G__ argc, argv)
    __GDEF
    int argc;
    char *argv[];
{
#ifndef NO_ZIPINFO
    char *p;
#endif
#if (!defined(SFX))
    int i;
#endif
    int retcode, error=FALSE;
#ifndef NO_EXCEPT_SIGNALS
#  ifdef REENTRANT
    savsigs_info *oldsighandlers = NULL;
#    define SET_SIGHANDLER(sigtype, newsighandler) \
      if ((retcode = setsignalhandler(__G__ &oldsighandlers, (sigtype), \
                                      (newsighandler))) > PK_WARN) \
          goto cleanup_and_exit
#  else
#    define SET_SIGHANDLER(sigtype, newsighandler) \
      signal((sigtype), (newsighandler))
#  endif
#endif /* NO_EXCEPT_SIGNALS */

    /* initialize international char support to the current environment */
    setlocale(LC_CTYPE, "");

    /* see if can use UTF-8 Unicode locale */
    {
        char *codeset;
        /* get the codeset (character set encoding) currently used */
#include <langinfo.h>

        codeset = nl_langinfo(CODESET);
        /* is the current codeset UTF-8 ? */
        if ((codeset != NULL) && (strcmp(codeset, "UTF-8") == 0)) {
            /* successfully found UTF-8 char coding */
            G.native_is_utf8 = TRUE;
        } else {
            /* Current codeset is not UTF-8 or cannot be determined. */
            G.native_is_utf8 = FALSE;
        }
        /* Note: At least for UnZip, trying to change the process codeset to
         *       UTF-8 does not work.  For the example Linux setup of the
         *       UnZip maintainer, a successful switch to "en-US.UTF-8"
         *       resulted in garbage display of all non-basic ASCII characters.
         */
    }

    /* initialize Unicode */
    G.unicode_escape_all = 0;
    G.unicode_mismatch = 0;

    G.unipath_version = 0;
    G.unipath_checksum = 0;
    G.unipath_filename = NULL;


    init_conversion_charsets();


#ifdef MALLOC_WORK
    /* The following (rather complex) expression determines the allocation
       size of the decompression work area.  It simulates what the
       combined "union" and "struct" declaration of the "static" work
       area reservation achieves automatically at compile time.
       Any decent compiler should evaluate this expression completely at
       compile time and provide constants to the zcalloc() call.
       (For better readability, some subexpressions are encapsulated
       in temporarly defined macros.)
     */
#  define UZ_SLIDE_CHUNK (sizeof(shrint)+sizeof(uch)+sizeof(uch))
#  define UZ_NUMOF_CHUNKS \
      (unsigned)(((WSIZE+UZ_SLIDE_CHUNK-1)/UZ_SLIDE_CHUNK > HSIZE) ? \
                 (WSIZE+UZ_SLIDE_CHUNK-1)/UZ_SLIDE_CHUNK : HSIZE)
    G.area.Slide = (uch *)zcalloc(UZ_NUMOF_CHUNKS, UZ_SLIDE_CHUNK);
#  undef UZ_SLIDE_CHUNK
#  undef UZ_NUMOF_CHUNKS
    G.area.shrink.Parent = (shrint *)G.area.Slide;
    G.area.shrink.value = G.area.Slide + (sizeof(shrint)*(HSIZE));
    G.area.shrink.Stack = G.area.Slide +
                           (sizeof(shrint) + sizeof(uch))*(HSIZE);
#endif

/*---------------------------------------------------------------------------
    Set signal handler for restoring echo, warn of zipfile corruption, etc.
  ---------------------------------------------------------------------------*/
#ifndef NO_EXCEPT_SIGNALS
#  ifdef SIGINT
    SET_SIGHANDLER(SIGINT, handler);
#  endif
#  ifdef SIGTERM                 /* some systems really have no SIGTERM */
    SET_SIGHANDLER(SIGTERM, handler);
#  endif
#  ifdef SIGABRT
    SET_SIGHANDLER(SIGABRT, handler);
#  endif
#  ifdef SIGBREAK
    SET_SIGHANDLER(SIGBREAK, handler);
#  endif
#  ifdef SIGBUS
    SET_SIGHANDLER(SIGBUS, handler);
#  endif
#  ifdef SIGILL
    SET_SIGHANDLER(SIGILL, handler);
#  endif
#  ifdef SIGSEGV
    SET_SIGHANDLER(SIGSEGV, handler);
#  endif
#endif /* NO_EXCEPT_SIGNALS */


/*---------------------------------------------------------------------------
    Sanity checks.  Commentary by Otis B. Driftwood and Fiorello:

    D:  It's all right.  That's in every contract.  That's what they
        call a sanity clause.

    F:  Ha-ha-ha-ha-ha.  You can't fool me.  There ain't no Sanity
        Claus.
  ---------------------------------------------------------------------------*/

#ifdef DEBUG
  /* test if we can support large files - 10/6/04 EG */
    if (sizeof(zoff_t) < 8) {
        Info(slide, 0x401, ((char *)slide, "LARGE_FILE_SUPPORT set but not supported\n"));
        retcode = PK_BADERR;
        goto cleanup_and_exit;
    }
    /* test if we can show 64-bit values */
    {
        zoff_t z = ~(zoff_t)0;  /* z should be all 1s now */
        char *sz;

        sz = FmZofft(z, FZOFFT_HEX_DOT_WID, "X");
        if ((sz[0] != 'F') || (strlen(sz) != 16))
        {
            z = 0;
        }

        /* shift z so only MSB is set */
        z <<= 63;
        sz = FmZofft(z, FZOFFT_HEX_DOT_WID, "X");
        if ((sz[0] != '8') || (strlen(sz) != 16))
        {
            Info(slide, 0x401, ((char *)slide,
              "Can't show 64-bit values correctly\n"));
            retcode = PK_BADERR;
            goto cleanup_and_exit;
        }
    }

    /* 2004-11-30 SMS.
       Test the NEXTBYTE macro for proper operation.
    */
    {
        int test_char;
        static uch test_buf[2] = { 'a', 'b' };

        G.inptr = test_buf;
        G.incnt = 1;

        test_char = NEXTBYTE;           /* Should get 'a'. */
        if (test_char == 'a')
        {
            test_char = NEXTBYTE;       /* Should get EOF, not 'b'. */
        }
        if (test_char != EOF)
        {
            Info(slide, 0x401, ((char *)slide,
 "NEXTBYTE macro failed.  Try compiling with ALT_NEXTBYTE defined?"));

            retcode = PK_BADERR;
            goto cleanup_and_exit;
        }
    }
#endif /* DEBUG */

/*---------------------------------------------------------------------------
    First figure out if we're running in UnZip mode or ZipInfo mode, and put
    the appropriate environment-variable options into the queue.  Then rip
    through any command-line options lurking about...
  ---------------------------------------------------------------------------*/

#ifdef SFX
    G.argv0 = argv[0];
    G.zipfn = G.argv0;

    uO.zipinfo_mode = FALSE;
    error = uz_opts(__G__ &argc, &argv);   /* UnZipSFX call only */

#else /* !SFX */

    G.noargs = (argc == 1);   /* no options, no zipfile, no anything */

#  ifndef NO_ZIPINFO
    for (p = argv[0] + strlen(argv[0]); p >= argv[0]; --p) {
        if (*p == DIR_END
#    ifdef DIR_END2
            || *p == DIR_END2
#    endif
           )
            break;
    }
    ++p;
    if (STRNICMP(p, Zipnfo, 7) == 0 ||
        STRNICMP(p, "ii", 2) == 0 ||
        (argc > 1 && strncmp(argv[1], "-Z", 2) == 0))
    {
        uO.zipinfo_mode = TRUE;
        if ((error = envargs(&argc, &argv, EnvZipInfo,
                             EnvZipInfo2)) != PK_OK)
            perror(NoMemEnvArguments);
    } else
#  endif /* !NO_ZIPINFO */
    {
        uO.zipinfo_mode = FALSE;
        if ((error = envargs(&argc, &argv, EnvUnZip,
                             EnvUnZip2)) != PK_OK)
            perror(NoMemEnvArguments);
    }

    if (!error) {
        /* Check the length of all passed command line parameters.
         * Command arguments might get sent through the Info() message
         * system, which uses the sliding window area as string buffer.
         * As arguments may additionally get fed through one of the FnFilter
         * macros, we require all command line arguments to be shorter than
         * WSIZE/4 (and ca. 2 standard line widths for fixed message text).
         */
        for (i = 1 ; i < argc; i++) {
           if (strlen(argv[i]) > ((WSIZE>>2) - 160)) {
               Info(slide, 0x401, ((char *)slide,
                 CmdLineParamTooLong, i));
               retcode = PK_PARAM;
               goto cleanup_and_exit;
           }
        }
#  ifndef NO_ZIPINFO
        if (uO.zipinfo_mode)
            error = zi_opts(__G__ &argc, &argv);
        else
#  endif /* !NO_ZIPINFO */
            error = uz_opts(__G__ &argc, &argv);
    }

#endif /* ?SFX */

    if ((argc < 0) || error) {
        retcode = error;
        goto cleanup_and_exit;
    }

/*---------------------------------------------------------------------------
    Now get the zipfile name from the command line and then process any re-
    maining options and file specifications.
  ---------------------------------------------------------------------------*/

#ifndef SFX
    G.wildzipfn = *argv++;
#endif

#if (defined(SFX) && !defined(SFX_EXDIR)) /* only check for -x */

    G.filespecs = argc;
    G.xfilespecs = 0;

    if (argc > 0) {
        char **pp = argv-1;

        G.pfnames = argv;
        while (*++pp)
            if (strcmp(*pp, "-x") == 0) {
                if (pp > argv) {
                    *pp = 0;              /* terminate G.pfnames */
                    G.filespecs = pp - G.pfnames;
                } else {
                    G.pfnames = (char **)fnames;  /* defaults */
                    G.filespecs = 0;
                }
                G.pxnames = pp + 1;      /* excluded-names ptr: _after_ -x */
                G.xfilespecs = argc - G.filespecs - 1;
                break;                    /* skip rest of args */
            }
        G.process_all_files = FALSE;
    } else
        G.process_all_files = TRUE;      /* for speed */

#else /* !SFX || SFX_EXDIR */             /* check for -x or -d */

    G.filespecs = argc;
    G.xfilespecs = 0;

    if (argc > 0) {
        int in_files=FALSE, in_xfiles=FALSE;
        char **pp = argv-1;

        G.process_all_files = FALSE;
        G.pfnames = argv;
        while (*++pp) {
            Trace((stderr, "pp - argv = %d\n", pp-argv));
            if (!uO.exdir && strncmp(*pp, "-d", 2) == 0) {
                int firstarg = (pp == argv);

                uO.exdir = (*pp) + 2;
                if (in_files) {      /* ... zipfile ... -d exdir ... */
                    *pp = (char *)NULL;         /* terminate G.pfnames */
                    G.filespecs = pp - G.pfnames;
                    in_files = FALSE;
                } else if (in_xfiles) {
                    *pp = (char *)NULL;         /* terminate G.pxnames */
                    G.xfilespecs = pp - G.pxnames;
                    /* "... -x xlist -d exdir":  nothing left */
                }
                /* first check for "-dexdir", then for "-d exdir" */
                if (*uO.exdir == '\0') {
                    if (*++pp)
                        uO.exdir = *pp;
                    else {
                        Info(slide, 0x401, ((char *)slide,
                          MustGiveExdir));
                        /* don't extract here by accident */
                        retcode = PK_PARAM;
                        goto cleanup_and_exit;
                    }
                }
                if (firstarg) { /* ... zipfile -d exdir ... */
                    if (pp[1]) {
                        G.pfnames = pp + 1;  /* argv+2 */
                        G.filespecs = argc - (G.pfnames-argv);  /* for now... */
                    } else {
                        G.process_all_files = TRUE;
                        G.pfnames = (char **)fnames;  /* GRR: necessary? */
                        G.filespecs = 0;     /* GRR: necessary? */
                        break;
                    }
                }
            } else if (!in_xfiles) {
                if (strcmp(*pp, "-x") == 0) {
                    in_xfiles = TRUE;
                    if (pp == G.pfnames) {
                        G.pfnames = (char **)fnames;  /* defaults */
                        G.filespecs = 0;
                    } else if (in_files) {
                        *pp = 0;                   /* terminate G.pfnames */
                        G.filespecs = pp - G.pfnames;  /* adjust count */
                        in_files = FALSE;
                    }
                    G.pxnames = pp + 1; /* excluded-names ptr starts after -x */
                    G.xfilespecs = argc - (G.pxnames-argv);  /* anything left */
                } else
                    in_files = TRUE;
            }
        }
    } else
        G.process_all_files = TRUE;      /* for speed */

    if (uO.exdir != (char *)NULL && !G.extract_flag)    /* -d ignored */
        Info(slide, 0x401, ((char *)slide, NotExtracting));
#endif /* ?(SFX && !SFX_EXDIR) */

    /* set Unicode-escape-all if option -U used */
    if (uO.U_flag == 1)
        G.unicode_escape_all = TRUE;


/*---------------------------------------------------------------------------
    Okey dokey, we have everything we need to get started.  Let's roll.
  ---------------------------------------------------------------------------*/

    retcode = process_zipfiles(__G);

cleanup_and_exit:
#if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
    /* restore all signal handlers back to their state at function entry */
    while (oldsighandlers != NULL) {
        savsigs_info *thissigsav = oldsighandlers;

        signal(thissigsav->sigtype, thissigsav->sighandler);
        oldsighandlers = thissigsav->previous;
        free(thissigsav);
    }
#endif
#if (defined(MALLOC_WORK) && !defined(REENTRANT))
    if (G.area.Slide != (uch *)NULL) {
        free(G.area.Slide);
        G.area.Slide = (uch *)NULL;
    }
#endif
    return(retcode);

} /* end main()/unzip() */





#if (defined(REENTRANT) && !defined(NO_EXCEPT_SIGNALS))
/*******************************/
/* Function setsignalhandler() */
/*******************************/

static int setsignalhandler(__G__ p_savedhandler_chain, signal_type,
                            newhandler)
    __GDEF
    savsigs_info **p_savedhandler_chain;
    int signal_type;
    void (*newhandler)(int);
{
    savsigs_info *savsig;

    savsig = malloc(sizeof(savsigs_info));
    if (savsig == NULL) {
        /* error message and break */
        Info(slide, 0x401, ((char *)slide, CantSaveSigHandler));
        return PK_MEM;
    }
    savsig->sigtype = signal_type;
    savsig->sighandler = signal(SIGINT, newhandler);
    if (savsig->sighandler == SIG_ERR) {
        free(savsig);
    } else {
        savsig->previous = *p_savedhandler_chain;
        *p_savedhandler_chain = savsig;
    }
    return PK_OK;

} /* end function setsignalhandler() */

#endif /* REENTRANT && !NO_EXCEPT_SIGNALS */





/**********************/
/* Function uz_opts() */
/**********************/

int uz_opts(__G__ pargc, pargv)
    __GDEF
    int *pargc;
    char ***pargv;
{
    char **argv, *s, *zipbomb_envar;
    int argc, c, error=FALSE, negative=0, showhelp=0;


    argc = *pargc;
    argv = *pargv;

    extern char OEM_CP[MAX_CP_NAME];
    extern char ISO_CP[MAX_CP_NAME];

    while (++argv, (--argc > 0 && *argv != NULL && **argv == '-')) {
        s = *argv + 1;
        while ((c = *s++) != 0) {    /* "!= 0":  prevent Turbo C warning */
            switch (c)
            {
                case ('-'):
                    ++negative;
                    break;
                case ('a'):
                    if (negative) {
                        uO.aflag = MAX(uO.aflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.aflag;
                    break;
                case ('b'):
                    if (negative) {
                        negative = 0;   /* do nothing:  "-b" is default */
                    } else {
                        uO.aflag = 0;
                    }
                    break;
                case ('B'): /* -B: back up existing files */
                    if (negative)
                        uO.B_flag = FALSE, negative = 0;
                    else
                        uO.B_flag = TRUE;
                    break;
                case ('c'):
                    if (negative) {
                        uO.cflag = FALSE, negative = 0;
#ifdef NATIVE
                        uO.aflag = 0;
#endif
                    } else {
                        uO.cflag = TRUE;
#ifdef NATIVE
                        uO.aflag = 2;   /* so you can read it on the screen */
#endif
                    }
                    break;
                case ('C'):    /* -C:  match filenames case-insensitively */
                    if (negative)
                        uO.C_flag = FALSE, negative = 0;
                    else
                        uO.C_flag = TRUE;
                    break;
#if (!defined(SFX) || defined(SFX_EXDIR))
                case ('d'):
                    if (negative) {   /* negative not allowed with -d exdir */
                        Info(slide, 0x401, ((char *)slide,
                          MustGiveExdir));
                        return(PK_PARAM);  /* don't extract here by accident */
                    }
                    if (uO.exdir != (char *)NULL) {
                        Info(slide, 0x401, ((char *)slide,
                          OnlyOneExdir));
                        return(PK_PARAM);    /* GRR:  stupid restriction? */
                    } else {
                        /* first check for "-dexdir", then for "-d exdir" */
                        uO.exdir = s;
                        if (*uO.exdir == '\0') {
                            if (argc > 1) {
                                --argc;
                                uO.exdir = *++argv;
                                if (*uO.exdir == '-') {
                                    Info(slide, 0x401, ((char *)slide,
                                      MustGiveExdir));
                                    return(PK_PARAM);
                                }
                                /* else uO.exdir points at extraction dir */
                            } else {
                                Info(slide, 0x401, ((char *)slide,
                                  MustGiveExdir));
                                return(PK_PARAM);
                            }
                        }
                        /* uO.exdir now points at extraction dir (-dexdir or
                         *  -d exdir); point s at end of exdir to avoid mis-
                         *  interpretation of exdir characters as more options
                         */
                        if (*s != 0)
                            while (*++s != 0)
                                ;
                    }
                    break;
#endif /* !SFX || SFX_EXDIR */
#if (!defined(NO_TIMESTAMPS))
                case ('D'):    /* -D: Skip restoring dir (or any) timestamp. */
                    if (negative) {
                        uO.D_flag = MAX(uO.D_flag-negative,0);
                        negative = 0;
                    } else
                        uO.D_flag++;
                    break;
#endif /* (!NO_TIMESTAMPS) */
                case ('e'):    /* just ignore -e, -x options (extract) */
                    break;
                case ('f'):    /* "freshen" (extract only newer files) */
                    if (negative)
                        uO.fflag = uO.uflag = FALSE, negative = 0;
                    else
                        uO.fflag = uO.uflag = TRUE;
                    break;
                case ('F'):    /* Acorn filetype & NFS extension handling */
                    if (negative)
                        uO.acorn_nfs_ext = FALSE, negative = 0;
                    else
                        uO.acorn_nfs_ext = TRUE;
                    break;
                case ('h'):    /* just print help message and quit */
                    if (showhelp == 0) {
#ifndef SFX
                        if (*s == 'h')
                            showhelp = 2;
                        else
#endif /* !SFX */
                        {
                            showhelp = 1;
                        }
                    }
                    break;
                case ('I'):
                    if (negative) {
                        Info(slide, 0x401, ((char *)slide,
                          "error:  encodings can't be negated"));
                        return(PK_PARAM);
                    } else {
                        if(*s) { /* Handle the -Icharset case */
                            /* Assume that charsets can't start with a dash to spot arguments misuse */
                            if(*s == '-') {
                                Info(slide, 0x401, ((char *)slide,
                                  "error:  a valid character encoding should follow the -I argument"));
                                return(PK_PARAM);
                            }
                            strncpy(ISO_CP, s, MAX_CP_NAME - 1);
                            ISO_CP[MAX_CP_NAME - 1] = '\0';
                        } else { /* -I charset */
                            ++argv;
                            if(!(--argc > 0 && *argv != NULL && **argv != '-')) {
                                Info(slide, 0x401, ((char *)slide,
                                  "error:  a valid character encoding should follow the -I argument"));
                                return(PK_PARAM);
                            }
                            s = *argv;
                            strncpy(ISO_CP, s, MAX_CP_NAME - 1);
                            ISO_CP[MAX_CP_NAME - 1] = '\0';
                        }
                        while(*(++s)); /* No params straight after charset name */
                    }
                    break;
                case ('j'):    /* junk pathnames/directory structure */
                    if (negative)
                        uO.jflag = FALSE, negative = 0;
                    else
                        uO.jflag = TRUE;
                    break;
                case ('K'):
                    if (negative) {
                        uO.K_flag = FALSE, negative = 0;
                    } else {
                        uO.K_flag = TRUE;
                    }
                    break;
#ifndef SFX
                case ('l'):
                    if (negative) {
                        uO.vflag = MAX(uO.vflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.vflag;
                    break;
#endif /* !SFX */
                case ('L'):    /* convert (some) filenames to lowercase */
                    if (negative) {
                        uO.L_flag = MAX(uO.L_flag-negative,0);
                        negative = 0;
                    } else
                        ++uO.L_flag;
                    break;
#ifdef MORE
                case ('M'):    /* send all screen output through "more" fn. */
/* GRR:  eventually check for numerical argument => height */
                    if (negative)
                        G.M_flag = FALSE, negative = 0;
                    else
                        G.M_flag = TRUE;
                    break;
#endif /* MORE */
                case ('n'):    /* don't overwrite any files */
                    if (negative)
                        uO.overwrite_none = FALSE, negative = 0;
                    else
                        uO.overwrite_none = TRUE;
                    break;
                case ('o'):    /* OK to overwrite files without prompting */
                    if (negative) {
                        uO.overwrite_all = MAX(uO.overwrite_all-negative,0);
                        negative = 0;
                    } else
                        ++uO.overwrite_all;
                    break;
                case ('O'):
                    if (negative) {
                        Info(slide, 0x401, ((char *)slide,
                          "error:  encodings can't be negated"));
                        return(PK_PARAM);
                    } else {
                        if(*s) { /* Handle the -Ocharset case */
                            /* Assume that charsets can't start with a dash to spot arguments misuse */
                            if(*s == '-') {
                                Info(slide, 0x401, ((char *)slide,
                                  "error:  a valid character encoding should follow the -I argument"));
                                return(PK_PARAM);
                            }
                            strncpy(OEM_CP, s, MAX_CP_NAME - 1);
                            OEM_CP[MAX_CP_NAME - 1] = '\0';
                        } else { /* -O charset */
                            ++argv;
                            if(!(--argc > 0 && *argv != NULL && **argv != '-')) {
                                Info(slide, 0x401, ((char *)slide,
                                  "error:  a valid character encoding should follow the -O argument"));
                                return(PK_PARAM);
                            }
                            s = *argv;
                            strncpy(OEM_CP, s, MAX_CP_NAME - 1);
                            OEM_CP[MAX_CP_NAME - 1] = '\0';
                        }
                        while(*(++s)); /* No params straight after charset name */
                    }
                    break;
                case ('p'):    /* pipes:  extract to stdout, no messages */
                    if (negative) {
                        uO.cflag = FALSE;
                        uO.qflag = MAX(uO.qflag-999,0);
                        negative = 0;
                    } else {
                        uO.cflag = TRUE;
                        uO.qflag += 999;
                    }
                    break;
#if CRYPT
                /* GRR:  yes, this is highly insecure, but dozens of people
                 * have pestered us for this, so here we go... */
                case ('P'):
                    if (negative) {   /* negative not allowed with -P passwd */
                        Info(slide, 0x401, ((char *)slide,
                          MustGivePasswd));
                        return(PK_PARAM);  /* don't extract here by accident */
                    }
                    if (uO.pwdarg != (char *)NULL) {
/*
                        GRR:  eventually support multiple passwords?
                        Info(slide, 0x401, ((char *)slide,
                          OnlyOnePasswd));
                        return(PK_PARAM);
 */
                    } else {
                        /* first check for "-Ppasswd", then for "-P passwd" */
                        uO.pwdarg = s;
                        if (*uO.pwdarg == '\0') {
                            if (argc > 1) {
                                --argc;
                                uO.pwdarg = *++argv;
                                if (*uO.pwdarg == '-') {
                                    Info(slide, 0x401, ((char *)slide,
                                      MustGivePasswd));
                                    return(PK_PARAM);
                                }
                                /* else pwdarg points at decryption password */
                            } else {
                                Info(slide, 0x401, ((char *)slide,
                                  MustGivePasswd));
                                return(PK_PARAM);
                            }
                        }
                        /* pwdarg now points at decryption password (-Ppasswd or
                         *  -P passwd); point s at end of passwd to avoid mis-
                         *  interpretation of passwd characters as more options
                         */
                        if (*s != 0)
                            while (*++s != 0)
                                ;
                    }
                    break;
#endif /* CRYPT */
                case ('q'):    /* quiet:  fewer comments/messages */
                    if (negative) {
                        uO.qflag = MAX(uO.qflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.qflag;
                    break;
                case ('t'):
                    if (negative)
                        uO.tflag = FALSE, negative = 0;
                    else
                        uO.tflag = TRUE;
                    break;
#ifdef TIMESTAMP
                case ('T'):
                    if (negative)
                        uO.T_flag = FALSE, negative = 0;
                    else
                        uO.T_flag = TRUE;
                    break;
#endif
                case ('u'):    /* update (extract only new and newer files) */
                    if (negative)
                        uO.uflag = FALSE, negative = 0;
                    else
                        uO.uflag = TRUE;
                    break;
                case ('U'):    /* escape UTF-8, or disable UTF-8 support */
                    if (negative) {
                        uO.U_flag = MAX(uO.U_flag-negative,0);
                        negative = 0;
                    } else
                        uO.U_flag++;
                    break;
#ifndef SFX
                case ('v'):    /* verbose */
                    if (negative) {
                        uO.vflag = MAX(uO.vflag-negative,0);
                        negative = 0;
                    } else if (uO.vflag)
                        ++uO.vflag;
                    else
                        uO.vflag = 2;
                    break;
#endif /* !SFX */
                case ('V'):    /* Version (retain VMS/DEC-20 file versions) */
                    if (negative)
                        uO.V_flag = FALSE, negative = 0;
                    else
                        uO.V_flag = TRUE;
                    break;
                case ('W'):    /* Wildcard interpretation (stop at '/'?) */
                    if (negative)
                        uO.W_flag = FALSE, negative = 0;
                    else
                        uO.W_flag = TRUE;
                    break;
                case ('x'):    /* extract:  default */
#ifdef SFX
                    /* when 'x' is the only option in this argument, and the
                     * next arg is not an option, assume this initiates an
                     * exclusion list (-x xlist):  terminate option-scanning
                     * and leave uz_opts with argv still pointing to "-x";
                     * the xlist is processed later
                     */
                    if (s - argv[0] == 2 && *s == '\0' &&
                        argc > 1 && argv[1][0] != '-') {
                        /* break out of nested loops without "++argv;--argc" */
                        goto opts_done;
                    }
#endif /* SFX */
                    break;
                case ('X'):   /* restore owner/protection info (need privs?) */
                    if (negative) {
                        uO.X_flag = MAX(uO.X_flag-negative,0);
                        negative = 0;
                    } else
                        ++uO.X_flag;
                    break;
                case ('z'):    /* display only the archive comment */
                    if (negative) {
                        uO.zflag = MAX(uO.zflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.zflag;
                    break;
#ifndef SFX
                case ('Z'):    /* should have been first option (ZipInfo) */
                    Info(slide, 0x401, ((char *)slide, Zfirst));
                    error = TRUE;
                    break;
#endif /* !SFX */
                case (':'):    /* allow "parent dir" path components */
                    if (negative) {
                        uO.ddotflag = MAX(uO.ddotflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.ddotflag;
                    break;
                case ('^'):    /* allow control chars in filenames */
                    if (negative) {
                        uO.cflxflag = MAX(uO.cflxflag-negative,0);
                        negative = 0;
                    } else
                        ++uO.cflxflag;
                    break;
                default:
                    error = TRUE;
                    break;

            } /* end switch */
        } /* end while (not end of argument string) */
    } /* end while (not done with switches) */

/*---------------------------------------------------------------------------
    Check for nonsensical combinations of options.
  ---------------------------------------------------------------------------*/

#ifdef SFX
opts_done:  /* yes, very ugly...but only used by UnZipSFX with -x xlist */
#endif

    if (showhelp > 0) {         /* just print help message and quit */
        *pargc = -1;
#ifndef SFX
        if (showhelp == 2) {
            help_extended(__G);
            return PK_OK;
        } else
#endif /* !SFX */
        {
            return USAGE(PK_OK);
        }
    }

    if ((uO.cflag && (uO.tflag || uO.uflag)) ||
        (uO.tflag && uO.uflag) || (uO.fflag && uO.overwrite_none))
    {
        Info(slide, 0x401, ((char *)slide, InvalidOptionsMsg));
        error = TRUE;
    }
    if (uO.aflag > 2)
        uO.aflag = 2;
    if (uO.overwrite_all && uO.overwrite_none) {
        Info(slide, 0x401, ((char *)slide, IgnoreOOptionMsg));
        uO.overwrite_all = FALSE;
    }
#ifdef MORE
    if (G.M_flag && !isatty(1))  /* stdout redirected: "more" func. useless */
        G.M_flag = 0;
#endif

#ifdef SFX
    if (error)
#else
    if ((argc-- == 0) || error)
#endif
    {
        *pargc = argc;
        *pargv = argv;
#ifndef SFX
        if (uO.vflag >= 2 && argc == -1) {              /* "unzip -v" */
            show_version_info(__G);
            return PK_OK;
        }
        if (!G.noargs && !error)
            error = TRUE;       /* had options (not -h or -v) but no zipfile */
#endif /* !SFX */
        return USAGE(error);
    }

#ifdef SFX
    /* print our banner unless we're being fairly quiet */
    if (uO.qflag < 2)
        Info(slide, error? 1 : 0, ((char *)slide, UnzipSFXBanner,
          UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, UZ_BETALEVEL,
          VersionDate));
#  ifdef BETA
    /* always print the beta warning:  no unauthorized distribution!! */
    Info(slide, error? 1 : 0, ((char *)slide, BetaVersion, "\n",
      "SFX"));
#  endif
#endif /* SFX */

    if (uO.cflag || uO.tflag || uO.vflag || uO.zflag
#ifdef TIMESTAMP
                                                     || uO.T_flag
#endif
                                                                 )
        G.extract_flag = FALSE;
    else
        G.extract_flag = TRUE;

    /* Disable the zipbomb detection, this is the only option set only via the shell variables but it should at least not clash with something in the future. */
    zipbomb_envar = getenv("UNZIP_DISABLE_ZIPBOMB_DETECTION");
    uO.zipbomb = TRUE;
    if (zipbomb_envar != NULL) {
      /* strcasecmp might be a better approach here but it is POSIX-only */
      if ((strcmp ("TRUE", zipbomb_envar) == 0)
       || (strcmp ("True", zipbomb_envar) == 0)
       || (strcmp ("true",zipbomb_envar) == 0)) {
        uO.zipbomb = FALSE;
      }
    }

    *pargc = argc;
    *pargv = argv;
    return PK_OK;

} /* end function uz_opts() */




/********************/
/* Function usage() */
/********************/

#ifdef SFX
#  define LOCAL "X"
   /* Default for all other systems: */
#  ifndef LOCAL
#    define LOCAL ""
#  endif

#  ifndef NO_TIMESTAMP
#    ifdef MORE
#      define SFXOPT1 "DM"
#    else
#      define SFXOPT1 "D"
#    endif
#  else
#    ifdef MORE
#      define SFXOPT1 "M"
#    else
#      define SFXOPT1 ""
#    endif
#  endif

int usage(__G__ error)   /* return PK-type error code */
    __GDEF
    int error;
{
    Info(slide, error? 1 : 0, ((char *)slide, UnzipSFXBanner,
      UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, UZ_BETALEVEL,
      VersionDate));
    Info(slide, error? 1 : 0, ((char *)slide, UnzipSFXOpts,
      SFXOPT1, LOCAL));
#  ifdef BETA
    Info(slide, error? 1 : 0, ((char *)slide, BetaVersion, "\n",
      "SFX"));
#  endif

    if (error)
        return PK_PARAM;
    else
        return PK_COOL;     /* just wanted usage screen: no error */

} /* end function usage() */





#else /* !SFX */
#  define QUOT ' '
#  define QUOTS ""

int usage(__G__ error)   /* return PK-type error code */
    __GDEF
    int error;
{
    int flag = (error? 1 : 0);


/*---------------------------------------------------------------------------
    Print either ZipInfo usage or UnZip usage, depending on incantation.
    (Strings must be no longer than 512 bytes for Turbo C, apparently.)
  ---------------------------------------------------------------------------*/

    if (uO.zipinfo_mode) {

#  ifndef NO_ZIPINFO

        Info(slide, flag, ((char *)slide, ZipInfoUsageLine1,
          ZI_MAJORVER, ZI_MINORVER, UZ_PATCHLEVEL, UZ_BETALEVEL,
          VersionDate,
          ZipInfoExample, QUOTS,QUOTS));
        Info(slide, flag, ((char *)slide, ZipInfoUsageLine2));
        Info(slide, flag, ((char *)slide, ZipInfoUsageLine3,
          ZipInfoUsageLine4));

#  endif /* !NO_ZIPINFO */

    } else {   /* UnZip mode */

        Info(slide, flag, ((char *)slide, UnzipUsageLine1,
          UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, UZ_BETALEVEL,
          VersionDate));
#  ifdef BETA
        Info(slide, flag, ((char *)slide, BetaVersion, "", ""));
#  endif

        Info(slide, flag, ((char *)slide, UnzipUsageLine2,
          ZIPINFO_MODE_OPTION, ZipInfoMode));

        Info(slide, flag, ((char *)slide, UnzipUsageLine3,
          local1));

        Info(slide, flag, ((char *)slide, UnzipUsageLine4,
          local2, local3));

        /* This is extra work for SMALL_MEM, but it will work since
         * LoadFarStringSmall2 uses the same buffer.  Remember, this
         * is a hack. */
        Info(slide, flag, ((char *)slide, UnzipUsageLine5,
          Example2, Example3,
          Example3));

    } /* end if (uO.zipinfo_mode) */

    if (error)
        return PK_PARAM;
    else
        return PK_COOL;     /* just wanted usage screen: no error */

} /* end function usage() */

#endif /* ?SFX */




#ifndef SFX

/* Print extended help to stdout. */
static void help_extended(__G)
    __GDEF
{
    extent i;             /* counter for help array */

    /* help array */
    static const char *text[] = {
  "",
  "Extended Help for UnZip",
  "",
  "See the UnZip Manual for more detailed help",
  "",
  "",
  "UnZip lists and extracts files in zip archives.  The default action is to",
  "extract zipfile entries to the current directory, creating directories as",
  "needed.  With appropriate options, UnZip lists the contents of archives",
  "instead.",
  "",
  "Basic unzip command line:",
  "  unzip [-Z] options archive[.zip] [file ...] [-x xfile ...] [-d exdir]",
  "",
  "Some examples:",
  "  unzip -l foo.zip        - list files in short format in archive foo.zip",
  "",
  "  unzip -t foo            - test the files in archive foo",
  "",
  "  unzip -Z foo            - list files using more detailed zipinfo format",
  "",
  "  unzip foo               - unzip the contents of foo in current dir",
  "",
  "  unzip -a foo            - unzip foo and convert text files to local OS",
  "",
  "If unzip is run in zipinfo mode, a more detailed list of archive contents",
  "is provided.  The -Z option sets zipinfo mode and changes the available",
  "options.",
  "",
  "Basic zipinfo command line:",
  "  zipinfo options archive[.zip] [file ...] [-x xfile ...]",
  "  unzip -Z options archive[.zip] [file ...] [-x xfile ...]",
  "",
  "Below, Mac OS refers to Mac OS before Mac OS X.  Mac OS X is a Unix based",
  "port and is referred to as Unix Apple.",
  "",
  "",
  "unzip options:",
  "  -Z   Switch to zipinfo mode.  Must be first option.",
  "  -hh  Display extended help.",
  "  -A   [OS/2, Unix DLL] Print extended help for DLL.",
  "  -c   Extract files to stdout/screen.  As -p but include names.  Also,",
  "         -a allowed and EBCDIC conversions done if needed.",
  "  -f   Freshen by extracting only if older file on disk.",
  "  -l   List files using short form.",
  "  -p   Extract files to pipe (stdout).  Only file data is output and all",
  "         files extracted in binary mode (as stored).",
  "  -t   Test archive files.",
  "  -T   Set timestamp on archive(s) to that of newest file.  Similar to",
  "       zip -o but faster.",
  "  -u   Update existing older files on disk as -f and extract new files.",
  "  -v   Use verbose list format.  If given alone as unzip -v show version",
  "         information.  Also can be added to other list commands for more",
  "         verbose output.",
  "  -z   Display only archive comment.",
  "",
  "unzip modifiers:",
  "  -a   Convert text files to local OS format.  Convert line ends, EOF",
  "         marker, and from or to EBCDIC character set as needed.",
  "  -b   Treat all files as binary.  [Tandem] Force filecode 180 ('C').",
  "         [VMS] Autoconvert binary files.  -bb forces convert of all files.",
  "  -B   [UNIXBACKUP compile option enabled] Save a backup copy of each",
  "         overwritten file in foo~ or foo~99999 format.",
  "  -C   Use case-insensitive matching.",
  "  -D   Skip restoration of timestamps for extracted directories.  On VMS this",
  "         is on by default and -D essentially becames -DD.",
  "  -DD  Skip restoration of timestamps for all entries.",
  "  -E   [MacOS (not Unix Apple)]  Display contents of MacOS extra field during",
  "         restore.",
  "  -F   [Acorn] Suppress removal of NFS filetype extension.  [Non-Acorn if",
  "         ACORN_FTYPE_NFS] Translate filetype and append to name.",
  "  -i   [MacOS] Ignore filenames in MacOS extra field.  Instead, use name in",
  "         standard header.",
  "  -j   Junk paths and deposit all files in extraction directory.",
  "  -J   [BeOS] Junk file attributes.  [MacOS] Ignore MacOS specific info.",
  "  -K   [AtheOS, BeOS, Unix] Restore SUID/SGID/Tacky file attributes.",
  "  -L   Convert to lowercase any names from uppercase only file system.",
  "  -LL  Convert all files to lowercase.",
  "  -M   Pipe all output through internal pager similar to Unix more(1).",
  "  -n   Never overwrite existing files.  Skip extracting that file, no prompt.",
  "  -N   [Amiga] Extract file comments as Amiga filenotes.",
  "  -o   Overwrite existing files without prompting.  Useful with -f.  Use with",
  "         care.",
  "  -P p Use password p to decrypt files.  THIS IS INSECURE!  Some OS show",
  "         command line to other users.",
  "  -q   Perform operations quietly.  The more q (as in -qq) the quieter.",
  "  -s   [OS/2, NT, MS-DOS] Convert spaces in filenames to underscores.",
  "  -S   [VMS] Convert text files (-a, -aa) into Stream_LF format.",
  "  -U   [UNICODE enabled] Show non-local characters as #Uxxxx or #Lxxxxxx ASCII",
  "         text escapes where x is hex digit.  [Old] -U used to leave names",
  "         uppercase if created on MS-DOS, VMS, etc.  See -L.",
  "  -UU  [UNICODE enabled] Disable use of stored UTF-8 paths.  Note that UTF-8",
  "         paths stored as native local paths are still processed as Unicode.",
  "  -V   Retain VMS file version numbers.",
  "  -W   [Only if WILD_STOP_AT_DIR] Modify pattern matching so ? and * do not",
  "         match directory separator /, but ** does.  Allows matching at specific",
  "         directory levels.",
  "  -X   [VMS, Unix, OS/2, NT, Tandem] Restore UICs and ACL entries under VMS,",
  "         or UIDs/GIDs under Unix, or ACLs under certain network-enabled",
  "         versions of OS/2, or security ACLs under Windows NT.  Can require",
  "         user privileges.",
  "  -XX  [NT] Extract NT security ACLs after trying to enable additional",
  "         system privileges.",
  "  -Y   [VMS] Treat archived name endings of .nnn as VMS version numbers.",
  "  -$   [MS-DOS, OS/2, NT] Restore volume label if extraction medium is",
  "         removable.  -$$ allows fixed media (hard drives) to be labeled.",
  "  -/ e [Acorn] Use e as extension list.",
  "  -:   [All but Acorn, VM/CMS, MVS, Tandem] Allow extract archive members into",
  "         locations outside of current extraction root folder.  This allows",
  "         paths such as ../foo to be extracted above the current extraction",
  "         directory, which can be a security problem.",
  "  -^   [Unix] Allow control characters in names of extracted entries.  Usually",
  "         this is not a good thing and should be avoided.",
  "  -2   [VMS] Force unconditional conversion of names to ODS-compatible names.",
  "         Default is to exploit destination file system, preserving cases and",
  "         extended name characters on ODS5 and applying ODS2 filtering on ODS2.",
  "",
  "",
  "Wildcards:",
  "  Internally unzip supports the following wildcards:",
  "    ?       (or %% or #, depending on OS) matches any single character",
  "    *       matches any number of characters, including zero",
  "    [list]  matches char in list (regex), can do range [ac-f], all but [!bf]",
  "  If port supports [], must escape [ as [[]",
  "  For shells that expand wildcards, escape (\\* or \"*\") so unzip can recurse.",
  "",
  "Include and Exclude:",
  "  -i pattern pattern ...   include files that match a pattern",
  "  -x pattern pattern ...   exclude files that match a pattern",
  "  Patterns are paths with optional wildcards and match paths as stored in",
  "  archive.  Exclude and include lists end at next option or end of line.",
  "    unzip archive -x pattern pattern ...",
  "",
  "Multi-part (split) archives (archives created as a set of split files):",
  "  Currently split archives are not readable by unzip.  A workaround is",
  "  to use zip to convert the split archive to a single-file archive and",
  "  use unzip on that.  See the manual page for Zip 3.0 or later.",
  "",
  "Streaming (piping into unzip):",
  "  Currently unzip does not support streaming.  The funzip utility can be",
  "  used to process the first entry in a stream.",
  "    cat archive | funzip",
  "",
  "Testing archives:",
  "  -t        test contents of archive",
  "  This can be modified using -q for quieter operation, and -qq for even",
  "  quieter operation.",
  "",
  "Unicode:",
  "  If compiled with Unicode support, unzip automatically handles archives",
  "  with Unicode entries.  Currently Unicode on Win32 systems is limited.",
  "  Characters not in the current character set are shown as ASCII escapes",
  "  in the form #Uxxxx where the Unicode character number fits in 16 bits,",
  "  or #Lxxxxxx where it doesn't, where x is the ASCII character for a hex",
  "  digit.",
  "",
  "",
  "zipinfo options (these are used in zipinfo mode (unzip -Z ...)):",
  "  -1  List names only, one per line.  No headers/trailers.  Good for scripts.",
  "  -2  List names only as -1, but include headers, trailers, and comments.",
  "  -s  List archive entries in short Unix ls -l format.  Default list format.",
  "  -m  List in long Unix ls -l format.  As -s, but includes compression %.",
  "  -l  List in long Unix ls -l format.  As -m, but compression in bytes.",
  "  -v  List zipfile information in verbose, multi-page format.",
  "  -h  List header line.  Includes archive name, actual size, total files.",
  "  -M  Pipe all output through internal pager similar to Unix more(1) command.",
  "  -t  List totals for files listed or for all files.  Includes uncompressed",
  "        and compressed sizes, and compression factors.",
  "  -T  Print file dates and times in a sortable decimal format (yymmdd.hhmmss)",
  "        Default date and time format is a more human-readable version.",
  "  -U  [UNICODE] If entry has a UTF-8 Unicode path, display any characters",
  "        not in current character set as text #Uxxxx and #Lxxxxxx escapes",
  "        representing the Unicode character number of the character in hex.",
  "  -UU [UNICODE]  Disable use of any UTF-8 path information.",
  "  -z  Include archive comment if any in listing.",
  "",
  "",
  "funzip stream extractor:",
  "  funzip extracts the first member in an archive to stdout.  Typically",
  "  used to unzip the first member of a stream or pipe.  If a file argument",
  "  is given, read from that file instead of stdin.",
  "",
  "funzip command line:",
  "  funzip [-password] [input[.zip|.gz]]",
  "",
  "",
  "unzipsfx self extractor:",
  "  Self-extracting archives made with unzipsfx are no more (or less)",
  "  portable across different operating systems than unzip executables.",
  "  In general, a self-extracting archive made on a particular Unix system,",
  "  for example, will only self-extract under the same flavor of Unix.",
  "  Regular unzip may still be used to extract embedded archive however.",
  "",
  "unzipsfx command line:",
  "  <unzipsfx+archive_filename>  [-options] [file(s) ... [-x xfile(s) ...]]",
  "",
  "unzipsfx options:",
  "  -c, -p - Output to pipe.  (See above for unzip.)",
  "  -f, -u - Freshen and Update, as for unzip.",
  "  -t     - Test embedded archive.  (Can be used to list contents.)",
  "  -z     - Print archive comment.  (See unzip above.)",
  "",
  "unzipsfx modifiers:",
  "  Most unzip modifiers are supported.  These include",
  "  -a     - Convert text files.",
  "  -n     - Never overwrite.",
  "  -o     - Overwrite without prompting.",
  "  -q     - Quiet operation.",
  "  -C     - Match names case-insensitively.",
  "  -j     - Junk paths.",
  "  -V     - Keep version numbers.",
  "  -s     - Convert spaces to underscores.",
  "  -$     - Restore volume label.",
  "",
  "If unzipsfx compiled with SFX_EXDIR defined, -d option also available:",
  "  -d exd - Extract to directory exd.",
  "By default, all files extracted to current directory.  This option",
  "forces extraction to specified directory.",
  "",
  "See unzipsfx manual page for more information.",
  ""
    };

    for (i = 0; i < sizeof(text)/sizeof(char *); i++)
    {
        Info(slide, 0, ((char *)slide, "%s\n", text[i]));
    }
} /* end function help_extended() */




/********************************/
/* Function show_version_info() */
/********************************/

static void show_version_info(__G)
    __GDEF
{
    if (uO.qflag > 3)                           /* "unzip -vqqqq" */
        Info(slide, 0, ((char *)slide, "%d\n",
          (UZ_MAJORVER*100 + UZ_MINORVER*10 + UZ_PATCHLEVEL)));
    else {
        char *envptr;
        int numopts = 0;

        Info(slide, 0, ((char *)slide, UnzipUsageLine1v,
          UZ_MAJORVER, UZ_MINORVER, UZ_PATCHLEVEL, UZ_BETALEVEL,
          VersionDate));
        Info(slide, 0, ((char *)slide,
          UnzipUsageLine2v));
        version(__G);
        Info(slide, 0, ((char *)slide, CompileOptions));
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          AcornFtypeNFS));
        ++numopts;
#  ifdef ASM_CRC
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          AsmCRC));
        ++numopts;
#  endif
#  ifdef ASM_INFLATECODES
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          AsmInflateCodes));
        ++numopts;
#  endif
#  ifdef CHECK_VERSIONS
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Check_Versions));
        ++numopts;
#  endif
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Copyright_Clean));
        ++numopts;
#  ifdef DEBUG
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          UDebug));
        ++numopts;
#  endif
#  ifdef DEBUG_TIME
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          DebugTime));
        ++numopts;
#  endif
#  ifdef LZW_CLEAN
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          LZW_Clean));
        ++numopts;
#  endif
#  ifndef MORE
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          No_More));
        ++numopts;
#  endif
#  ifdef NO_ZIPINFO
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          No_ZipInfo));
        ++numopts;
#  endif
#  ifdef REENTRANT
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Reentrant));
        ++numopts;
#  endif
#  ifdef REGARGS
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          RegArgs));
        ++numopts;
#  endif
#  ifdef RETURN_CODES
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Return_Codes));
        ++numopts;
#  endif
#  ifdef SET_DIR_ATTRIB
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          SetDirAttrib));
        ++numopts;
#  endif
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          SymLinkSupport));
        ++numopts;
#  ifdef TIMESTAMP
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          TimeStamp));
        ++numopts;
#  endif
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          UnixBackup));
        ++numopts;
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_EF_UT_time));
        ++numopts;
#  ifndef LZW_CLEAN
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_Unshrink));
        ++numopts;
#  endif
#  ifdef USE_DEFLATE64
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_Deflate64));
        ++numopts;
#  endif
        sprintf((char *)(slide+256), Use_Unicode,
          G.native_is_utf8 ? SysChUTF8 : SysChOther);
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          (char *)(slide+256)));
        ++numopts;
#  ifdef MULT_VOLUME
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_MultiVol));
        ++numopts;
#  endif
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_LFS));
        ++numopts;
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_Zip64));
        ++numopts;
#  ifdef USE_VFAT
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          Use_VFAT_support));
        ++numopts;
#  endif
#  ifdef USE_ZLIB
        sprintf((char *)(slide+256), UseZlib,
          ZLIB_VERSION, zlibVersion());
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          (char *)(slide+256)));
        ++numopts;
#  endif
#  ifdef USE_BZIP2
        sprintf((char *)(slide+256), UseBZip2,
          BZ2_bzlibVersion());
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          (char *)(slide+256)));
        ++numopts;
#  endif
#  ifdef VMS_TEXT_CONV
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          VmsTextConv));
        ++numopts;
#  endif
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          WildStopAtDir));
        ++numopts;
#  if CRYPT
#    ifdef PASSWD_FROM_STDIN
        Info(slide, 0, ((char *)slide, CompileOptFormat,
          PasswdStdin));
#    endif
        Info(slide, 0, ((char *)slide, Decryption,
          CR_MAJORVER, CR_MINORVER, CR_BETA_VER,
          CryptDate));
        ++numopts;
#  endif /* CRYPT */
        if (numopts == 0)
            Info(slide, 0, ((char *)slide,
              CompileOptFormat,
              None));

        Info(slide, 0, ((char *)slide, EnvOptions));
        envptr = getenv(EnvUnZip);
        Info(slide, 0, ((char *)slide, EnvOptFormat,
          EnvUnZip,
          (envptr == (char *)NULL || *envptr == 0)?
          None : envptr));
        envptr = getenv(EnvUnZip2);
        Info(slide, 0, ((char *)slide, EnvOptFormat,
          EnvUnZip2,
          (envptr == (char *)NULL || *envptr == 0)?
          None : envptr));
        envptr = getenv(EnvZipInfo);
        Info(slide, 0, ((char *)slide, EnvOptFormat,
          EnvZipInfo,
          (envptr == (char *)NULL || *envptr == 0)?
          None : envptr));
        envptr = getenv(EnvZipInfo2);
        Info(slide, 0, ((char *)slide, EnvOptFormat,
          EnvZipInfo2,
          (envptr == (char *)NULL || *envptr == 0)?
          None : envptr));
    }
} /* end function show_version() */

#endif /* !SFX */
