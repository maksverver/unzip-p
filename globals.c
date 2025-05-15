/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2003-May-08 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  globals.c

  Routines to allocate and initialize globals, with or without threads.

  Contents:  registerGlobalPointer()
             deregisterGlobalPointer()
             getGlobalPointer()
             globalsCtor()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"

#ifndef FUNZIP
/* initialization of sigs is completed at runtime so unzip(sfx) executable
 * won't look like a zipfile
 */
char central_hdr_sig[4]   = {0, 0, 0x01, 0x02};
char local_hdr_sig[4]     = {0, 0, 0x03, 0x04};
char end_central_sig[4]   = {0, 0, 0x05, 0x06};
char end_central64_sig[4] = {0, 0, 0x06, 0x06};
char end_centloc64_sig[4] = {0, 0, 0x06, 0x07};
/* extern char extd_local_sig[4] = {0, 0, 0x07, 0x08};  NOT USED YET */

const char *fnames[2] = {"*", NULL};    /* default filenames vector */
#endif


#ifndef REENTRANT
   Uz_Globs G;
#else /* REENTRANT */

#  ifndef USETHREADID
     Uz_Globs *GG;
#  else /* USETHREADID */
#    define THREADID_ENTRIES  0x40

     int lastScan;
     Uz_Globs  *threadPtrTable[THREADID_ENTRIES];
     ulg        threadIdTable [THREADID_ENTRIES] = {
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,    /* Make sure there are */
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,    /* THREADID_ENTRIES 0s */
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
     };

     static const char TooManyThreads[] =
       "error:  more than %d simultaneous threads.\n\
        Some threads are probably not calling DESTROYTHREAD()\n";
     static const char EntryNotFound[] =
       "error:  couldn't find global pointer in table.\n\
        Maybe somebody accidentally called DESTROYTHREAD() twice.\n";
     static const char GlobalPointerMismatch[] =
       "error:  global pointer in table does not match pointer passed as\
 parameter\n";

static void registerGlobalPointer(__GPRO);



static void registerGlobalPointer(__G)
    __GDEF
{
    int scan=0;
    ulg tid = GetThreadId();

    while (threadIdTable[scan] && scan < THREADID_ENTRIES)
        scan++;

    if (scan == THREADID_ENTRIES) {
        const char *tooMany = TooManyThreads;
        Info(slide, 0x421, ((char *)slide, tooMany, THREADID_ENTRIES));
        free(pG);
        EXIT(PK_MEM);   /* essentially memory error before we've started */
    }

    threadIdTable [scan] = tid;
    threadPtrTable[scan] = pG;
    lastScan = scan;
}



void deregisterGlobalPointer(__G)
    __GDEF
{
    int scan=0;
    ulg tid = GetThreadId();


    while (threadIdTable[scan] != tid && scan < THREADID_ENTRIES)
        scan++;

/*---------------------------------------------------------------------------
    There are two things we can do if we can't find the entry:  ignore it or
    scream.  The most likely reason for it not to be here is the user calling
    this routine twice.  Since this could cause BIG problems if any globals
    are accessed after the first call, we'd better scream.
  ---------------------------------------------------------------------------*/

    if (scan == THREADID_ENTRIES || threadPtrTable[scan] != pG) {
        const char *noEntry;
        if (scan == THREADID_ENTRIES)
            noEntry = EntryNotFound;
        else
            noEntry = GlobalPointerMismatch;
        Info(slide, 0x421, ((char *)slide, noEntry));
        EXIT(PK_WARN);   /* programming error, but after we're all done */
    }

    threadIdTable [scan] = 0;
    lastScan = scan;
    free(threadPtrTable[scan]);
}



Uz_Globs *getGlobalPointer()
{
    int scan=0;
    ulg tid = GetThreadId();

    while (threadIdTable[scan] != tid && scan < THREADID_ENTRIES)
        scan++;

/*---------------------------------------------------------------------------
    There are two things we can do if we can't find the entry:  ignore it or
    scream.  The most likely reason for it not to be here is the user calling
    this routine twice.  Since this could cause BIG problems if any globals
    are accessed after the first call, we'd better scream.
  ---------------------------------------------------------------------------*/

    if (scan == THREADID_ENTRIES) {
        const char *noEntry = EntryNotFound;
        fprintf(stderr, noEntry);  /* can't use Info w/o a global pointer */
        EXIT(PK_ERR);   /* programming error while still working */
    }

    return threadPtrTable[scan];
}

#  endif /* ?USETHREADID */
#endif /* ?REENTRANT */



Uz_Globs *globalsCtor()
{
#ifdef REENTRANT
    Uz_Globs *pG = (Uz_Globs *)malloc(sizeof(Uz_Globs));

    if (!pG)
        return (Uz_Globs *)NULL;
#endif /* REENTRANT */

    /* for REENTRANT version, G is defined as (*pG) */

    memzero(&G, sizeof(Uz_Globs));

#ifndef FUNZIP

    uO.lflag=(-1);
    G.wildzipfn = "";
    G.pfnames = (char **)fnames;
    G.pxnames = (char **)&fnames[1];
    G.pInfo = G.info;
    G.sol = TRUE;          /* at start of line */

    G.message = UzpMessagePrnt;
    G.input = UzpInput;           /* not used by anyone at the moment... */
    G.mpause = UzpMorePause;
    G.decr_passwd = UzpPassword;
#endif /* !FUNZIP */

    G.echofd = -1;

#ifdef SYSTEM_SPECIFIC_CTOR
    SYSTEM_SPECIFIC_CTOR(__G);
#endif

#ifdef REENTRANT
#  ifdef USETHREADID
    registerGlobalPointer(__G);
#  else
    GG = &G;
#  endif /* ?USETHREADID */
#endif /* REENTRANT */

    return &G;
}
