/*
  Copyright (c) 1990-2004 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
   ttyio.h
 */

#ifndef __ttyio_h   /* don't include more than once */
#  define __ttyio_h

#  ifndef __crypt_h
#    include "crypt.h"  /* ensure that encryption header file has been seen */
#  endif

#  if (CRYPT || (defined(UNZIP) && !defined(FUNZIP)))
/*
 * Non-echo keyboard/console input support is needed and enabled.
 */

#    ifndef __G         /* UnZip only, for now (DLL stuff) */
#      define __G
#      define __G__
#      define __GDEF
#      define __GPRO    void
#      define __GPRO__
#    endif

/* Function prototypes */

/* For all other systems, ttyio.c supplies the two functions Echoff() and
 * Echon() for suppressing and (re)enabling console input echo.
 */
#    ifndef echoff
#      define echoff(f)  Echoff(__G__ f)
#      define echon()    Echon(__G)
   void Echoff(__GPRO__ int f);
   void Echon(__GPRO);
#    endif

/* this stuff is used by MORE and also now by the ctrl-S code; fileio.c only */
#    if (defined(UNZIP) && !defined(FUNZIP))
#      ifdef HAVE_WORKING_GETCH
#        define FGETCH(f)  getch()
#      endif
#      ifndef FGETCH
     /* default for all systems where no getch()-like function is available */
     int zgetch(__GPRO__ int f);
#        define FGETCH(f)  zgetch(__G__ f)
#      endif
#    endif /* UNZIP && !FUNZIP */

#    if (CRYPT)
   char *getp(__GPRO__ const char *m, char *p, int n);
#    endif

#  else /* !(CRYPT || (UNZIP && !FUNZIP)) */

/*
 * No need for non-echo keyboard/console input; provide dummy definitions.
 */
#    define echoff(f)
#    define echon()

#  endif /* ?(CRYPT || (UNZIP && !FUNZIP)) */

#endif /* !__ttyio_h */
