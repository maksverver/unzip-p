/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  fileio.c

  This file contains routines for doing direct but relatively generic input/
  output, file-related sorts of things, plus some miscellaneous stuff.  Most
  of the stuff has to do with opening, closing, reading and/or writing files.

  Contains:  open_input_file()
             open_outfile()
             undefer_input()
             defer_leftover_input()
             readbuf()
             readbyte()
             fillinbuf()
             seek_zipf()
             flush()
             is_vms_varlen_txt()      (VMS_TEXT_CONV only)
             disk_error()
             UzpMessagePrnt()
             UzpInput()
             UzpMorePause()
             UzpPassword()
             handler()
             dos_to_unix_time()
             check_for_newer()
             do_string()
             makeword()
             makelong()
             makeint64()
             fzofft()
             str2iso()                (CRYPT && NEED_STR2ISO, only)
             str2oem()                (CRYPT && NEED_STR2OEM, only)
             zstrnicmp()              (NO_STRNICMP only)

  ---------------------------------------------------------------------------*/


#define __FILEIO_C      /* identifies this source module */
#define UNZIP_INTERNAL
#include "unzip.h"
#include "crc32.h"
#include "crypt.h"
#include "ttyio.h"

/* setup of codepage conversion for decryption passwords */
#if CRYPT
#  if (defined(CRYP_USES_ISO2OEM) && !defined(IZ_ISO2OEM_ARRAY))
#    define IZ_ISO2OEM_ARRAY            /* pull in iso2oem[] table */
#  endif
#  if (defined(CRYP_USES_OEM2ISO) && !defined(IZ_OEM2ISO_ARRAY))
#    define IZ_OEM2ISO_ARRAY            /* pull in oem2iso[] table */
#  endif
#endif
#include "ebcdic.h"   /* definition/initialization of ebcdic[] */


/*
   Note: Under Windows, the maximum size of the buffer that can be used
   with any of the *printf calls is 16,384, so win_fprintf was used to
   feed the fprintf clone no more than 16K chunks at a time. This should
   be valid for anything up to 64K (and probably beyond, assuming your
   buffers are that big).
*/
#ifdef USE_FWRITE
#  define WriteError(buf,len,strm) \
     ((extent)fwrite((char *)(buf),1,(extent)(len),strm) != (extent)(len))
#else
#  define WriteError(buf,len,strm) \
     ((extent)write(fileno(strm),(char *)(buf),(extent)(len)) != (extent)(len))
#endif

/*
   2005-09-16 SMS.
   On VMS, when output is redirected to a file, as in a command like
   "PIPE UNZIP -v > X.OUT", the output file is created with VFC record
   format, and multiple calls to write() or fwrite() will produce multiple
   records, even when there's no newline terminator in the buffer.
   The result is unsightly output with spurious newlines.  Using fprintf()
   instead of write() here, and disabling a fflush(stdout) in UzpMessagePrnt()
   below, together seem to solve the problem.

   According to the C RTL manual, "The write and decc$record_write
   functions always generate at least one record."  Also, "[T]he fwrite
   function always generates at least <number_items> records."  So,
   "fwrite(buf, len, 1, strm)" is much better ("1" record) than
   "fwrite(buf, 1, len, strm)" ("len" (1-character) records, _really_
   ugly), but neither is better than write().  Similarly, "The fflush
   function always generates a record if there is unwritten data in the
   buffer."  Apparently fprintf() buffers the stuff somewhere, and puts
   out a record (only) when it sees a newline.
*/
#define WriteTxtErr(buf,len,strm)  WriteError(buf,len,strm)

#ifdef VMS_TEXT_CONV
static int is_vms_varlen_txt(__GPRO__ uch *ef_buf, unsigned ef_len);
#endif


/****************************/
/* Strings used in fileio.c */
/****************************/

static const char CannotOpenZipfile[] =
  "error:  cannot open zipfile [ %s ]\n        %s\n";

static const char CannotDeleteOldFile[] =
  "error:  cannot delete old %s\n        %s\n";
static const char CannotRenameOldFile[] =
  "error:  cannot rename old %s\n        %s\n";
static const char BackupSuffix[] = "~";
static const char CannotCreateFile[] =
    "error:  cannot create %s\n        %s\n";
static const char ReadError[] = "error:  zipfile read error\n";
static const char FilenameTooLongTrunc[] =
  "warning:  filename too long--truncating.\n";
static const char UFilenameCorrupt[] =
  "error: Unicode filename corrupt.\n";
static const char UFilenameTooLongTrunc[] =
  "warning:  Converted Unicode filename too long--truncating.\n";
static const char ExtraFieldTooLong[] =
  "warning:  extra field too long (%d).  Ignoring...\n";
static const char ExtraFieldCorrupt[] =
  "warning:  extra field (type: 0x%04x) corrupt.  Continuing...\n";
static const char DiskFullQuery[] =
    "%s:  write error (disk full?).  Continue? (y/n/^C) ";
static const char ZipfileCorrupt[] =
    "error:  zipfile probably corrupt (%s)\n";
     static const char FileIsSymLink[] =
       "%s exists and is a symbolic link%s.\n";
#ifdef MORE
     static const char MorePrompt[] = "--More--(%lu)";
#endif
static const char QuitPrompt[] =
    "--- Press `Q' to quit, or any other key to continue ---";
static const char HidePrompt[] =  /* "\r                       \r"; */
    "\r                                                         \r";
#if CRYPT
    static const char PasswPrompt[] = "[%s] %s password: ";
    static const char PasswPrompt2[] = "Enter password: ";
    static const char PasswRetry[] = "password incorrect--reenter: ";
#endif /* CRYPT */





/******************************/
/* Function open_input_file() */
/******************************/

int open_input_file(__G)    /* return 1 if open failed */
    __GDEF
{
    /*
     *  open the zipfile for reading and in BINARY mode to prevent cr/lf
     *  translation, which would corrupt the bitstreams
     */

    G.zipfd = open(G.zipfn, O_RDONLY | O_BINARY);

    /* if (G.zipfd < 0) */  /* no good for Windows CE port */
    if (G.zipfd == -1)
    {
        Info(slide, 0x401, ((char *)slide, CannotOpenZipfile,
          G.zipfn, strerror(errno)));
        return 1;
    }
    return 0;

} /* end function open_input_file() */





/***************************/
/* Function open_outfile() */
/***************************/

int open_outfile(__G)           /* return 1 if fail */
    __GDEF
{
    if (zstat(G.filename, &G.statbuf) == 0 ||
        lstat(G.filename, &G.statbuf) == 0)
    {
        Trace((stderr, "open_outfile:  stat(%s) returns 0:  file exists\n",
          FnFilter1(G.filename)));
        if (uO.B_flag) {    /* do backup */
            char *tname;
            z_stat tmpstat;
            int blen, flen, tlen;

            blen = strlen(BackupSuffix);
            flen = strlen(G.filename);
            tlen = flen + blen + 6;    /* includes space for 5 digits */
            if (tlen >= FILNAMSIZ) {   /* in case name is too long, truncate */
                tname = (char *)malloc(FILNAMSIZ);
                if (tname == NULL)
                    return 1;                 /* in case we run out of space */
                tlen = FILNAMSIZ - 1 - blen;
                strcpy(tname, G.filename);    /* make backup name */
                tname[tlen] = '\0';
                if (flen > tlen) flen = tlen;
                tlen = FILNAMSIZ;
            } else {
                tname = (char *)malloc(tlen);
                if (tname == NULL)
                    return 1;                 /* in case we run out of space */
                strcpy(tname, G.filename);    /* make backup name */
            }
            strcpy(tname+flen, BackupSuffix);

            if (IS_OVERWRT_ALL) {
                /* If there is a previous backup file, delete it,
                 * otherwise the following rename operation may fail.
                 */
                if (zstat(tname, &tmpstat) == 0)
                    unlink(tname);
            } else {
                /* Check if backupname exists, and, if it's true, try
                 * appending numbers of up to 5 digits (or the maximum
                 * "unsigned int" number on 16-bit systems) to the
                 * BackupSuffix, until an unused name is found.
                 */
                unsigned maxtail, i;
                char *numtail = tname + flen + blen;

                /* take account of the "unsigned" limit on 16-bit systems: */
                maxtail = ( ((~0) >= 99999L) ? 99999 : (~0) );
                switch (tlen - flen - blen - 1) {
                    case 4: maxtail = 9999; break;
                    case 3: maxtail = 999; break;
                    case 2: maxtail = 99; break;
                    case 1: maxtail = 9; break;
                    case 0: maxtail = 0; break;
                }
                /* while filename exists */
                for (i = 0; (i < maxtail) && (zstat(tname, &tmpstat) == 0);)
                    sprintf(numtail,"%u", ++i);
            }

            if (rename(G.filename, tname) != 0) {   /* move file */
                Info(slide, 0x401, ((char *)slide,
                  CannotRenameOldFile,
                  FnFilter1(G.filename), strerror(errno)));
                free(tname);
                return 1;
            }
            Trace((stderr, "open_outfile:  %s now renamed into %s\n",
              FnFilter1(G.filename), FnFilter2(tname)));
            free(tname);
        } else
        {
            if (unlink(G.filename) != 0) {
                Info(slide, 0x401, ((char *)slide,
                  CannotDeleteOldFile,
                  FnFilter1(G.filename), strerror(errno)));
                return 1;
            }
            Trace((stderr, "open_outfile:  %s now deleted\n",
              FnFilter1(G.filename)));
        }
    }
#ifdef DEBUG
    Info(slide, 1, ((char *)slide,
      "open_outfile:  doing fopen(%s) for reading\n", FnFilter1(G.filename)));
    if ((G.outfile = zfopen(G.filename, FOPR)) == (FILE *)NULL)
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading failed:  does not exist\n",
          FnFilter1(G.filename)));
    else {
        Info(slide, 1, ((char *)slide,
          "open_outfile:  fopen(%s) for reading succeeded:  file exists\n",
          FnFilter1(G.filename)));
        fclose(G.outfile);
    }
#endif /* DEBUG */
    Trace((stderr, "open_outfile:  doing fopen(%s) for writing\n",
      FnFilter1(G.filename)));
    {
        mode_t umask_sav = umask(0077);
        /* These features require the ability to re-read extracted data from
           the output files. Output files are created with Read&Write access.
         */
        G.outfile = zfopen(G.filename, FOPWR);
        umask(umask_sav);
    }
    if (G.outfile == (FILE *)NULL) {
        Info(slide, 0x401, ((char *)slide, CannotCreateFile,
          FnFilter1(G.filename), strerror(errno)));
        return 1;
    }
    Trace((stderr, "open_outfile:  fopen(%s) for writing succeeded\n",
      FnFilter1(G.filename)));

#ifdef USE_FWRITE
#  ifdef _IOFBF  /* make output fully buffered (works just about like write()) */
    setvbuf(G.outfile, (char *)slide, _IOFBF, WSIZE);
#  else
    setbuf(G.outfile, (char *)slide);
#  endif
#endif /* USE_FWRITE */
    return 0;

} /* end function open_outfile() */





/*
 * These functions allow NEXTBYTE to function without needing two bounds
 * checks.  Call defer_leftover_input() if you ever have filled G.inbuf
 * by some means other than readbyte(), and you then want to start using
 * NEXTBYTE.  When going back to processing bytes without NEXTBYTE, call
 * undefer_input().  For example, extract_or_test_member brackets its
 * central section that does the decompression with these two functions.
 * If you need to check the number of bytes remaining in the current
 * file while using NEXTBYTE, check (G.csize + G.incnt), not G.csize.
 */

/****************************/
/* function undefer_input() */
/****************************/

void undefer_input(__G)
    __GDEF
{
    if (G.incnt > 0)
        G.csize += G.incnt;
    if (G.incnt_leftover > 0) {
        /* We know that "(G.csize < MAXINT)" so we can cast G.csize to int:
         * This condition was checked when G.incnt_leftover was set > 0 in
         * defer_leftover_input(), and it is NOT allowed to touch G.csize
         * before calling undefer_input() when (G.incnt_leftover > 0)
         * (single exception: see readbyte()'s  "G.csize <= 0" handling) !!
         */
        if (G.csize < 0L)
            G.csize = 0L;
        G.incnt = G.incnt_leftover + (int)G.csize;
        G.inptr = G.inptr_leftover - (int)G.csize;
        G.incnt_leftover = 0;
    } else if (G.incnt < 0)
        G.incnt = 0;
} /* end function undefer_input() */





/***********************************/
/* function defer_leftover_input() */
/***********************************/

void defer_leftover_input(__G)
    __GDEF
{
    if ((zoff_t)G.incnt > G.csize) {
        /* (G.csize < MAXINT), we can safely cast it to int !! */
        if (G.csize < 0L)
            G.csize = 0L;
        G.inptr_leftover = G.inptr + (int)G.csize;
        G.incnt_leftover = G.incnt - (int)G.csize;
        G.incnt = (int)G.csize;
    } else
        G.incnt_leftover = 0;
    G.csize -= G.incnt;
} /* end function defer_leftover_input() */





/**********************/
/* Function readbuf() */
/**********************/

unsigned readbuf(__G__ buf, size)   /* return number of bytes read into buf */
    __GDEF
    char *buf;
    unsigned size;
{
    unsigned count;
    unsigned n;

    n = size;
    while (size) {
        if (G.incnt <= 0) {
            if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0)
                return (n-size);
            else if (G.incnt < 0) {
                /* another hack, but no real harm copying same thing twice */
                (*G.message)((void *)&G,
                  (uch *)ReadError,  /* CANNOT use slide */
                  (ulg)strlen(ReadError), 0x401);
                return 0;  /* discarding some data; better than lock-up */
            }
            /* buffer ALWAYS starts on a block boundary:  */
            G.cur_zipfile_bufstart += INBUFSIZ;
            G.inptr = G.inbuf;
        }
        count = MIN(size, (unsigned)G.incnt);
        memcpy(buf, G.inptr, count);
        buf += count;
        G.inptr += count;
        G.incnt -= count;
        size -= count;
    }
    return n;

} /* end function readbuf() */





/***********************/
/* Function readbyte() */
/***********************/

int readbyte(__G)   /* refill inbuf and return a byte if available, else EOF */
    __GDEF
{
    if (G.mem_mode)
        return EOF;
    if (G.csize <= 0) {
        G.csize--;             /* for tests done after exploding */
        G.incnt = 0;
        return EOF;
    }
    if (G.incnt <= 0) {
        if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) == 0) {
            return EOF;
        } else if (G.incnt < 0) {  /* "fail" (abort, retry, ...) returns this */
            /* another hack, but no real harm copying same thing twice */
            (*G.message)((void *)&G,
              (uch *)ReadError,
              (ulg)strlen(ReadError), 0x401);
            echon();
            DESTROYGLOBALS();
            EXIT(PK_BADERR);    /* totally bailing; better than lock-up */
        }
        G.cur_zipfile_bufstart += INBUFSIZ; /* always starts on block bndry */
        G.inptr = G.inbuf;
        defer_leftover_input(__G);           /* decrements G.csize */
    }

#if CRYPT
    if (G.pInfo->encrypted) {
        uch *p;
        int n;

        /* This was previously set to decrypt one byte beyond G.csize, when
         * incnt reached that far.  GRR said, "but it's required:  why?"  This
         * was a bug in fillinbuf() -- was it also a bug here?
         */
        for (n = G.incnt, p = G.inptr;  n--;  p++)
            zdecode(*p);
    }
#endif /* CRYPT */

    --G.incnt;
    return *G.inptr++;

} /* end function readbyte() */





#if defined(USE_ZLIB) || defined(USE_BZIP2)

/************************/
/* Function fillinbuf() */
/************************/

int fillinbuf(__G) /* like readbyte() except returns number of bytes in inbuf */
    __GDEF
{
    if (G.mem_mode ||
                  (G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
        return 0;
    G.cur_zipfile_bufstart += INBUFSIZ;  /* always starts on a block boundary */
    G.inptr = G.inbuf;
    defer_leftover_input(__G);           /* decrements G.csize */

#  if CRYPT
    if (G.pInfo->encrypted) {
        uch *p;
        int n;

        for (n = G.incnt, p = G.inptr;  n--;  p++)
            zdecode(*p);
    }
#  endif /* CRYPT */

    return G.incnt;

} /* end function fillinbuf() */

#endif /* USE_ZLIB || USE_BZIP2 */





/************************/
/* Function seek_zipf() */
/************************/

int seek_zipf(__G__ abs_offset)
    __GDEF
    zoff_t abs_offset;
{
/*
 *  Seek to the block boundary of the block which includes abs_offset,
 *  then read block into input buffer and set pointers appropriately.
 *  If block is already in the buffer, just set the pointers.  This function
 *  is used by do_seekable (process.c), extract_or_test_entrylist (extract.c)
 *  and do_string (fileio.c).  Also, a slightly modified version is embedded
 *  within extract_or_test_entrylist (extract.c).  readbyte() and readbuf()
 *  (fileio.c) are compatible.  NOTE THAT abs_offset is intended to be the
 *  "proper offset" (i.e., if there were no extra bytes prepended);
 *  cur_zipfile_bufstart contains the corrected offset.
 *
 *  Since seek_zipf() is never used during decompression, it is safe to
 *  use the slide[] buffer for the error message.
 *
 * returns PK error codes:
 *  PK_BADERR if effective offset in zipfile is negative
 *  PK_EOF if seeking past end of zipfile
 *  PK_OK when seek was successful
 */
    zoff_t request = abs_offset + G.extra_bytes;
    zoff_t inbuf_offset = request % INBUFSIZ;
    zoff_t bufstart = request - inbuf_offset;

    if (request < 0) {
        Info(slide, 1, ((char *)slide, SeekMsg,
             G.zipfn, ReportMsg));
        return(PK_BADERR);
    } else if (bufstart != G.cur_zipfile_bufstart) {
        Trace((stderr,
          "fpos_zip: abs_offset = %s, G.extra_bytes = %s\n",
          FmZofft(abs_offset, NULL, NULL),
          FmZofft(G.extra_bytes, NULL, NULL)));
        G.cur_zipfile_bufstart = zlseek(G.zipfd, bufstart, SEEK_SET);
        Trace((stderr,
          "       request = %s, (abs+extra) = %s, inbuf_offset = %s\n",
          FmZofft(request, NULL, NULL),
          FmZofft((abs_offset+G.extra_bytes), NULL, NULL),
          FmZofft(inbuf_offset, NULL, NULL)));
        Trace((stderr, "       bufstart = %s, cur_zipfile_bufstart = %s\n",
          FmZofft(bufstart, NULL, NULL),
          FmZofft(G.cur_zipfile_bufstart, NULL, NULL)));
        if ((G.incnt = read(G.zipfd, (char *)G.inbuf, INBUFSIZ)) <= 0)
            return(PK_EOF);
        G.incnt -= (int)inbuf_offset;
        G.inptr = G.inbuf + (int)inbuf_offset;
    } else {
        G.incnt += (G.inptr-G.inbuf) - (int)inbuf_offset;
        G.inptr = G.inbuf + (int)inbuf_offset;
    }
    return(PK_OK);
} /* end function seek_zipf() */





/********************/
/* Function flush() */   /* returns PK error codes: */
/********************/   /* if tflag => always 0; PK_DISK if write error */

int flush(__G__ rawbuf, size, unshrink)
    __GDEF
    uch *rawbuf;
    ulg size;
    int unshrink;
{
    uch *p;
    uch *q;
    uch *transbuf;
#if (defined(VMS_TEXT_CONV))
    ulg transbufsiz;
#endif
    /* static int didCRlast = FALSE;    moved to globals.h */


/*---------------------------------------------------------------------------
    Compute the CRC first; if testing or if disk is full, that's it.
  ---------------------------------------------------------------------------*/

    G.crc32val = crc32(G.crc32val, rawbuf, (extent)size);

    if (uO.tflag || size == 0L)  /* testing or nothing to write:  all done */
        return PK_OK;

    if (G.disk_full)
        return PK_DISK;         /* disk already full:  ignore rest of file */

/*---------------------------------------------------------------------------
    Write the bytes rawbuf[0..size-1] to the output device, first converting
    end-of-lines and ASCII/EBCDIC as needed.  If SMALL_MEM or MED_MEM are NOT
    defined, outbuf is assumed to be at least as large as rawbuf and is not
    necessarily checked for overflow.
  ---------------------------------------------------------------------------*/

    if (!G.pInfo->textmode) {   /* write raw binary data */
        /* GRR:  note that for standard MS-DOS compilers, size argument to
         * fwrite() can never be more than 65534, so WriteError macro will
         * have to be rewritten if size can ever be that large.  For now,
         * never more than 32K.  Also note that write() returns an int, which
         * doesn't necessarily limit size to 32767 bytes if write() is used
         * on 16-bit systems but does make it more of a pain; however, because
         * at least MSC 5.1 has a lousy implementation of fwrite() (as does
         * DEC Ultrix cc), write() is used anyway.
         */
        if (!uO.cflag && WriteError(rawbuf, size, G.outfile))
            return disk_error(__G);
        else if (uO.cflag && (*G.message)((void *)&G, rawbuf, size, 0))
            return PK_OK;
    } else {   /* textmode:  aflag is true */
        if (unshrink) {
            /* rawbuf = outbuf */
            transbuf = G.outbuf2;
#if (defined(VMS_TEXT_CONV))
            transbufsiz = TRANSBUFSIZ;
#endif
        } else {
            /* rawbuf = slide */
            transbuf = G.outbuf;
#if (defined(VMS_TEXT_CONV))
            transbufsiz = OUTBUFSIZ;
            Trace((stderr, "\ntransbufsiz = OUTBUFSIZ = %u\n",
                   (unsigned)OUTBUFSIZ));
#endif
        }
        if (G.newfile) {
#ifdef VMS_TEXT_CONV
            if (G.pInfo->hostnum == VMS_ && G.extra_field &&
                is_vms_varlen_txt(__G__ G.extra_field,
                                  G.lrec.extra_field_length))
                G.VMS_line_state = 0;    /* 0: ready to read line length */
            else
                G.VMS_line_state = -1;   /* -1: don't treat as VMS text */
#endif
            G.didCRlast = FALSE;         /* no previous buffers written */
            G.newfile = FALSE;
        }

#ifdef VMS_TEXT_CONV
        if (G.VMS_line_state >= 0)
        {
            p = rawbuf;
            q = transbuf;
            while ((extent)(p-rawbuf) < (extent)size) {
                switch (G.VMS_line_state) {

                    /* 0: ready to read line length */
                    case 0:
                        G.VMS_line_length = 0;
                        if ((extent)(p-rawbuf) == (extent)size-1) {
                            /* last char */
                            G.VMS_line_length = (unsigned)(*p++);
                            G.VMS_line_state = 1;
                        } else {
                            G.VMS_line_length = makeword(p);
                            p += 2;
                            G.VMS_line_state = 2;
                        }
                        G.VMS_line_pad =
                               ((G.VMS_line_length & 1) != 0); /* odd */
                        break;

                    /* 1: read one byte of length, need second */
                    case 1:
                        G.VMS_line_length += ((unsigned)(*p++) << 8);
                        G.VMS_line_state = 2;
                        break;

                    /* 2: ready to read VMS_line_length chars */
                    case 2:
                        {
                            extent remaining = (extent)size+(rawbuf-p);
                            extent outroom;

                            if (G.VMS_line_length < remaining) {
                                remaining = G.VMS_line_length;
                                G.VMS_line_state = 3;
                            }

                            outroom = transbuf+(extent)transbufsiz-q;
                            if (remaining >= outroom) {
                                remaining -= outroom;
                                for (;outroom > 0; p++, outroom--)
                                    *q++ = native(*p);
                                if (!uO.cflag && WriteError(transbuf,
                                    (extent)(q-transbuf), G.outfile))
                                    return disk_error(__G);
                                else if (uO.cflag && (*G.message)((void *)&G,
                                         transbuf, (ulg)(q-transbuf), 0))
                                    return PK_OK;
                                q = transbuf;
                                /* fall through to normal case */
                            }
                            G.VMS_line_length -= remaining;
                            for (;remaining > 0; p++, remaining--)
                                *q++ = native(*p);
                        }
                        break;

                    /* 3: ready to PutNativeEOL */
                    case 3:
                        if (q > transbuf+(extent)transbufsiz-lenEOL) {
                            if (!uO.cflag &&
                                WriteError(transbuf, (extent)(q-transbuf),
                                  G.outfile))
                                return disk_error(__G);
                            else if (uO.cflag && (*G.message)((void *)&G,
                                     transbuf, (ulg)(q-transbuf), 0))
                                return PK_OK;
                            q = transbuf;
                        }
                        PutNativeEOL
                        G.VMS_line_state = G.VMS_line_pad ? 4 : 0;
                        break;

                    /* 4: ready to read pad byte */
                    case 4:
                        ++p;
                        G.VMS_line_state = 0;
                        break;
                }
            } /* end while */

        } else
#endif /* VMS_TEXT_CONV */

    /*-----------------------------------------------------------------------
        Algorithm:  CR/LF => native; lone CR => native; lone LF => native.
        This routine is only for non-raw-VMS, non-raw-VM/CMS files (i.e.,
        stream-oriented files, not record-oriented).
      -----------------------------------------------------------------------*/

        /* else not VMS text */ {
            p = rawbuf;
            if (*p == LF && G.didCRlast)
                ++p;
            G.didCRlast = FALSE;
            for (q = transbuf;  (extent)(p-rawbuf) < (extent)size;  ++p) {
                if (*p == CR) {           /* lone CR or CR/LF: treat as EOL  */
                    PutNativeEOL
                    if ((extent)(p-rawbuf) == (extent)size-1)
                        /* last char in buffer */
                        G.didCRlast = TRUE;
                    else if (p[1] == LF)  /* get rid of accompanying LF */
                        ++p;
                } else if (*p == LF)      /* lone LF */
                    PutNativeEOL
                else
                if (*p != CTRLZ)          /* lose all ^Z's */
                    *q++ = native(*p);
            }
        }

    /*-----------------------------------------------------------------------
        Done translating:  write whatever we've got to file (or screen).
      -----------------------------------------------------------------------*/

        Trace((stderr, "p - rawbuf = %u   q-transbuf = %u   size = %lu\n",
          (unsigned)(p-rawbuf), (unsigned)(q-transbuf), size));
        if (q > transbuf) {
            if (!uO.cflag && WriteError(transbuf, (extent)(q-transbuf),
                G.outfile))
                return disk_error(__G);
            else if (uO.cflag && (*G.message)((void *)&G, transbuf,
                (ulg)(q-transbuf), 0))
                return PK_OK;
        }
    }

    return PK_OK;

} /* end function flush() [resp. partflush() for 16-bit Deflate64 support] */





#ifdef VMS_TEXT_CONV

/********************************/
/* Function is_vms_varlen_txt() */
/********************************/

static int is_vms_varlen_txt(__G__ ef_buf, ef_len)
    __GDEF
    uch *ef_buf;        /* buffer containing extra field */
    unsigned ef_len;    /* total length of extra field */
{
    unsigned eb_id;
    unsigned eb_len;
    uch *eb_data;
    unsigned eb_datlen;
#  define VMSREC_C_UNDEF  0
#  define VMSREC_C_VAR    2
    uch vms_rectype = VMSREC_C_UNDEF;
 /* uch vms_fileorg = 0; */ /* currently, fileorg is not used... */

#  define VMSPK_ITEMID            0
#  define VMSPK_ITEMLEN           2
#  define VMSPK_ITEMHEADSZ        4

#  define VMSATR_C_RECATTR        4
#  define VMS_FABSIG              0x42414656      /* "VFAB" */
/* offsets of interesting fields in VMS fabdef structure */
#  define VMSFAB_B_RFM            31      /* record format byte */
#  define VMSFAB_B_ORG            29      /* file organization byte */

    if (ef_len == 0 || ef_buf == NULL)
        return FALSE;

    while (ef_len >= EB_HEADSIZE) {
        eb_id = makeword(EB_ID + ef_buf);
        eb_len = makeword(EB_LEN + ef_buf);

        if (eb_len > (ef_len - EB_HEADSIZE)) {
            /* discovered some extra field inconsistency! */
            Trace((stderr,
              "is_vms_varlen_txt: block length %u > rest ef_size %u\n", eb_len,
              ef_len - EB_HEADSIZE));
            break;
        }

        switch (eb_id) {
          case EF_PKVMS:
            /* The PKVMS e.f. raw data part consists of:
             * a) 4 bytes CRC checksum
             * b) list of uncompressed variable-length data items
             * Each data item is introduced by a fixed header
             *  - 2 bytes data type ID
             *  - 2 bytes <size> of data
             *  - <size> bytes of actual attribute data
             */

            /* get pointer to start of data and its total length */
            eb_data = ef_buf+(EB_HEADSIZE+4);
            eb_datlen = eb_len-4;

            /* test the CRC checksum */
            if (makelong(ef_buf+EB_HEADSIZE) !=
                crc32(CRCVAL_INITIAL, eb_data, (extent)eb_datlen))
            {
                Info(slide, 1, ((char *)slide,
                  "[Warning: CRC error, discarding PKWARE extra field]\n"));
                /* skip over the data analysis code */
                break;
            }

            /* scan through the attribute data items */
            while (eb_datlen > 4)
            {
                unsigned fldsize = makeword(&eb_data[VMSPK_ITEMLEN]);

                /* check the item type word */
                switch (makeword(&eb_data[VMSPK_ITEMID])) {
                  case VMSATR_C_RECATTR:
                    /* we have found the (currently only) interesting
                     * data item */
                    if (fldsize >= 1) {
                        vms_rectype = eb_data[VMSPK_ITEMHEADSZ] & 15;
                     /* vms_fileorg = eb_data[VMSPK_ITEMHEADSZ] >> 4; */
                    }
                    break;
                  default:
                    break;
                }
                /* skip to next data item */
                eb_datlen -= fldsize + VMSPK_ITEMHEADSZ;
                eb_data += fldsize + VMSPK_ITEMHEADSZ;
            }
            break;

          case EF_IZVMS:
            if (makelong(ef_buf+EB_HEADSIZE) == VMS_FABSIG) {
                if ((eb_data = extract_izvms_block(__G__
                                                   ef_buf+EB_HEADSIZE, eb_len,
                                                   &eb_datlen, NULL, 0))
                    != NULL)
                {
                    if (eb_datlen >= VMSFAB_B_RFM+1) {
                        vms_rectype = eb_data[VMSFAB_B_RFM] & 15;
                     /* vms_fileorg = eb_data[VMSFAB_B_ORG] >> 4; */
                    }
                    free(eb_data);
                }
            }
            break;

          default:
            break;
        }

        /* Skip this extra field block */
        ef_buf += (eb_len + EB_HEADSIZE);
        ef_len -= (eb_len + EB_HEADSIZE);
    }

    return (vms_rectype == VMSREC_C_VAR);

} /* end function is_vms_varlen_txtfile() */

#endif /* VMS_TEXT_CONV */




/*************************/
/* Function disk_error() */
/*************************/

int disk_error(__G)
    __GDEF
{
    /* OK to use slide[] here because this file is finished regardless */
    Info(slide, 0x4a1, ((char *)slide, DiskFullQuery, FnFilter1(G.filename)));

    if ( fgets(G.answerbuf, sizeof(G.answerbuf), stdin) &&
         *G.answerbuf == 'y' )  /* stop writing to this file */
        G.disk_full = 1;        /* (outfile bad?), but new OK */
    else
        G.disk_full = 2;        /* no:  exit program */

    return PK_DISK;

} /* end function disk_error() */






/*****************************/
/* Function UzpMessagePrnt() */
/*****************************/

int UZ_EXP UzpMessagePrnt(pG, buf, size, flag)
    void *pG;    /* globals struct:  always passed */
    uch *buf;    /* preformatted string to be printed */
    ulg size;    /* length of string (may include nulls) */
    int flag;    /* flag bits */
{
    /* IMPORTANT NOTE:
     *    The name of the first parameter of UzpMessagePrnt(), which passes
     *    the "Uz_Globs" address, >>> MUST <<< be identical to the string
     *    expansion of the __G__ macro in the REENTRANT case (see globals.h).
     *    This name identity is mandatory for the  macro
     *    (in the SMALL_MEM case) !!!
     */
    int error;
    uch *q=buf, *endbuf=buf+(unsigned)size;
#ifdef MORE
    uch *p=buf;
#  if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    int islinefeed = FALSE;
#  endif
#endif
    FILE *outfp;


/*---------------------------------------------------------------------------
    These tests are here to allow fine-tuning of UnZip's output messages,
    but none of them will do anything without setting the appropriate bit
    in the flag argument of every Info() statement which is to be turned
    *off*.  That is, all messages are currently turned on for all ports.
  ---------------------------------------------------------------------------*/

    if (MSG_STDERR(flag) && !((Uz_Globs *)pG)->UzO.tflag)
        outfp = (FILE *)stderr;
    else
        outfp = (FILE *)stdout;

#ifdef QUERY_TRNEWLN
    /* some systems require termination of query prompts with '\n' to force
     * immediate display */
    if (MSG_MNEWLN(flag)) {   /* assumes writable buffer (e.g., slide[]) */
        *endbuf++ = '\n';     /*  with room for one more char at end of buf */
        ++size;               /*  (safe assumption:  only used for four */
    }                         /*  short queries in extract.c and fileio.c) */
#endif

    if (MSG_TNEWLN(flag)) {   /* again assumes writable buffer:  fragile... */
        if ((!size && !((Uz_Globs *)pG)->sol) ||
            (size && (endbuf[-1] != '\n')))
        {
            *endbuf++ = '\n';
            ++size;
        }
    }

#ifdef MORE
#  ifdef SCREENSIZE
    /* room for --More-- and one line of overlap: */
#    if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    SCREENSIZE(&((Uz_Globs *)pG)->height, &((Uz_Globs *)pG)->width);
#    else
    SCREENSIZE(&((Uz_Globs *)pG)->height, (int *)NULL);
#    endif
    ((Uz_Globs *)pG)->height -= 2;
#  else
    /* room for --More-- and one line of overlap: */
    ((Uz_Globs *)pG)->height = SCREENLINES - 2;
#    if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
    ((Uz_Globs *)pG)->width = SCREENWIDTH;
#    endif
#  endif
#endif /* MORE */

    if (MSG_LNEWLN(flag) && !((Uz_Globs *)pG)->sol) {
        /* not at start of line:  want newline */
            putc('\n', outfp);
            fflush(outfp);
#ifdef MORE
            if (((Uz_Globs *)pG)->M_flag)
            {
#  if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
                ((Uz_Globs *)pG)->chars = 0;
#  endif
                ++((Uz_Globs *)pG)->numlines;
                ++((Uz_Globs *)pG)->lines;
                if (((Uz_Globs *)pG)->lines >= ((Uz_Globs *)pG)->height)
                    (*((Uz_Globs *)pG)->mpause)((void *)pG,
                      MorePrompt, 1);
            }
#endif /* MORE */
            if (MSG_STDERR(flag) && ((Uz_Globs *)pG)->UzO.tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                putc('\n', stderr);
                fflush(stderr);
            }
        ((Uz_Globs *)pG)->sol = TRUE;
    }

    /* put zipfile name, filename and/or error/warning keywords here */

#ifdef MORE
    if (((Uz_Globs *)pG)->M_flag)
    {
        while (p < endbuf) {
            if (*p == '\n') {
#  if (defined(SCREENWIDTH) && defined(SCREENLWRAP))
                islinefeed = TRUE;
            } else if (SCREENLWRAP) {
                if (*p == '\r') {
                    ((Uz_Globs *)pG)->chars = 0;
                } else {
#    ifdef TABSIZE
                    if (*p == '\t')
                        ((Uz_Globs *)pG)->chars +=
                            (TABSIZE - (((Uz_Globs *)pG)->chars % TABSIZE));
                    else
#    endif
                        ++((Uz_Globs *)pG)->chars;

                    if (((Uz_Globs *)pG)->chars >= ((Uz_Globs *)pG)->width)
                        islinefeed = TRUE;
                }
            }
            if (islinefeed) {
                islinefeed = FALSE;
                ((Uz_Globs *)pG)->chars = 0;
#  endif /* (SCREENWIDTH && SCREEN_LWRAP) */
                ++((Uz_Globs *)pG)->numlines;
                ++((Uz_Globs *)pG)->lines;
                if (((Uz_Globs *)pG)->lines >= ((Uz_Globs *)pG)->height)
                {
                    if ((error = WriteTxtErr(q, p-q+1, outfp)) != 0)
                        return error;
                    fflush(outfp);
                    ((Uz_Globs *)pG)->sol = TRUE;
                    q = p + 1;
                    (*((Uz_Globs *)pG)->mpause)((void *)pG,
                      MorePrompt, 1);
                }
            }
            INCSTR(p);
        } /* end while */
        size = (ulg)(p - q);   /* remaining text */
    }
#endif /* MORE */

    if (size) {
            if ((error = WriteTxtErr(q, size, outfp)) != 0)
                return error;
            fflush(outfp);
            if (MSG_STDERR(flag) && ((Uz_Globs *)pG)->UzO.tflag &&
                !isatty(1) && isatty(2))
            {
                /* error output from testing redirected:  also send to stderr */
                if ((error = WriteTxtErr(q, size, stderr)) != 0)
                    return error;
                fflush(stderr);
            }
        ((Uz_Globs *)pG)->sol = (endbuf[-1] == '\n');
    }
    return 0;

} /* end function UzpMessagePrnt() */





/***********************/
/* Function UzpInput() */   /* GRR:  this is a placeholder for now */
/***********************/

int UZ_EXP UzpInput(pG, buf, size, flag)
    void *pG;     /* globals struct:  always passed */
    uch *buf;     /* preformatted string to be printed */
    int *size;    /* (address of) size of buf and of returned string */
    int flag;     /* flag bits (bit 0: no echo) */
{
    /* tell picky compilers to shut up about "unused variable" warnings */
    (void)pG; (void)buf; (void)flag;

    *size = 0;
    return 0;

} /* end function UzpInput() */





/***************************/
/* Function UzpMorePause() */
/***************************/

void UZ_EXP UzpMorePause(pG, prompt, flag)
    void *pG;             /* globals struct:  always passed */
    const char *prompt;   /* "--More--" prompt */
    int flag;             /* 0 = any char OK; 1 = accept only '\n', ' ', q */
{
    uch c;

/*---------------------------------------------------------------------------
    Print a prompt and wait for the user to press a key, then erase prompt
    if possible.
  ---------------------------------------------------------------------------*/

    if (!((Uz_Globs *)pG)->sol)
        fprintf(stderr, "\n");
    /* numlines may or may not be used: */
    fprintf(stderr, prompt, ((Uz_Globs *)pG)->numlines);
    fflush(stderr);
    if (flag & 1) {
        do {
            c = (uch)FGETCH(0);
        } while (c != '\r' && c != '\n' && c != ' ' && c != 'q' && c != 'Q');
    } else
        c = (uch)FGETCH(0);

    /* newline was not echoed, so cover up prompt line */
    fprintf(stderr, HidePrompt);
    fflush(stderr);

    if (tolower(c) == 'q') {
        DESTROYGLOBALS();
        EXIT(PK_COOL);
    }

    ((Uz_Globs *)pG)->sol = TRUE;

#ifdef MORE
    /* space for another screen, enter for another line. */
    if ((flag & 1) && c == ' ')
        ((Uz_Globs *)pG)->lines = 0;
#endif /* MORE */

} /* end function UzpMorePause() */





/**************************/
/* Function UzpPassword() */
/**************************/

#define PROMPT_MAX_LEN (2*FILNAMSIZ + 15)

int UZ_EXP UzpPassword (pG, rcnt, pwbuf, size, zfn, efn)
    void *pG;          /* pointer to UnZip's internal global vars */
    int *rcnt;         /* retry counter */
    char *pwbuf;       /* buffer for password */
    int size;          /* size of password buffer */
    const char *zfn;   /* name of zip archive */
    const char *efn;   /* name of archive entry being processed */
{
#if CRYPT
    int r = IZ_PW_ENTERED;
    const char *m;
    char *prompt;

#  ifndef REENTRANT
    /* tell picky compilers to shut up about "unused variable" warnings */
    (void)pG;
#  endif

    if (*rcnt == 0) {           /* First call for current entry */
        *rcnt = 2;
        if ( (prompt = (char *)malloc(PROMPT_MAX_LEN)) != NULL &&
             (size_t) snprintf( prompt, PROMPT_MAX_LEN, PasswPrompt,
                                FnFilter1(zfn), FnFilter2(efn) )
                < PROMPT_MAX_LEN) {
            m = prompt;
        } else {
            m = PasswPrompt2;
        }
    } else {                    /* Retry call, previous password was wrong */
        (*rcnt)--;
        prompt = NULL;
        m = PasswRetry;
    }

    m = getp(__G__ m, pwbuf, size);
    free(prompt);
    if (m == NULL) {
        r = IZ_PW_ERROR;
    } else if (*pwbuf == '\0') {
        r = IZ_PW_CANCELALL;
    }
    return r;

#else /* !CRYPT */
    /* tell picky compilers to shut up about "unused variable" warnings */
    (void)pG; (void)rcnt; (void)pwbuf; (void)size; (void)zfn; (void)efn;

    return IZ_PW_ERROR;  /* internal error; function should never get called */
#endif /* ?CRYPT */

} /* end function UzpPassword() */





/**********************/
/* Function handler() */
/**********************/

void handler(signal)   /* upon interrupt, turn on echo and exit cleanly */
    int signal;
{
    GETGLOBALS();

#if !(defined(SIGBUS) || defined(SIGSEGV))      /* add a newline if not at */
    (*G.message)((void *)&G, slide, 0L, 0x41);  /*  start of line (to stderr; */
#endif                                          /*  slide[] should be safe) */

    echon();

#ifdef SIGBUS
    if (signal == SIGBUS) {
        Info(slide, 0x421, ((char *)slide, ZipfileCorrupt,
          "bus error"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
#endif /* SIGBUS */

#ifdef SIGILL
    if (signal == SIGILL) {
        Info(slide, 0x421, ((char *)slide, ZipfileCorrupt,
          "illegal instruction"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
#endif /* SIGILL */

#ifdef SIGSEGV
    if (signal == SIGSEGV) {
        Info(slide, 0x421, ((char *)slide, ZipfileCorrupt,
          "segmentation violation"));
        DESTROYGLOBALS();
        EXIT(PK_BADERR);
    }
#endif /* SIGSEGV */

    /* probably ctrl-C */
    DESTROYGLOBALS();
    EXIT(IZ_CTRLC);       /* was EXIT(0), then EXIT(PK_ERR) */
}





#if (!defined(HAVE_MKTIME))
/* also used in amiga/filedate.c and win32/win32.c */
const ush ydays[] =
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
#endif

/*******************************/
/* Function dos_to_unix_time() */ /* used for freshening/updating/timestamps */
/*******************************/

time_t dos_to_unix_time(dosdatetime)
    ulg dosdatetime;
{
    time_t m_time;

#ifdef HAVE_MKTIME

    const time_t now = time(NULL);
    struct tm *tm;
#  define YRBASE  1900

    tm = localtime(&now);
    tm->tm_isdst = -1;          /* let mktime determine if DST is in effect */

    /* dissect date */
    tm->tm_year = ((int)(dosdatetime >> 25) & 0x7f) + (1980 - YRBASE);
    tm->tm_mon  = ((int)(dosdatetime >> 21) & 0x0f) - 1;
    tm->tm_mday = ((int)(dosdatetime >> 16) & 0x1f);

    /* dissect time */
    tm->tm_hour = (int)((unsigned)dosdatetime >> 11) & 0x1f;
    tm->tm_min  = (int)((unsigned)dosdatetime >> 5) & 0x3f;
    tm->tm_sec  = (int)((unsigned)dosdatetime << 1) & 0x3e;

    m_time = mktime(tm);
    NATIVE_TO_TIMET(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    TTrace((stderr, "  final m_time  =       %lu\n", (ulg)m_time));

#else /* !HAVE_MKTIME */

    int yr, mo, dy, hh, mm, ss;
#  define YRBASE  1970
    int leap;
    unsigned days;
    struct tm *tm;
#  ifndef BSD4_4
#    if (defined(BSD))
    struct timeb tbp;
#    else /* !(BSD) */
#      ifdef DECLARE_TIMEZONE
    extern time_t timezone;
#      endif
#    endif /* ?(BSD) */
#  endif /* !BSD4_4 */

    /* dissect date */
    yr = ((int)(dosdatetime >> 25) & 0x7f) + (1980 - YRBASE);
    mo = ((int)(dosdatetime >> 21) & 0x0f) - 1;
    dy = ((int)(dosdatetime >> 16) & 0x1f) - 1;

    /* dissect time */
    hh = (int)((unsigned)dosdatetime >> 11) & 0x1f;
    mm = (int)((unsigned)dosdatetime >> 5) & 0x3f;
    ss = (int)((unsigned)dosdatetime & 0x1f) * 2;

/*---------------------------------------------------------------------------
    Calculate the number of seconds since the epoch, usually 1 January 1970.
  ---------------------------------------------------------------------------*/

    /* leap = # of leap yrs from YRBASE up to but not including current year */
    leap = ((yr + YRBASE - 1) / 4);   /* leap year base factor */

    /* calculate days from BASE to this year and add expired days this year */
    days = (yr * 365) + (leap - 492) + ydays[mo];

    /* if year is a leap year and month is after February, add another day */
    if ((mo > 1) && ((yr+YRBASE)%4 == 0) && ((yr+YRBASE) != 2100))
        ++days;                 /* OK through 2199 */

    /* convert date & time to seconds relative to 00:00:00, 01/01/YRBASE */
    m_time = (time_t)((unsigned long)(days + dy) * 86400L +
                      (unsigned long)hh * 3600L +
                      (unsigned long)(mm * 60 + ss));
      /* - 1;   MS-DOS times always rounded up to nearest even second */
    TTrace((stderr, "dos_to_unix_time:\n"));
    TTrace((stderr, "  m_time before timezone = %lu\n", (ulg)m_time));

/*---------------------------------------------------------------------------
    Adjust for local standard timezone offset.
  ---------------------------------------------------------------------------*/

#  if (defined(BSD))
#    ifdef BSD4_4
    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */
    if ((tm = localtime(&m_time)) != (struct tm *)NULL)
        m_time -= tm->tm_gmtoff;                /* sec. EAST of GMT: subtr. */
#    else /* !(BSD4_4 */
    ftime(&tbp);                                /* get `timezone' */
    m_time += tbp.timezone * 60L;               /* seconds WEST of GMT:  add */
#    endif /* ?(BSD4_4) */
#  else /* !(BSD) */
    /* tzset was already called at start of process_zipfiles() */
    /* tzset(); */              /* set `timezone' variable */
    m_time += timezone;         /* seconds WEST of GMT:  add */
#  endif /* ?(BSD) */
    TTrace((stderr, "  m_time after timezone =  %lu\n", (ulg)m_time));

/*---------------------------------------------------------------------------
    Adjust for local daylight savings (summer) time.
  ---------------------------------------------------------------------------*/

#  ifndef BSD4_4  /* (DST already added to tm_gmtoff, so skip tm_isdst) */
    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */
    TIMET_TO_NATIVE(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    if (((tm = localtime((time_t *)&m_time)) != NULL) && tm->tm_isdst)
        m_time -= 60L * 60L;    /* adjust for daylight savings time */
    NATIVE_TO_TIMET(m_time)     /* NOP unless MSC 7.0 or Macintosh */
    TTrace((stderr, "  m_time after DST =       %lu\n", (ulg)m_time));
#  endif /* !BSD4_4 */

#endif /* ?HAVE_MKTIME */

    if ( (dosdatetime >= DOSTIME_2038_01_18) &&
         (m_time < (time_t)0x70000000L) )
        m_time = U_TIME_T_MAX;  /* saturate in case of (unsigned) overflow */
    if (m_time < (time_t)0L)    /* a converted DOS time cannot be negative */
        m_time = S_TIME_T_MAX;  /*  -> saturate at max signed time_t value */

    return m_time;

} /* end function dos_to_unix_time() */





/******************************/
/* Function check_for_newer() */  /* used for overwriting/freshening/updating */
/******************************/

int check_for_newer(__G__ filename)  /* return 1 if existing file is newer */
    __GDEF                           /*  or equal; 0 if older; -1 if doesn't */
    char *filename;                  /*  exist yet */
{
    time_t existing, archive;
    iztimes z_utime;

    Trace((stderr, "check_for_newer:  doing stat(%s)\n", FnFilter1(filename)));
    if (zstat(filename, &G.statbuf)) {
        Trace((stderr,
          "check_for_newer:  stat(%s) returns %d:  file does not exist\n",
          FnFilter1(filename), zstat(filename, &G.statbuf)));
        Trace((stderr, "check_for_newer:  doing lstat(%s)\n",
          FnFilter1(filename)));
        /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
        if (lstat(filename, &G.statbuf) == 0) {
            Trace((stderr,
              "check_for_newer:  lstat(%s) returns 0:  symlink does exist\n",
              FnFilter1(filename)));
            if (!uO.qflag && !IS_OVERWRT_ALL)
                Info(slide, 0, ((char *)slide, FileIsSymLink,
                  FnFilter1(filename), " with no real file"));
            return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
        }
        return DOES_NOT_EXIST;
    }
    Trace((stderr, "check_for_newer:  stat(%s) returns 0:  file exists\n",
      FnFilter1(filename)));

    /* GRR OPTION:  could instead do this test ONLY if G.symlnk is true */
    if (lstat(filename, &G.statbuf) == 0 && S_ISLNK(G.statbuf.st_mode)) {
        Trace((stderr, "check_for_newer:  %s is a symbolic link\n",
          FnFilter1(filename)));
        if (!uO.qflag && !IS_OVERWRT_ALL)
            Info(slide, 0, ((char *)slide, FileIsSymLink,
              FnFilter1(filename), ""));
        return EXISTS_AND_OLDER;   /* symlink dates are meaningless */
    }

    NATIVE_TO_TIMET(G.statbuf.st_mtime)   /* NOP unless MSC 7.0 or Macintosh */

    /* The `Unix extra field mtime' should be used for comparison with the
     * time stamp of the existing file >>>ONLY<<< when the EF info is also
     * used to set the modification time of the extracted file.
     */
    if (G.extra_field &&
#ifdef IZ_CHECK_TZ
        G.tz_is_valid &&
#endif
        (ef_scan_for_izux(G.extra_field, G.lrec.extra_field_length, 0,
                          G.lrec.last_mod_dos_datetime, &z_utime, NULL)
         & EB_UT_FL_MTIME))
    {
        TTrace((stderr, "check_for_newer:  using Unix extra field mtime\n"));
        existing = G.statbuf.st_mtime;
        archive  = z_utime.mtime;
    } else {
        /* round up existing filetime to nearest 2 seconds for comparison,
         * but saturate in case of arithmetic overflow
         */
        existing = ((G.statbuf.st_mtime & 1) &&
                    (G.statbuf.st_mtime + 1 > G.statbuf.st_mtime)) ?
                   G.statbuf.st_mtime + 1 : G.statbuf.st_mtime;
        archive  = dos_to_unix_time(G.lrec.last_mod_dos_datetime);
    }

    TTrace((stderr, "check_for_newer:  existing %lu, archive %lu, e-a %ld\n",
      (ulg)existing, (ulg)archive, (long)(existing-archive)));

    return (existing >= archive);

} /* end function check_for_newer() */





/************************/
/* Function do_string() */
/************************/

int do_string(__G__ length, option)   /* return PK-type error code */
    __GDEF
    unsigned int length;        /* without prototype, ush converted to this */
    int option;
{
    unsigned comment_bytes_left;
    unsigned int block_len;
    int error=PK_OK;
    unsigned int length2;


/*---------------------------------------------------------------------------
    This function processes arbitrary-length (well, usually) strings.  Four
    major options are allowed:  SKIP, wherein the string is skipped (pretty
    logical, eh?); DISPLAY, wherein the string is printed to standard output
    after undergoing any necessary or unnecessary character conversions;
    DS_FN, wherein the string is put into the filename[] array after under-
    going appropriate conversions (including case-conversion, if that is
    indicated: see the global variable pInfo->lcflag); and EXTRA_FIELD,
    wherein the `string' is assumed to be an extra field and is copied to
    the (freshly malloced) buffer G.extra_field.  The third option should
    be OK since filename is dimensioned at 1025, but we check anyway.

    The string, by the way, is assumed to start at the current file-pointer
    position; its length is given by 'length'.  So start off by checking the
    length of the string:  if zero, we're already done.
  ---------------------------------------------------------------------------*/

    if (!length)
        return PK_COOL;

    switch (option) {

#if (defined(SFX) && defined(CHEAP_SFX_AUTORUN))
    /*
     * Special case: See if the comment begins with an autorun command line.
     * Save that and display (or skip) the remainder.
     */

    case CHECK_AUTORUN:
    case CHECK_AUTORUN_Q:
        comment_bytes_left = length;
        if (length >= 10)
        {
            block_len = readbuf(__G__ (char *)G.outbuf, 10);
            if (block_len == 0)
                return PK_EOF;
            comment_bytes_left -= block_len;
            G.outbuf[block_len] = '\0';
            if (!strcmp((char *)G.outbuf, "$AUTORUN$>")) {
                char *eol;
                length -= 10;
                block_len = readbuf(__G__ G.autorun_command,
                                    MIN(length, sizeof(G.autorun_command)-1));
                if (block_len == 0)
                    return PK_EOF;
                comment_bytes_left -= block_len;
                G.autorun_command[block_len] = '\0';
                A_TO_N(G.autorun_command);
                eol = strchr(G.autorun_command, '\n');
                if (!eol)
                    eol = G.autorun_command + strlen(G.autorun_command) - 1;
                length -= eol + 1 - G.autorun_command;
                while (eol >= G.autorun_command && isspace(*eol))
                    *eol-- = '\0';
            }
        }
        if (option == CHECK_AUTORUN_Q)  /* don't display the remainder */
            length = 0;
        /* seek to beginning of remaining part of comment -- rewind if */
        /* displaying entire comment, or skip to end if discarding it  */
        seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                  (G.inptr - G.inbuf) + comment_bytes_left - length);
        if (!length)
            break;
        /*  FALL THROUGH...  */
#endif /* SFX && CHEAP_SFX_AUTORUN */

    /*
     * First normal case:  print string on standard output.  First set loop
     * variables, then loop through the comment in chunks of OUTBUFSIZ bytes,
     * converting formats and printing as we go.  The second half of the
     * loop conditional was added because the file might be truncated, in
     * which case comment_bytes_left will remain at some non-zero value for
     * all time.  outbuf and slide are used as scratch buffers because they
     * are available (we should be either before or in between any file pro-
     * cessing).
     */

    case DISPLAY:
    case DISPL_8:
        comment_bytes_left = length;
        block_len = OUTBUFSIZ;       /* for the while statement, first time */
        while (comment_bytes_left > 0 && block_len > 0) {
            uch *p = G.outbuf;
            uch *q = G.outbuf;

            if ((block_len = readbuf(__G__ (char *)G.outbuf,
                   MIN((unsigned)OUTBUFSIZ, comment_bytes_left))) == 0)
                return PK_EOF;
            comment_bytes_left -= block_len;

            /* this is why we allocated an extra byte for outbuf:  terminate
             *  with zero (ASCIIZ) */
            G.outbuf[block_len] = '\0';

            /* remove all ASCII carriage returns from comment before printing
             * (since used before A_TO_N(), check for CR instead of '\r')
             */
            while (*p) {
                while (*p == CR)
                    ++p;
                *q++ = *p++;
            }
            /* could check whether (p - outbuf) == block_len here */
            *q = '\0';

            if (option == DISPL_8) {
                /* translate the text coded in the entry's host-dependent
                   "extended ASCII" charset into the compiler's (system's)
                   internal text code page */
                Ext_ASCII_TO_Native((char *)G.outbuf, G.pInfo->hostnum,
                                    G.pInfo->hostver, G.pInfo->HasUxAtt,
                                    FALSE);
            } else {
                A_TO_N(G.outbuf);   /* translate string to native */
            }

#ifdef NOANSIFILT       /* GRR:  can ANSI be used with EBCDIC? */
            (*G.message)((void *)&G, G.outbuf, (ulg)(q-G.outbuf), 0);
#else /* ASCII, filter out ANSI escape sequences and handle ^S (pause) */
            p = G.outbuf - 1;
            q = slide;
            while (*++p) {
                int pause = FALSE;

                if (*p == 0x1B) {          /* ASCII escape char */
                    *q++ = '^';
                    *q++ = '[';
                } else if (*p == 0x13) {   /* ASCII ^S (pause) */
                    pause = TRUE;
                    if (p[1] == LF)        /* ASCII LF */
                        *q++ = *++p;
                    else if (p[1] == CR && p[2] == LF) {  /* ASCII CR LF */
                        *q++ = *++p;
                        *q++ = *++p;
                    }
                } else
                    *q++ = *p;
                if ((unsigned)(q-slide) > WSIZE-3 || pause) {   /* flush */
                    (*G.message)((void *)&G, slide, (ulg)(q-slide), 0);
                    q = slide;
                    if (pause && G.extract_flag) /* don't pause for list/test */
                        (*G.mpause)((void *)&G, QuitPrompt, 0);
                }
            }
            (*G.message)((void *)&G, slide, (ulg)(q-slide), 0);
#endif /* ?NOANSIFILT */
        }
        /* add '\n' if not at start of line */
        (*G.message)((void *)&G, slide, 0L, 0x40);
        break;

    /*
     * Second case:  read string into filename[] array.  The filename should
     * never ever be longer than FILNAMSIZ-1 (1024), but for now we'll check,
     * just to be sure.
     */

    case DS_FN:
    case DS_FN_L:
        /* get the whole filename as need it for Unicode checksum */
        if (G.fnfull_bufsize <= length) {
            extent fnbufsiz = FILNAMSIZ;

            if (fnbufsiz <= length)
                fnbufsiz = length + 1;
            if (G.filename_full)
                free(G.filename_full);
            G.filename_full = malloc(fnbufsiz);
            if (G.filename_full == NULL)
                return PK_MEM;
            G.fnfull_bufsize = fnbufsiz;
        }
        if (readbuf(__G__ G.filename_full, length) == 0)
            return PK_EOF;
        G.filename_full[length] = '\0';      /* terminate w/zero:  ASCIIZ */

        /* if needed, chop off end so standard filename is a valid length */
        if (length >= FILNAMSIZ) {
            Info(slide, 0x401, ((char *)slide,
              FilenameTooLongTrunc));
            error = PK_WARN;
            length = FILNAMSIZ - 1;
        }
        /* no excess size */
        block_len = 0;
        strncpy(G.filename, G.filename_full, length);
        G.filename[length] = '\0';      /* terminate w/zero:  ASCIIZ */

        /* translate the Zip entry filename coded in host-dependent "extended
           ASCII" into the compiler's (system's) internal text code page */
        Ext_ASCII_TO_Native(G.filename, G.pInfo->hostnum, G.pInfo->hostver,
                            G.pInfo->HasUxAtt, (option == DS_FN_L));

        if (G.pInfo->lcflag)      /* replace with lowercase filename */
            STRLOWER(G.filename, G.filename);

        if (G.pInfo->vollabel && length > 8 && G.filename[8] == '.') {
            char *p = G.filename+8;
            while (*p++)
                p[-1] = *p;  /* disk label, and 8th char is dot:  remove dot */
        }

        if (!block_len)         /* no overflow, we're done here */
            break;

        /*
         * We truncated the filename, so print what's left and then fall
         * through to the SKIP routine.
         */
        Info(slide, 0x401, ((char *)slide, "[ %s ]\n", FnFilter1(G.filename)));
        length = block_len;     /* SKIP the excess bytes... */
        /*  FALL THROUGH...  */

    /*
     * Third case:  skip string, adjusting readbuf's internal variables
     * as necessary (and possibly skipping to and reading a new block of
     * data).
     */

    case SKIP:
        /* cur_zipfile_bufstart already takes account of extra_bytes, so don't
         * correct for it twice: */
        seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                  (G.inptr-G.inbuf) + length);
        break;

    /*
     * Fourth case:  assume we're at the start of an "extra field"; malloc
     * storage for it and read data into the allocated space.
     */

    case EXTRA_FIELD:
        if (G.extra_field != (uch *)NULL)
            free(G.extra_field);
        if ((G.extra_field = (uch *)malloc(length)) == (uch *)NULL) {
            Info(slide, 0x401, ((char *)slide, ExtraFieldTooLong,
              length));
            /* cur_zipfile_bufstart already takes account of extra_bytes,
             * so don't correct for it twice: */
            seek_zipf(__G__ G.cur_zipfile_bufstart - G.extra_bytes +
                      (G.inptr-G.inbuf) + length);
        } else {
            if ((length2 = readbuf(__G__ (char *)G.extra_field, length)) == 0)
                return PK_EOF;
            if(length2 < length) {
              memset (__G__ (char *)G.extra_field+length2, 0 , length-length2);
              length = length2;
            }
            /* Looks like here is where extra fields are read */
            if (getZip64Data(__G__ G.extra_field, length) != PK_COOL)
            {
                Info(slide, 0x401, ((char *)slide,
                  ExtraFieldCorrupt, EF_PKSZ64));
                error = PK_WARN;
            }
            G.unipath_filename = NULL;
            if (G.UzO.U_flag < 2) {
              /* check if GPB11 (General Purpuse Bit 11) is set indicating
                 the standard path and comment are UTF-8 */
              if (G.pInfo->GPFIsUTF8) {
                /* if GPB11 set then filename_full is untruncated UTF-8 */
                G.unipath_filename = G.filename_full;
              } else {
                /* Get the Unicode fields if exist */
                getUnicodeData(__G__ G.extra_field, length);
                if (G.unipath_filename && strlen(G.unipath_filename) == 0) {
                  /* the standard filename field is UTF-8 */
                  free(G.unipath_filename);
                  G.unipath_filename = G.filename_full;
                }
              }
              if (G.unipath_filename) {
                if (G.native_is_utf8 && !G.unicode_escape_all) {
                  strncpy(G.filename, G.unipath_filename, FILNAMSIZ - 1);
                  /* make sure filename is short enough */
                  if (strlen(G.unipath_filename) >= FILNAMSIZ) {
                    G.filename[FILNAMSIZ - 1] = '\0';
                    Info(slide, 0x401, ((char *)slide,
                      UFilenameTooLongTrunc));
                    error = PK_WARN;
                  }
                } else {
                  char *fn;

                  /* convert UTF-8 to local character set */
                  fn = utf8_to_local_string(G.unipath_filename,
                                            G.unicode_escape_all);

                  /* 2022-07-22 SMS, et al.  CVE-2022-0530
                   * Detect conversion failure, emit message.
                   * Continue with unconverted name.
                   */
                  if (fn == NULL)
                  {
                    Info(slide, 0x401, ((char *)slide,
                     UFilenameCorrupt));
                    error = PK_ERR;
                  }
                  else
                  {
                    /* make sure filename is short enough */
                    if (strlen(fn) >= FILNAMSIZ) {
                      fn[FILNAMSIZ - 1] = '\0';
                      Info(slide, 0x401, ((char *)slide,
                        UFilenameTooLongTrunc));
                      error = PK_WARN;
                    }
                    /* replace filename with converted UTF-8 */
                    strcpy(G.filename, fn);
                    free(fn);
                  }
                }
                if (G.unipath_filename != G.filename_full)
                  free(G.unipath_filename);
                G.unipath_filename = NULL;
              }
            }
        }
        break;

    } /* end switch (option) */

    return error;

} /* end function do_string() */





/***********************/
/* Function makeword() */
/***********************/

ush makeword(b)
    const uch *b;
{
    /*
     * Convert Intel style 'short' integer to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (ush)((b[1] << 8) | b[0]);
}





/***********************/
/* Function makelong() */
/***********************/

ulg makelong(sig)
    const uch *sig;
{
    /*
     * Convert intel style 'long' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((ulg)sig[3]) << 24)
         + (((ulg)sig[2]) << 16)
         + (ulg)((((unsigned)sig[1]) << 8)
               + ((unsigned)sig[0]));
}





/************************/
/* Function makeint64() */
/************************/

zusz_t makeint64(sig)
    const uch *sig;
{
    /*
     * Convert intel style 'int64' variable to non-Intel non-16-bit
     * host format.  This routine also takes care of byte-ordering.
     */
    return (((zusz_t)sig[7]) << 56)
        + (((zusz_t)sig[6]) << 48)
        + (((zusz_t)sig[5]) << 40)
        + (((zusz_t)sig[4]) << 32)
        + (zusz_t)((((ulg)sig[3]) << 24)
                 + (((ulg)sig[2]) << 16)
                 + (((unsigned)sig[1]) << 8)
                 + (sig[0]));

}





/*********************/
/* Function fzofft() */
/*********************/

/* Format a zoff_t value in a cylindrical buffer set. */
char *fzofft(__G__ val, pre, post)
    __GDEF
    zoff_t val;
    const char *pre;
    const char *post;
{
    /* Storage cylinder. (now in globals.h) */
    /*static char fzofft_buf[FZOFFT_NUM][FZOFFT_LEN];*/
    /*static int fzofft_index = 0;*/

    /* Temporary format string storage. */
    char fmt[16];

    /* Assemble the format string. */
    fmt[0] = '%';
    fmt[1] = '\0';             /* Start after initial "%". */
    if (pre == FZOFFT_HEX_WID)  /* Special hex width. */
    {
        strcat(fmt, FZOFFT_HEX_WID_VALUE);
    }
    else if (pre == FZOFFT_HEX_DOT_WID) /* Special hex ".width". */
    {
        strcat(fmt, ".");
        strcat(fmt, FZOFFT_HEX_WID_VALUE);
    }
    else if (pre != NULL)       /* Caller's prefix (width). */
    {
        strcat(fmt, pre);
    }

    strcat(fmt, FZOFFT_FMT);   /* Long or long-long or whatever. */

    if (post == NULL)
        strcat(fmt, "d");      /* Default radix = decimal. */
    else
        strcat(fmt, post);     /* Caller's radix. */

    /* Advance the cylinder. */
    G.fzofft_index = (G.fzofft_index + 1) % FZOFFT_NUM;

    /* Write into the current chamber. */
    sprintf(G.fzofft_buf[G.fzofft_index], fmt, val);

    /* Return a pointer to this chamber. */
    return G.fzofft_buf[G.fzofft_index];
}




#if CRYPT

#  ifdef NEED_STR2ISO
/**********************/
/* Function str2iso() */
/**********************/

char *str2iso(dst, src)
    char *dst;                          /* destination buffer */
    const char *src;                    /* source string */
{
#    ifdef INTERN_TO_ISO
    INTERN_TO_ISO(src, dst);
#    else
    uch c;
    char *dstp = dst;

    do {
        c = (uch)foreign(*src++);
        *dstp++ = (char)ASCII2ISO(c);
    } while (c != '\0');
#    endif

    return dst;
}
#  endif /* NEED_STR2ISO */


#  ifdef NEED_STR2OEM
/**********************/
/* Function str2oem() */
/**********************/

char *str2oem(dst, src)
    char *dst;                          /* destination buffer */
    const char *src;                    /* source string */
{
#    ifdef INTERN_TO_OEM
    INTERN_TO_OEM(src, dst);
#    else
    uch c;
    char *dstp = dst;

    do {
        c = (uch)foreign(*src++);
        *dstp++ = (char)ASCII2OEM(c);
    } while (c != '\0');
#    endif

    return dst;
}
#  endif /* NEED_STR2OEM */

#endif /* CRYPT */





#ifdef NO_STRNICMP

/************************/
/* Function zstrnicmp() */
/************************/

int zstrnicmp(s1, s2, n)
    const char *s1, *s2;
    unsigned n;
{
    for (; n > 0;  --n, ++s1, ++s2) {

        if (tolower((uch)*s1) != tolower((uch)*s2))
            /* test includes early termination of one string */
            return ((uch)tolower(*s1) < (uch)tolower((uch)*s2))? -1 : 1;

        if (*s1 == '\0')   /* both strings terminate early */
            return 0;
    }
    return 0;
}

#endif /* NO_STRNICMP */





