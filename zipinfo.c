/*
  Copyright (c) 1990-2009 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  zipinfo.c                                              Greg Roelofs et al.

  This file contains all of the ZipInfo-specific listing routines for UnZip.

  Contains:  zi_opts()
             zi_end_central()
             zipinfo()
             zi_long()
             zi_short()
             zi_time()

  ---------------------------------------------------------------------------*/


#define UNZIP_INTERNAL
#include "unzip.h"


#ifndef NO_ZIPINFO  /* strings use up too much space in small-memory systems */

/* Define OS-specific attributes for use on ALL platforms--the S_xxxx
 * versions of these are defined differently (or not defined) by different
 * compilers and operating systems. */

#  define UNX_IFMT       0170000     /* Unix file type mask */
#  define UNX_IFREG      0100000     /* Unix regular file */
#  define UNX_IFSOCK     0140000     /* Unix socket (BSD, not SysV or Amiga) */
#  define UNX_IFLNK      0120000     /* Unix symbolic link (not SysV, Amiga) */
#  define UNX_IFBLK      0060000     /* Unix block special       (not Amiga) */
#  define UNX_IFDIR      0040000     /* Unix directory */
#  define UNX_IFCHR      0020000     /* Unix character special   (not Amiga) */
#  define UNX_IFIFO      0010000     /* Unix fifo    (BCC, not MSC or Amiga) */
#  define UNX_ISUID      04000       /* Unix set user id on execution */
#  define UNX_ISGID      02000       /* Unix set group id on execution */
#  define UNX_ISVTX      01000       /* Unix directory permissions control */
#  define UNX_ENFMT      UNX_ISGID   /* Unix record locking enforcement flag */
#  define UNX_IRWXU      00700       /* Unix read, write, execute: owner */
#  define UNX_IRUSR      00400       /* Unix read permission: owner */
#  define UNX_IWUSR      00200       /* Unix write permission: owner */
#  define UNX_IXUSR      00100       /* Unix execute permission: owner */
#  define UNX_IRWXG      00070       /* Unix read, write, execute: group */
#  define UNX_IRGRP      00040       /* Unix read permission: group */
#  define UNX_IWGRP      00020       /* Unix write permission: group */
#  define UNX_IXGRP      00010       /* Unix execute permission: group */
#  define UNX_IRWXO      00007       /* Unix read, write, execute: other */
#  define UNX_IROTH      00004       /* Unix read permission: other */
#  define UNX_IWOTH      00002       /* Unix write permission: other */
#  define UNX_IXOTH      00001       /* Unix execute permission: other */

#  define VMS_IRUSR      UNX_IRUSR   /* VMS read/owner */
#  define VMS_IWUSR      UNX_IWUSR   /* VMS write/owner */
#  define VMS_IXUSR      UNX_IXUSR   /* VMS execute/owner */
#  define VMS_IRGRP      UNX_IRGRP   /* VMS read/group */
#  define VMS_IWGRP      UNX_IWGRP   /* VMS write/group */
#  define VMS_IXGRP      UNX_IXGRP   /* VMS execute/group */
#  define VMS_IROTH      UNX_IROTH   /* VMS read/other */
#  define VMS_IWOTH      UNX_IWOTH   /* VMS write/other */
#  define VMS_IXOTH      UNX_IXOTH   /* VMS execute/other */

#  define AMI_IFMT       06000       /* Amiga file type mask */
#  define AMI_IFDIR      04000       /* Amiga directory */
#  define AMI_IFREG      02000       /* Amiga regular file */
#  define AMI_IHIDDEN    00200       /* to be supported in AmigaDOS 3.x */
#  define AMI_ISCRIPT    00100       /* executable script (text command file) */
#  define AMI_IPURE      00040       /* allow loading into resident memory */
#  define AMI_IARCHIVE   00020       /* not modified since bit was last set */
#  define AMI_IREAD      00010       /* can be opened for reading */
#  define AMI_IWRITE     00004       /* can be opened for writing */
#  define AMI_IEXECUTE   00002       /* executable image, a loadable runfile */
#  define AMI_IDELETE    00001       /* can be deleted */

#  define THS_IFMT    0xF000         /* Theos file type mask */
#  define THS_IFIFO   0x1000         /* pipe */
#  define THS_IFCHR   0x2000         /* char device */
#  define THS_IFSOCK  0x3000         /* socket */
#  define THS_IFDIR   0x4000         /* directory */
#  define THS_IFLIB   0x5000         /* library */
#  define THS_IFBLK   0x6000         /* block device */
#  define THS_IFREG   0x8000         /* regular file */
#  define THS_IFREL   0x9000         /* relative (direct) */
#  define THS_IFKEY   0xA000         /* keyed */
#  define THS_IFIND   0xB000         /* indexed */
#  define THS_IFRND   0xC000         /* ???? */
#  define THS_IFR16   0xD000         /* 16 bit real mode program */
#  define THS_IFP16   0xE000         /* 16 bit protected mode prog */
#  define THS_IFP32   0xF000         /* 32 bit protected mode prog */
#  define THS_IMODF   0x0800         /* modified */
#  define THS_INHID   0x0400         /* not hidden */
#  define THS_IEUSR   0x0200         /* erase permission: owner */
#  define THS_IRUSR   0x0100         /* read permission: owner */
#  define THS_IWUSR   0x0080         /* write permission: owner */
#  define THS_IXUSR   0x0040         /* execute permission: owner */
#  define THS_IROTH   0x0004         /* read permission: other */
#  define THS_IWOTH   0x0002         /* write permission: other */
#  define THS_IXOTH   0x0001         /* execute permission: other */


#  ifndef NSK_UNSTRUCTURED
#    define NSK_UNSTRUCTURED   0
#  endif
#  ifndef NSK_OBJECTFILECODE
#    define NSK_OBJECTFILECODE 100
#  endif
#  ifndef NSK_EDITFILECODE
#    define NSK_EDITFILECODE   101
#  endif

#  define LFLAG  3   /* short "ls -l" type listing */

static int   zi_long      (__GPRO__ zusz_t *pEndprev, int error_in_archive);
static int   zi_short     (__GPRO);
static void  zi_showMacTypeCreator
                          (__GPRO__ uch *ebfield);
static char *zi_time      (__GPRO__ const ulg *datetimez,
                           const time_t *modtimez, char *d_t_str);


/**********************************************/
/*  Strings used in zipinfo.c (ZipInfo half)  */
/**********************************************/

static const char nullStr[] = "";
static const char PlurSufx[] = "s";

static const char ZipInfHeader2[] =
  "Zip file size: %s bytes, number of entries: %s\n";
static const char EndCentDirRec[] = "\nEnd-of-central-directory record:\n";
static const char LineSeparators[] = "-------------------------------\n\n";
static const char ZipFSizeVerbose[] = "\
  Zip archive file size:               %s (%sh)\n";
static const char ActOffsetCentDir[] = "\
  Actual end-cent-dir record offset:   %s (%sh)\n\
  Expected end-cent-dir record offset: %s (%sh)\n\
  (based on the length of the central directory and its expected offset)\n\n";
static const char SinglePartArchive1[] = "\
  This zipfile constitutes the sole disk of a single-part archive; its\n\
  central directory contains %s %s.\n\
  The central directory is %s (%sh) bytes long,\n";
static const char SinglePartArchive2[] = "\
  and its (expected) offset in bytes from the beginning of the zipfile\n\
  is %s (%sh).\n\n";
static const char MultiPartArchive1[] = "\
  This zipfile constitutes disk %lu of a multi-part archive.  The central\n\
  directory starts on disk %lu at an offset within that archive part\n";
static const char MultiPartArchive2[] = "\
  of %s (%sh) bytes.  The entire\n\
  central directory is %s (%sh) bytes long.\n";
static const char MultiPartArchive3[] = "\
  %s of the archive entries %s contained within this zipfile volume,\n\
  out of a total of %s %s.\n\n";

static const char CentralDirEntry[] =
  "\nCentral directory entry #%lu:\n---------------------------\n\n";
static const char ZipfileStats[] =
  "%lu file%s, %s bytes uncompressed, %s bytes compressed:  %s%d.%d%%\n";

/* zi_long() strings */
static const char OS_FAT[] = "MS-DOS, OS/2 or NT FAT";
static const char OS_Amiga[] = "Amiga";
static const char OS_VMS[] = "VMS";
static const char OS_Unix[] = "Unix";
static const char OS_VMCMS[] = "VM/CMS";
static const char OS_AtariST[] = "Atari ST";
static const char OS_HPFS[] = "OS/2 or NT HPFS";
static const char OS_Macintosh[] = "Macintosh HFS";
static const char OS_ZSystem[] = "Z-System";
static const char OS_CPM[] = "CP/M";
static const char OS_TOPS20[] = "TOPS-20";
static const char OS_NTFS[] = "NTFS";
static const char OS_QDOS[] = "SMS/QDOS";
static const char OS_Acorn[] = "Acorn RISC OS";
static const char OS_MVS[] = "MVS";
static const char OS_VFAT[] = "Win32 VFAT";
static const char OS_AtheOS[] = "AtheOS";
static const char OS_BeOS[] = "BeOS";
static const char OS_Tandem[] = "Tandem NSK";
static const char OS_Theos[] = "Theos";
static const char OS_MacDarwin[] = "Mac OS/X (Darwin)";

static const char MthdNone[] = "none (stored)";
static const char MthdShrunk[] = "shrunk";
static const char MthdRedF1[] = "reduced (factor 1)";
static const char MthdRedF2[] = "reduced (factor 2)";
static const char MthdRedF3[] = "reduced (factor 3)";
static const char MthdRedF4[] = "reduced (factor 4)";
static const char MthdImplode[] = "imploded";
static const char MthdToken[] = "tokenized";
static const char MthdDeflate[] = "deflated";
static const char MthdDeflat64[] = "deflated (enhanced-64k)";
static const char MthdDCLImplode[] = "imploded (PK DCL)";
static const char MthdBZip2[] = "bzipped";
static const char MthdLZMA[] = "LZMA-ed";
static const char MthdTerse[] = "tersed (IBM)";
static const char MthdLZ77[] = "LZ77-compressed (IBM)";
static const char MthdWavPack[] = "WavPacked";
static const char MthdPPMd[] = "PPMd-ed";

static const char DeflNorm[] = "normal";
static const char DeflMax[] = "maximum";
static const char DeflFast[] = "fast";
static const char DeflSFast[] = "superfast";

static const char ExtraBytesPreceding[] =
  "  There are an extra %s bytes preceding this file.\n\n";

static const char UnknownNo[] = "unknown (%d)";

  static const char LocalHeaderOffset[] =
    "\n  offset of local header from start of archive:   %s\n\
                                                  (%sh) bytes\n";
static const char HostOS[] =
  "  file system or operating system of origin:      %s\n";
static const char EncodeSWVer[] =
  "  version of encoding software:                   %u.%u\n";
static const char MinOSCompReq[] =
  "  minimum file system compatibility required:     %s\n";
static const char MinSWVerReq[] =
  "  minimum software version required to extract:   %u.%u\n";
static const char CompressMethod[] =
  "  compression method:                             %s\n";
static const char SlideWindowSizeImplode[] =
  "  size of sliding dictionary (implosion):         %cK\n";
static const char ShannonFanoTrees[] =
  "  number of Shannon-Fano trees (implosion):       %c\n";
static const char CompressSubtype[] =
  "  compression sub-type (deflation):               %s\n";
static const char FileSecurity[] =
  "  file security status:                           %sencrypted\n";
static const char ExtendedLocalHdr[] =
  "  extended local header:                          %s\n";
static const char FileModDate[] =
  "  file last modified on (DOS date/time):          %s\n";
static const char UT_FileModDate[] =
  "  file last modified on (UT extra field modtime): %s %s\n";
static const char LocalTime[] = "local";
  static const char GMTime[] = "UTC";
static const char CRC32Value[] =
  "  32-bit CRC value (hex):                         %.8lx\n";
static const char CompressedFileSize[] =
  "  compressed size:                                %s bytes\n";
static const char UncompressedFileSize[] =
  "  uncompressed size:                              %s bytes\n";
static const char FilenameLength[] =
  "  length of filename:                             %u characters\n";
static const char ExtraFieldLength[] =
  "  length of extra field:                          %u bytes\n";
static const char FileCommentLength[] =
  "  length of file comment:                         %u characters\n";
static const char FileDiskNum[] =
  "  disk number on which file begins:               disk %lu\n";
static const char ApparentFileType[] =
  "  apparent file type:                             %s\n";
static const char VMSFileAttributes[] =
  "  VMS file attributes (%06o octal):             %s\n";
static const char AmigaFileAttributes[] =
  "  Amiga file attributes (%06o octal):           %s\n";
static const char UnixFileAttributes[] =
  "  Unix file attributes (%06o octal):            %s\n";
static const char NonMSDOSFileAttributes[] =
  "  non-MSDOS external file attributes:             %06lX hex\n";
static const char MSDOSFileAttributes[] =
  "  MS-DOS file attributes (%02X hex):                none\n";
static const char MSDOSFileAttributesRO[] =
  "  MS-DOS file attributes (%02X hex):                read-only\n";
static const char MSDOSFileAttributesAlpha[] =
  "  MS-DOS file attributes (%02X hex):                %s%s%s%s%s%s%s%s\n";
static const char TheosFileAttributes[] =
  "  Theos file attributes (%04X hex):               %s\n";

static const char TheosFTypLib[] = "Library     ";
static const char TheosFTypDir[] = "Directory   ";
static const char TheosFTypReg[] = "Sequential  ";
static const char TheosFTypRel[] = "Direct      ";
static const char TheosFTypKey[] = "Keyed       ";
static const char TheosFTypInd[] = "Indexed     ";
static const char TheosFTypR16[] = " 86 program ";
static const char TheosFTypP16[] = "286 program ";
static const char TheosFTypP32[] = "386 program ";
static const char TheosFTypUkn[] = "???         ";

static const char ExtraFieldTrunc[] = "\n\
  error: EF data block (type 0x%04x) size %u exceeds remaining extra field\n\
         space %u; block length has been truncated.\n";
static const char ExtraFields[] = "\n\
  The central-directory extra field contains:";
static const char ExtraFieldType[] = "\n\
  - A subfield with ID 0x%04x (%s) and %u data bytes";
static const char efPKSZ64[] = "PKWARE 64-bit sizes";
static const char efAV[] = "PKWARE AV";
static const char efOS2[] = "OS/2";
static const char efPKVMS[] = "PKWARE VMS";
static const char efPKWin32[] = "PKWARE Win32";
static const char efPKUnix[] = "PKWARE Unix";
static const char efIZVMS[] = "Info-ZIP VMS";
static const char efIZUnix[] = "old Info-ZIP Unix/OS2/NT";
static const char efIZUnix2[] = "Unix UID/GID (16-bit)";
static const char efIZUnix3[] = "Unix UID/GID (any size)";
static const char efTime[] = "universal time";
static const char efU8Path[] = "UTF8 path name";
static const char efU8Commnt[] = "UTF8 entry comment";
static const char efJLMac[] = "old Info-ZIP Macintosh";
static const char efMac3[] = "new Info-ZIP Macintosh";
static const char efZipIt[] = "ZipIt Macintosh";
static const char efSmartZip[] = "SmartZip Macintosh";
static const char efZipIt2[] = "ZipIt Macintosh (short)";
static const char efVMCMS[] = "VM/CMS";
static const char efMVS[] = "MVS";
static const char efACL[] = "OS/2 ACL";
static const char efNTSD[] = "Security Descriptor";
static const char efAtheOS[] = "AtheOS";
static const char efBeOS[] = "BeOS";
static const char efQDOS[] = "SMS/QDOS";
static const char efAOSVS[] = "AOS/VS";
static const char efSpark[] = "Acorn SparkFS";
static const char efMD5[] = "Fred Kantor MD5";
static const char efASiUnix[] = "ASi Unix";
static const char efTandem[] = "Tandem NSK";
static const char efTheos[] = "Theos";
static const char efUnknown[] = "unknown";

static const char OS2EAs[] = ".\n\
    The local extra field has %lu bytes of OS/2 extended attributes.\n\
    (May not match OS/2 \"dir\" amount due to storage method)";
static const char izVMSdata[] = ".  The extra\n\
    field is %s and has %u bytes of VMS %s information%s";
static const char izVMSstored[] = "stored";
static const char izVMSrleenc[] = "run-length encoded";
static const char izVMSdeflat[] = "deflated";
static const char izVMScunknw[] = "compressed(?)";
static const char *izVMScomp[4] =
  {izVMSstored, izVMSrleenc, izVMSdeflat, izVMScunknw};
static const char ACLdata[] = ".\n\
    The local extra field has %lu bytes of access control list information";
static const char NTSDData[] = ".\n\
    The local extra field has %lu bytes of NT security descriptor data";
static const char UTdata[] = ".\n\
    The local extra field has UTC/GMT %s time%s";
static const char UTmodification[] = "modification";
static const char UTaccess[] = "access";
static const char UTcreation[] = "creation";
static const char U8PthCmnComplete[] = ".\n\
    The UTF8 data of the extra field (V%u, ASCII name CRC `%.8lx') are:\n   ";
static const char U8PthCmnF24[] = ". The first\n\
    24 UTF8 bytes in the extra field (V%u, ASCII name CRC `%.8lx') are:\n   ";
static const char ZipItFname[] = ".\n\
    The Mac long filename is %s";
static const char Mac3data[] = ".\n\
    The local extra field has %lu bytes of %scompressed Macintosh\n\
    finder attributes";
 /* MacOSdata[] is used by EF_MAC3, EF_ZIPIT, EF_ZIPIT2 and EF_JLEE e. f. */
static const char MacOSdata[] = ".\n\
    The associated file has type code `%c%c%c%c' and creator code `%c%c%c%c'";
static const char MacOSdata1[] = ".\n\
    The associated file has type code `0x%lx' and creator code `0x%lx'";
static const char MacOSJLEEflags[] = ".\n    File is marked as %s";
static const char MacOS_RF[] = "Resource-fork";
static const char MacOS_DF[] = "Data-fork";
static const char MacOSMAC3flags[] = ".\n\
    File is marked as %s, File Dates are in %d Bit";
static const char AtheOSdata[] = ".\n\
    The local extra field has %lu bytes of %scompressed AtheOS file attributes";
static const char BeOSdata[] = ".\n\
    The local extra field has %lu bytes of %scompressed BeOS file attributes";
 /* The associated file has type code `%c%c%c%c' and creator code `%c%c%c%c'" */
static const char QDOSdata[] = ".\n\
    The QDOS extra field subtype is `%c%c%c%c'";
static const char AOSVSdata[] = ".\n\
    The AOS/VS extra field revision is %d.%d";
static const char TandemUnstr[] = "Unstructured";
static const char TandemRel[]   = "Relative";
static const char TandemEntry[] = "Entry Sequenced";
static const char TandemKey[]   = "Key Sequenced";
static const char TandemEdit[]  = "Edit";
static const char TandemObj[]  = "Object";
static const char *TandemFileformat[6] =
  {TandemUnstr, TandemRel, TandemEntry, TandemKey, TandemEdit, TandemObj};
static const char Tandemdata[] = ".\n\
    The file was originally a Tandem %s file, with file code %u";
static const char MD5data[] = ".\n\
    The 128-bit MD5 signature is %s";

static const char First20[] = ".  The first\n    20 are:  ";
static const char ColonIndent[] = ":\n   ";
static const char efFormat[] = " %02x";

static const char lExtraFieldType[] = "\n\
  There %s a local extra field with ID 0x%04x (%s) and\n\
  %u data bytes (%s).\n";
static const char efIZuid[] =
  "GMT modification/access times and Unix UID/GID";
static const char efIZnouid[] = "GMT modification/access times only";


static const char NoFileComment[] = "\n  There is no file comment.\n";
static const char FileCommBegin[] = "\n\
------------------------- file comment begins ----------------------------\n";
static const char FileCommEnd[] = "\
-------------------------- file comment ends -----------------------------\n";

/* zi_time() strings */
static const char BogusFmt[] = "%03d";
static const char shtYMDHMTime[] = "%02u-%s-%02u %02u:%02u";
static const char lngYMDHMSTime[] = "%u %s %u %02u:%02u:%02u";
static const char DecimalTime[] = "%04u%02u%02u.%02u%02u%02u";
static const char lngYMDHMSTimeError[] = "???? ??? ?? ??:??:??";





/************************/
/*  Function zi_opts()  */
/************************/

int zi_opts(__G__ pargc, pargv)
    int *pargc;
    char ***pargv;
    __GDEF
{
    char   **argv, *s;
    int    argc, c, error=FALSE, negative=0;
    int    hflag_slmv=TRUE, hflag_2=FALSE;  /* diff options => diff defaults */
    int    tflag_slm=TRUE, tflag_2v=FALSE;
    int    explicit_h=FALSE, explicit_t=FALSE;

    extern char OEM_CP[MAX_CP_NAME];
    extern char ISO_CP[MAX_CP_NAME];

    G.extract_flag = FALSE;   /* zipinfo does not extract to disk */
    argc = *pargc;
    argv = *pargv;

    while (--argc > 0 && (*++argv)[0] == '-') {
        s = argv[0] + 1;
        while ((c = *s++) != 0) {    /* "!= 0":  prevent Turbo C warning */
            switch (c) {
                case '-':
                    ++negative;
                    break;
                case '1':      /* shortest listing:  JUST filenames */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 1;
                    break;
                case '2':      /* just filenames, plus headers if specified */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 2;
                    break;
                case ('C'):    /* -C:  match filenames case-insensitively */
                    if (negative)
                        uO.C_flag = FALSE, negative = 0;
                    else
                        uO.C_flag = TRUE;
                    break;
                case 'h':      /* header line */
                    if (negative)
                        hflag_2 = hflag_slmv = FALSE, negative = 0;
                    else {
                        hflag_2 = hflag_slmv = explicit_h = TRUE;
                        if (uO.lflag == -1)
                            uO.lflag = 0;
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
                case 'l':      /* longer form of "ls -l" type listing */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 5;
                    break;
                case 'm':      /* medium form of "ls -l" type listing */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 4;
                    break;
#  ifdef MORE
                case 'M':      /* send output through built-in "more" */
                    if (negative)
                        G.M_flag = FALSE, negative = 0;
                    else
                        G.M_flag = TRUE;
                    break;
#  endif
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
                case 's':      /* default:  shorter "ls -l" type listing */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 3;
                    break;
                case 't':      /* totals line */
                    if (negative)
                        tflag_2v = tflag_slm = FALSE, negative = 0;
                    else {
                        tflag_2v = tflag_slm = explicit_t = TRUE;
                        if (uO.lflag == -1)
                            uO.lflag = 0;
                    }
                    break;
                case ('T'):    /* use (sortable) decimal time format */
                    if (negative)
                        uO.T_flag = FALSE, negative = 0;
                    else
                        uO.T_flag = TRUE;
                    break;
                case ('U'):    /* escape UTF-8, or disable UTF-8 support */
                    if (negative) {
                        uO.U_flag = MAX(uO.U_flag-negative,0);
                        negative = 0;
                    } else
                        uO.U_flag++;
                    break;
                case 'v':      /* turbo-verbose listing */
                    if (negative)
                        uO.lflag = -2, negative = 0;
                    else
                        uO.lflag = 10;
                    break;
                case ('W'):    /* Wildcard interpretation (stop at '/'?) */
                    if (negative)
                        uO.W_flag = FALSE, negative = 0;
                    else
                        uO.W_flag = TRUE;
                    break;
                case 'z':      /* print zipfile comment */
                    if (negative)
                        uO.zflag = negative = 0;
                    else
                        uO.zflag = 1;
                    break;
                case 'Z':      /* ZipInfo mode:  ignore */
                    break;
                default:
                    error = TRUE;
                    break;
            }
        }
    }
    if ((argc-- == 0) || error) {
        *pargc = argc;
        *pargv = argv;
        return USAGE(error);
    }

#  ifdef MORE
    if (G.M_flag && !isatty(1))  /* stdout redirected: "more" func useless */
        G.M_flag = 0;
#  endif

    /* if no listing options given (or all negated), or if only -h/-t given
     * with individual files specified, use default listing format */
    if ((uO.lflag < 0) || ((argc > 0) && (uO.lflag == 0)))
        uO.lflag = LFLAG;

    /* set header and totals flags to default or specified values */
    switch (uO.lflag) {
        case 0:   /* 0:  can only occur if either -t or -h explicitly given; */
        case 2:   /*  therefore set both flags equal to normally false value */
            uO.hflag = hflag_2;
            uO.tflag = tflag_2v;
            break;
        case 1:   /* only filenames, *always* */
            uO.hflag = FALSE;
            uO.tflag = FALSE;
            uO.zflag = FALSE;
            break;
        case 3:
        case 4:
        case 5:
            uO.hflag = ((argc > 0) && !explicit_h)? FALSE : hflag_slmv;
            uO.tflag = ((argc > 0) && !explicit_t)? FALSE : tflag_slm;
            break;
        case 10:
            uO.hflag = hflag_slmv;
            uO.tflag = tflag_2v;
            break;
    }

    *pargc = argc;
    *pargv = argv;
    return 0;

} /* end function zi_opts() */





/*******************************/
/*  Function zi_end_central()  */
/*******************************/

void zi_end_central(__G)
    __GDEF
{
/*---------------------------------------------------------------------------
    Print out various interesting things about the zipfile.
  ---------------------------------------------------------------------------*/

    if (uO.lflag > 9) {
        /* verbose format */
        Info(slide, 0, ((char *)slide, EndCentDirRec));
        Info(slide, 0, ((char *)slide, LineSeparators));

        Info(slide, 0, ((char *)slide, ZipFSizeVerbose,
          FmZofft(G.ziplen, "11", NULL),
          FmZofft(G.ziplen, FZOFFT_HEX_DOT_WID, "X")));
        Info(slide, 0, ((char *)slide, ActOffsetCentDir,
          FmZofft(G.real_ecrec_offset, "11", "u"),
          FmZofft(G.real_ecrec_offset, FZOFFT_HEX_DOT_WID, "X"),
          FmZofft(G.expect_ecrec_offset, "11", "u"),
          FmZofft(G.expect_ecrec_offset, FZOFFT_HEX_DOT_WID, "X")));

        if (G.ecrec.number_this_disk == 0) {
            Info(slide, 0, ((char *)slide, SinglePartArchive1,
              FmZofft(G.ecrec.total_entries_central_dir, NULL, "u"),
              (G.ecrec.total_entries_central_dir == 1)? "entry" : "entries",
              FmZofft(G.ecrec.size_central_directory, NULL, "u"),
              FmZofft(G.ecrec.size_central_directory,
                      FZOFFT_HEX_DOT_WID, "X")));
            Info(slide, 0, ((char *)slide, SinglePartArchive2,
              FmZofft(G.ecrec.offset_start_central_directory, NULL, "u"),
              FmZofft(G.ecrec.offset_start_central_directory,
                      FZOFFT_HEX_DOT_WID, "X")));
        } else {
            Info(slide, 0, ((char *)slide, MultiPartArchive1,
              (ulg)(G.ecrec.number_this_disk + 1),
              (ulg)(G.ecrec.num_disk_start_cdir + 1)));
            Info(slide, 0, ((char *)slide, MultiPartArchive2,
              FmZofft(G.ecrec.offset_start_central_directory, NULL, "u"),
              FmZofft(G.ecrec.offset_start_central_directory,
                      FZOFFT_HEX_DOT_WID, "X"),
              FmZofft(G.ecrec.size_central_directory, NULL, "u"),
              FmZofft(G.ecrec.size_central_directory,
                      FZOFFT_HEX_DOT_WID, "X")));
            Info(slide, 0, ((char *)slide, MultiPartArchive3,
              FmZofft(G.ecrec.num_entries_centrl_dir_ths_disk, NULL, "u"),
              (G.ecrec.num_entries_centrl_dir_ths_disk == 1)? "is" : "are",
              FmZofft(G.ecrec.total_entries_central_dir, NULL, "u"),
              (G.ecrec.total_entries_central_dir == 1) ? "entry" : "entries"));
        }
    }
    else if (uO.hflag) {
        /* print zip file size and number of contained entries: */
        Info(slide, 0, ((char *)slide, ZipInfHeader2,
          FmZofft(G.ziplen, NULL, NULL),
          FmZofft(G.ecrec.total_entries_central_dir, NULL, "u")));
    }

} /* end function zi_end_central() */





/************************/
/*  Function zipinfo()  */
/************************/

int zipinfo(__G)   /* return PK-type error code */
    __GDEF
{
    int do_this_file=FALSE, error, error_in_archive=PK_COOL;
    int *fn_matched=NULL, *xn_matched=NULL;
    ulg j, members=0L;
    zusz_t tot_csize=0L, tot_ucsize=0L;
    zusz_t endprev;   /* buffers end of previous entry for zi_long()'s check
                       *  of extra bytes */


/*---------------------------------------------------------------------------
    Malloc space for check on unmatched filespecs (no big deal if one or both
    are NULL).
  ---------------------------------------------------------------------------*/

    if (G.filespecs > 0  &&
        (fn_matched=(int *)malloc(G.filespecs*sizeof(int))) != NULL)
        for (j = 0;  j < G.filespecs;  ++j)
            fn_matched[j] = FALSE;

    if (G.xfilespecs > 0  &&
        (xn_matched=(int *)malloc(G.xfilespecs*sizeof(int))) != NULL)
        for (j = 0;  j < G.xfilespecs;  ++j)
            xn_matched[j] = FALSE;

/*---------------------------------------------------------------------------
    Set file pointer to start of central directory, then loop through cen-
    tral directory entries.  Check that directory-entry signature bytes are
    actually there (just a precaution), then process the entry.  We know
    the entire central directory is on this disk:  we wouldn't have any of
    this information unless the end-of-central-directory record was on this
    disk, and we wouldn't have gotten to this routine unless this is also
    the disk on which the central directory starts.  In practice, this had
    better be the *only* disk in the archive, but maybe someday we'll add
    multi-disk support.
  ---------------------------------------------------------------------------*/

    uO.L_flag = FALSE;      /* zipinfo mode: never convert name to lowercase */
    G.pInfo = G.info;       /* (re-)initialize, (just to make sure) */
    G.pInfo->textmode = 0;  /* so one can read on screen (is this ever used?) */

    /* reset endprev for new zipfile; account for multi-part archives (?) */
    endprev = (G.crec.relative_offset_local_header == 4L)? 4L : 0L;


    for (j = 1L;; j++) {
        if (readbuf(__G__ G.sig, 4) == 0) {
            error_in_archive = PK_EOF;
            break;
        }
        if (memcmp(G.sig, central_hdr_sig, 4)) {  /* is it a CentDir entry? */
            /* no new central directory entry
             * -> is the number of processed entries compatible with the
             *    number of entries as stored in the end_central record?
             */
            if (((j - 1) &
                 (ulg)(G.ecrec.have_ecr64 ? MASK_ZUCN64 : MASK_ZUCN16))
                == (ulg)G.ecrec.total_entries_central_dir)
            {
                /* "j modulus 4T/64k" matches the reported 64/16-bit-unsigned
                 * number of directory entries -> probably, the regular
                 * end of the central directory has been reached
                 */
                break;
            } else {
                Info(slide, 0x401,
                     ((char *)slide, CentSigMsg, j));
                Info(slide, 0x401,
                     ((char *)slide,"%s", ReportMsg));
                error_in_archive = PK_BADERR;   /* sig not found */
                break;
            }
        }
        /* process_cdir_file_hdr() sets pInfo->hostnum, pInfo->lcflag, ...: */
        if ((error = process_cdir_file_hdr(__G)) != PK_COOL) {
            error_in_archive = error;   /* only PK_EOF defined */
            break;
        }

        if ((error = do_string(__G__ G.crec.filename_length, DS_FN)) !=
             PK_COOL)
        {
          if (error > error_in_archive)
              error_in_archive = error;
          if (error > PK_WARN)        /* fatal */
              break;
        }

        if (!G.process_all_files) {   /* check if specified on command line */
            unsigned i;

            if (G.filespecs == 0)
                do_this_file = TRUE;
            else {  /* check if this entry matches an `include' argument */
                do_this_file = FALSE;
                for (i = 0; i < G.filespecs; i++)
                    if (match(G.filename, G.pfnames[i], uO.C_flag WISEP)) {
                        do_this_file = TRUE;
                        if (fn_matched)
                            fn_matched[i] = TRUE;
                        break;       /* found match, so stop looping */
                    }
            }
            if (do_this_file) {  /* check if this is an excluded file */
                for (i = 0; i < G.xfilespecs; i++)
                    if (match(G.filename, G.pxnames[i], uO.C_flag WISEP)) {
                        do_this_file = FALSE;  /* ^-- ignore case in match */
                        if (xn_matched)
                            xn_matched[i] = TRUE;
                        break;
                    }
            }
        }

    /*-----------------------------------------------------------------------
        If current file was specified on command line, or if no names were
        specified, do the listing for this file.  Otherwise, get rid of the
        file comment and go back for the next file.
      -----------------------------------------------------------------------*/

        if (G.process_all_files || do_this_file) {

            /* Read the extra field, if any.  The extra field info is required
             * for resolving the Zip64 sizes/offsets and may be used in more
             * analysis of the entry below.
             */
            if ((error = do_string(__G__ G.crec.extra_field_length,
                                   EXTRA_FIELD)) != 0)
            {
                if (G.extra_field != NULL) {
                    free(G.extra_field);
                    G.extra_field = NULL;
                }
                error_in_archive = error;
                /* The premature return in case of a "fatal" error (PK_EOF) is
                 * delayed until we analyze the extra field contents.
                 * This allows us to display all the other info that has been
                 * successfully read in.
                 */
            }

            switch (uO.lflag) {
                case 1:
                case 2:
                    fnprint(__G);
                    SKIP_(G.crec.file_comment_length)
                    break;

                case 3:
                case 4:
                case 5:
                    if ((error = zi_short(__G)) != PK_COOL) {
                        error_in_archive = error;   /* might be warning */
                    }
                    break;

                case 10:
                    Info(slide, 0, ((char *)slide,
                      CentralDirEntry, j));
                    if ((error = zi_long(__G__ &endprev,
                                         error_in_archive)) != PK_COOL) {
                        error_in_archive = error;   /* might be warning */
                    }
                    break;

                default:
                    SKIP_(G.crec.file_comment_length)
                    break;

            } /* end switch (lflag) */
            if (error > PK_WARN)        /* fatal */
                break;

            tot_csize += G.crec.csize;
            tot_ucsize += G.crec.ucsize;
            if (G.crec.general_purpose_bit_flag & 1)
                tot_csize -= 12;   /* don't count encryption header */
            ++members;
        } else {        /* not listing this file */
            SKIP_(G.crec.extra_field_length)
            SKIP_(G.crec.file_comment_length)
            if (endprev != 0) endprev = 0;

        } /* end if (list member?) */

    } /* end for-loop (j: member files) */

/*---------------------------------------------------------------------------
    Check that we actually found requested files; if so, print totals.
  ---------------------------------------------------------------------------*/

    if ((error_in_archive <= PK_WARN) && uO.tflag) {
        char *sgn = "";
        int cfactor = ratio(tot_ucsize, tot_csize);

        if (cfactor < 0) {
            sgn = "-";
            cfactor = -cfactor;
        }
        Info(slide, 0, ((char *)slide, ZipfileStats,
          members, (members==1L)? nullStr:PlurSufx,
          FmZofft(tot_ucsize, NULL, "u"),
          FmZofft(tot_csize, NULL, "u"),
          sgn, cfactor/10, cfactor%10));
    }

/*---------------------------------------------------------------------------
    Check for unmatched filespecs on command line and print warning if any
    found.
  ---------------------------------------------------------------------------*/

    if (fn_matched) {
        if (error_in_archive <= PK_WARN)
            for (j = 0;  j < G.filespecs;  ++j)
                if (!fn_matched[j])
                    Info(slide, 0x401, ((char *)slide,
                      FilenameNotMatched, G.pfnames[j]));
        free((void *)fn_matched);
    }
    if (xn_matched) {
        if (error_in_archive <= PK_WARN)
            for (j = 0;  j < G.xfilespecs;  ++j)
                if (!xn_matched[j])
                    Info(slide, 0x401, ((char *)slide,
                      ExclFilenameNotMatched, G.pxnames[j]));
        free((void *)xn_matched);
    }


    /* Skip the following checks in case of a premature listing break. */
    if (error_in_archive <= PK_WARN) {

/*---------------------------------------------------------------------------
    Double check that we're back at the end-of-central-directory record.
  ---------------------------------------------------------------------------*/

        if ( (memcmp(G.sig,
                     (G.ecrec.have_ecr64 ?
                      end_central64_sig : end_central_sig),
                     4) != 0)
            && (!G.ecrec.is_zip64_archive)
            && (memcmp(G.sig, end_central_sig, 4) != 0)
           ) {          /* just to make sure again */
            Info(slide, 0x401,
                 ((char *)slide,"%s", EndSigMsg));
            error_in_archive = PK_WARN;   /* didn't find sig */
        }

        /* Set specific return code when no files have been found. */
        if (members == 0L && error_in_archive <= PK_WARN)
            error_in_archive = PK_FIND;

        if (uO.lflag >= 10)
            (*G.message)((void *)&G, (uch *)"\n", 1L, 0);
    }

    return error_in_archive;

} /* end function zipinfo() */





/************************/
/*  Function zi_long()  */
/************************/

static int zi_long(__G__ pEndprev, error_in_archive)
    /* return PK-type error code */
    __GDEF
    zusz_t *pEndprev;                /* for zi_long() check of extra bytes */
    int error_in_archive;            /* may signal premature return */
{
    iztimes z_utime;
    int  error;
    unsigned  hostnum, hostver, extnum, extver, methid, methnum, xattr;
    char workspace[12], attribs[22];
    const char *varmsg_str;
    char unkn[16];
    static const char *os[NUM_HOSTS] = {
        OS_FAT, OS_Amiga, OS_VMS, OS_Unix, OS_VMCMS, OS_AtariST, OS_HPFS,
        OS_Macintosh, OS_ZSystem, OS_CPM, OS_TOPS20, OS_NTFS, OS_QDOS,
        OS_Acorn, OS_VFAT, OS_MVS, OS_BeOS, OS_Tandem, OS_Theos, OS_MacDarwin,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        OS_AtheOS
    };
    static const char *method[NUM_METHODS] = {
        MthdNone, MthdShrunk, MthdRedF1, MthdRedF2, MthdRedF3, MthdRedF4,
        MthdImplode, MthdToken, MthdDeflate, MthdDeflat64, MthdDCLImplode,
        MthdBZip2, MthdLZMA, MthdTerse, MthdLZ77, MthdWavPack, MthdPPMd
    };
    static const char *dtypelng[4] = {
        DeflNorm, DeflMax, DeflFast, DeflSFast
    };


/*---------------------------------------------------------------------------
    Check whether there's any extra space inside the zipfile.  If *pEndprev is
    zero, it's probably a signal that OS/2 extra fields are involved (with
    unknown compressed size).  We won't worry about prepended junk here...
  ---------------------------------------------------------------------------*/

    if (G.crec.relative_offset_local_header != *pEndprev && *pEndprev > 0L) {
        /*  GRR DEBUG
        Info(slide, 0, ((char *)slide,
          "  [crec.relative_offset_local_header = %lu, endprev = %lu]\n",
          G.crec.relative_offset_local_header, *pEndprev));
         */
        Info(slide, 0, ((char *)slide, ExtraBytesPreceding,
          FmZofft((G.crec.relative_offset_local_header - (*pEndprev)),
          NULL, NULL)));
    }

    /* calculate endprev for next time around (problem:  extra fields may
     * differ in length between local and central-directory records) */
    *pEndprev = G.crec.relative_offset_local_header + (4L + LREC_SIZE) +
      G.crec.filename_length + G.crec.extra_field_length + G.crec.csize;

/*---------------------------------------------------------------------------
    Print out various interesting things about the compressed file.
  ---------------------------------------------------------------------------*/

    hostnum = (unsigned)(G.pInfo->hostnum);
    hostver = (unsigned)(G.pInfo->hostver);
    extnum = (unsigned)MIN(G.crec.version_needed_to_extract[1], NUM_HOSTS);
    extver = (unsigned)G.crec.version_needed_to_extract[0];
    methid = (unsigned)G.crec.compression_method;
    methnum = find_compr_idx(G.crec.compression_method);

    (*G.message)((void *)&G, (uch *)"  ", 2L, 0);  fnprint(__G);

    Info(slide, 0, ((char *)slide, LocalHeaderOffset,
      FmZofft(G.crec.relative_offset_local_header, NULL, "u"),
      FmZofft(G.crec.relative_offset_local_header, FZOFFT_HEX_DOT_WID, "X")));

    if (hostnum >= NUM_HOSTS) {
        sprintf(unkn, UnknownNo,
                (int)G.crec.version_made_by[1]);
        varmsg_str = unkn;
    } else {
        varmsg_str = os[hostnum];
    }
    Info(slide, 0, ((char *)slide, HostOS, varmsg_str));
    Info(slide, 0, ((char *)slide, EncodeSWVer, hostver/10,
      hostver%10));

    if ((extnum >= NUM_HOSTS) || (os[extnum] == NULL)) {
        sprintf(unkn, UnknownNo,
                (int)G.crec.version_needed_to_extract[1]);
        varmsg_str = unkn;
    } else {
        varmsg_str = os[extnum];
    }
    Info(slide, 0, ((char *)slide, MinOSCompReq, varmsg_str));
    Info(slide, 0, ((char *)slide, MinSWVerReq, extver/10,
      extver%10));

    if (methnum >= NUM_METHODS) {
        sprintf(unkn, UnknownNo, G.crec.compression_method);
        varmsg_str = unkn;
    } else {
        varmsg_str = method[methnum];
    }
    Info(slide, 0, ((char *)slide, CompressMethod, varmsg_str));
    if (methid == IMPLODED) {
        Info(slide, 0, ((char *)slide, SlideWindowSizeImplode,
          (G.crec.general_purpose_bit_flag & 2)? '8' : '4'));
        Info(slide, 0, ((char *)slide, ShannonFanoTrees,
          (G.crec.general_purpose_bit_flag & 4)? '3' : '2'));
    } else if (methid == DEFLATED || methid == ENHDEFLATED) {
        ush  dnum=(ush)((G.crec.general_purpose_bit_flag>>1) & 3);

        Info(slide, 0, ((char *)slide, CompressSubtype,
          dtypelng[dnum]));
    }

    Info(slide, 0, ((char *)slide, FileSecurity,
      (G.crec.general_purpose_bit_flag & 1) ? nullStr : "not "));
    Info(slide, 0, ((char *)slide, ExtendedLocalHdr,
      (G.crec.general_purpose_bit_flag & 8) ? "yes" : "no"));
    /* print upper 3 bits for amusement? */

    /* For printing of date & time, a "char d_t_buf[21]" is required.
     * To save stack space, we reuse the "char attribs[22]" buffer which
     * is not used yet.
     */
#  define d_t_buf attribs

    zi_time(__G__ &G.crec.last_mod_dos_datetime, NULL, d_t_buf);
    Info(slide, 0, ((char *)slide, FileModDate, d_t_buf));
    if (G.extra_field &&
#  ifdef IZ_CHECK_TZ
        G.tz_is_valid &&
#  endif
        (ef_scan_for_izux(G.extra_field, G.crec.extra_field_length, 1,
                          G.crec.last_mod_dos_datetime, &z_utime, NULL)
         & EB_UT_FL_MTIME))
    {
        TIMET_TO_NATIVE(z_utime.mtime)   /* NOP unless MSC 7.0 or Macintosh */
        d_t_buf[0] = (char)0;               /* signal "show local time" */
        zi_time(__G__ &G.crec.last_mod_dos_datetime, &(z_utime.mtime), d_t_buf);
        Info(slide, 0, ((char *)slide, UT_FileModDate,
          d_t_buf, LocalTime));
        d_t_buf[0] = (char)1;           /* signal "show UTC (GMT) time" */
        zi_time(__G__ &G.crec.last_mod_dos_datetime, &(z_utime.mtime), d_t_buf);
        Info(slide, 0, ((char *)slide, UT_FileModDate,
          d_t_buf, GMTime));
    }

    Info(slide, 0, ((char *)slide, CRC32Value, G.crec.crc32));
    Info(slide, 0, ((char *)slide, CompressedFileSize,
      FmZofft(G.crec.csize, NULL, "u")));
    Info(slide, 0, ((char *)slide, UncompressedFileSize,
      FmZofft(G.crec.ucsize, NULL, "u")));
    Info(slide, 0, ((char *)slide, FilenameLength,
      G.crec.filename_length));
    Info(slide, 0, ((char *)slide, ExtraFieldLength,
      G.crec.extra_field_length));
    Info(slide, 0, ((char *)slide, FileCommentLength,
      G.crec.file_comment_length));
    Info(slide, 0, ((char *)slide, FileDiskNum,
      (ulg)(G.crec.disk_number_start + 1)));
    Info(slide, 0, ((char *)slide, ApparentFileType,
      (G.crec.internal_file_attributes & 1)? "text"
         : (G.crec.internal_file_attributes & 2)? "ebcdic"
              : "binary"));             /* changed to accept EBCDIC */
    xattr = (unsigned)((G.crec.external_file_attributes >> 16) & 0xFFFF);
    if (hostnum == VMS_) {
        char   *p=attribs, *q=attribs+1;
        int    i, j, k;

        for (k = 0;  k < 12;  ++k)
            workspace[k] = 0;
        if (xattr & VMS_IRUSR)
            workspace[0] = 'R';
        if (xattr & VMS_IWUSR) {
            workspace[1] = 'W';
            workspace[3] = 'D';
        }
        if (xattr & VMS_IXUSR)
            workspace[2] = 'E';
        if (xattr & VMS_IRGRP)
            workspace[4] = 'R';
        if (xattr & VMS_IWGRP) {
            workspace[5] = 'W';
            workspace[7] = 'D';
        }
        if (xattr & VMS_IXGRP)
            workspace[6] = 'E';
        if (xattr & VMS_IROTH)
            workspace[8] = 'R';
        if (xattr & VMS_IWOTH) {
            workspace[9] = 'W';
            workspace[11] = 'D';
        }
        if (xattr & VMS_IXOTH)
            workspace[10] = 'E';

        *p++ = '(';
        for (k = j = 0;  j < 3;  ++j) {    /* loop over groups of permissions */
            for (i = 0;  i < 4;  ++i, ++k)  /* loop over perms within a group */
                if (workspace[k])
                    *p++ = workspace[k];
            *p++ = ',';                       /* group separator */
            if (j == 0)
                while ((*p++ = *q++) != ',')
                    ;                         /* system, owner perms are same */
        }
        *p-- = '\0';
        *p = ')';   /* overwrite last comma */
        Info(slide, 0, ((char *)slide, VMSFileAttributes, xattr,
          attribs));

    } else if (hostnum == AMIGA_) {
        switch (xattr & AMI_IFMT) {
            case AMI_IFDIR:  attribs[0] = 'd';  break;
            case AMI_IFREG:  attribs[0] = '-';  break;
            default:         attribs[0] = '?';  break;
        }
        attribs[1] = (xattr & AMI_IHIDDEN)?   'h' : '-';
        attribs[2] = (xattr & AMI_ISCRIPT)?   's' : '-';
        attribs[3] = (xattr & AMI_IPURE)?     'p' : '-';
        attribs[4] = (xattr & AMI_IARCHIVE)?  'a' : '-';
        attribs[5] = (xattr & AMI_IREAD)?     'r' : '-';
        attribs[6] = (xattr & AMI_IWRITE)?    'w' : '-';
        attribs[7] = (xattr & AMI_IEXECUTE)?  'e' : '-';
        attribs[8] = (xattr & AMI_IDELETE)?   'd' : '-';
        attribs[9] = 0;   /* better dlm the string */
        Info(slide, 0, ((char *)slide, AmigaFileAttributes,
          xattr, attribs));

    } else if (hostnum == THEOS_) {
        const char *fpFtyp;

        switch (xattr & THS_IFMT) {
            case THS_IFLIB:  fpFtyp = TheosFTypLib;  break;
            case THS_IFDIR:  fpFtyp = TheosFTypDir;  break;
            case THS_IFREG:  fpFtyp = TheosFTypReg;  break;
            case THS_IFREL:  fpFtyp = TheosFTypRel;  break;
            case THS_IFKEY:  fpFtyp = TheosFTypKey;  break;
            case THS_IFIND:  fpFtyp = TheosFTypInd;  break;
            case THS_IFR16:  fpFtyp = TheosFTypR16;  break;
            case THS_IFP16:  fpFtyp = TheosFTypP16;  break;
            case THS_IFP32:  fpFtyp = TheosFTypP32;  break;
            default:         fpFtyp = TheosFTypUkn;  break;
        }
        strcpy(attribs, fpFtyp);
        attribs[12] = (xattr & THS_INHID) ? '.' : 'H';
        attribs[13] = (xattr & THS_IMODF) ? '.' : 'M';
        attribs[14] = (xattr & THS_IWOTH) ? '.' : 'W';
        attribs[15] = (xattr & THS_IROTH) ? '.' : 'R';
        attribs[16] = (xattr & THS_IEUSR) ? '.' : 'E';
        attribs[17] = (xattr & THS_IXUSR) ? '.' : 'X';
        attribs[18] = (xattr & THS_IWUSR) ? '.' : 'W';
        attribs[19] = (xattr & THS_IRUSR) ? '.' : 'R';
        attribs[20] = 0;
        Info(slide, 0, ((char *)slide, TheosFileAttributes,
          xattr, attribs));
    } else if ((hostnum != FS_FAT_) && (hostnum != FS_HPFS_) &&
               (hostnum != FS_NTFS_) && (hostnum != FS_VFAT_) &&
               (hostnum != ACORN_) &&
               (hostnum != VM_CMS_) && (hostnum != MVS_))
    {                                 /* assume Unix-like */
        switch ((unsigned)(xattr & UNX_IFMT)) {
            case (unsigned)UNX_IFDIR:   attribs[0] = 'd';  break;
            case (unsigned)UNX_IFREG:   attribs[0] = '-';  break;
            case (unsigned)UNX_IFLNK:   attribs[0] = 'l';  break;
            case (unsigned)UNX_IFBLK:   attribs[0] = 'b';  break;
            case (unsigned)UNX_IFCHR:   attribs[0] = 'c';  break;
            case (unsigned)UNX_IFIFO:   attribs[0] = 'p';  break;
            case (unsigned)UNX_IFSOCK:  attribs[0] = 's';  break;
            default:          attribs[0] = '?';  break;
        }
        attribs[1] = (xattr & UNX_IRUSR)? 'r' : '-';
        attribs[4] = (xattr & UNX_IRGRP)? 'r' : '-';
        attribs[7] = (xattr & UNX_IROTH)? 'r' : '-';

        attribs[2] = (xattr & UNX_IWUSR)? 'w' : '-';
        attribs[5] = (xattr & UNX_IWGRP)? 'w' : '-';
        attribs[8] = (xattr & UNX_IWOTH)? 'w' : '-';

        if (xattr & UNX_IXUSR)
            attribs[3] = (xattr & UNX_ISUID)? 's' : 'x';
        else
            attribs[3] = (xattr & UNX_ISUID)? 'S' : '-';   /* S = undefined */
        if (xattr & UNX_IXGRP)
            attribs[6] = (xattr & UNX_ISGID)? 's' : 'x';   /* == UNX_ENFMT */
        else
            attribs[6] = (xattr & UNX_ISGID)? 'l' : '-';
        if (xattr & UNX_IXOTH)
            attribs[9] = (xattr & UNX_ISVTX)? 't' : 'x';   /* "sticky bit" */
        else
            attribs[9] = (xattr & UNX_ISVTX)? 'T' : '-';   /* T = undefined */
        attribs[10] = 0;

        Info(slide, 0, ((char *)slide, UnixFileAttributes, xattr,
          attribs));

    } else {
        Info(slide, 0, ((char *)slide, NonMSDOSFileAttributes,
            G.crec.external_file_attributes >> 8));

    } /* endif (hostnum: external attributes format) */

    if ((xattr=(unsigned)(G.crec.external_file_attributes & 0xFF)) == 0)
        Info(slide, 0, ((char *)slide, MSDOSFileAttributes,
          xattr));
    else if (xattr == 1)
        Info(slide, 0, ((char *)slide, MSDOSFileAttributesRO,
          xattr));
    else
        Info(slide, 0, ((char *)slide, MSDOSFileAttributesAlpha,
          xattr, (xattr&1)? "rdo " : nullStr,
          (xattr&2)? "hid " : nullStr,
          (xattr&4)? "sys " : nullStr,
          (xattr&8)? "lab " : nullStr,
          (xattr&16)? "dir " : nullStr,
          (xattr&32)? "arc " : nullStr,
          (xattr&64)? "lnk " : nullStr,
          (xattr&128)? "exe" : nullStr));

/*---------------------------------------------------------------------------
    Analyze the extra field, if any, and print the file comment, if any (the
    filename has already been printed, above).  That finishes up this file
    entry...
  ---------------------------------------------------------------------------*/

    if (G.crec.extra_field_length > 0) {
        uch *ef_ptr = G.extra_field;
        ush ef_len = G.crec.extra_field_length;
        ush eb_id, eb_datalen;
        const char *ef_fieldname;

        if (error_in_archive > PK_WARN)   /* fatal:  can't continue */
            /* delayed "fatal error" return from extra field reading */
            return error_in_archive;
        if (G.extra_field == (uch *)NULL)
            return PK_ERR;   /* not consistent with crec length */

        Info(slide, 0, ((char *)slide, ExtraFields));

        while (ef_len >= EB_HEADSIZE) {
            eb_id = makeword(&ef_ptr[EB_ID]);
            eb_datalen = makeword(&ef_ptr[EB_LEN]);
            ef_ptr += EB_HEADSIZE;
            ef_len -= EB_HEADSIZE;

            if (eb_datalen > (ush)ef_len) {
                Info(slide, 0x421, ((char *)slide,
                  ExtraFieldTrunc, eb_id, eb_datalen, ef_len));
                eb_datalen = ef_len;
            }

            switch (eb_id) {
                case EF_PKSZ64:
                    ef_fieldname = efPKSZ64;
                    if ((G.crec.relative_offset_local_header
                         & (~(zusz_t)0xFFFFFFFFL)) != 0) {
                        /* Subtract the size of the 64bit local offset from
                           the local e.f. size, local Z64 e.f. block has no
                           offset; when only local offset present, the entire
                           local PKSZ64 block is missing. */
                        *pEndprev -= (eb_datalen == 8 ? 12 : 8);
                    }
                    break;
                case EF_AV:
                    ef_fieldname = efAV;
                    break;
                case EF_OS2:
                    ef_fieldname = efOS2;
                    break;
                case EF_ACL:
                    ef_fieldname = efACL;
                    break;
                case EF_NTSD:
                    ef_fieldname = efNTSD;
                    break;
                case EF_PKVMS:
                    ef_fieldname = efPKVMS;
                    break;
                case EF_IZVMS:
                    ef_fieldname = efIZVMS;
                    break;
                case EF_PKW32:
                    ef_fieldname = efPKWin32;
                    break;
                case EF_PKUNIX:
                    ef_fieldname = efPKUnix;
                    break;
                case EF_IZUNIX:
                    ef_fieldname = efIZUnix;
                    if (hostnum == UNIX_ && *pEndprev > 0L)
                        *pEndprev += 4L;  /* also have UID/GID in local copy */
                    break;
                case EF_IZUNIX2:
                    ef_fieldname = efIZUnix2;
                    if (*pEndprev > 0L)
                        *pEndprev += 4L;  /* 4 byte UID/GID in local copy */
                    break;
                case EF_IZUNIX3:
                    ef_fieldname = efIZUnix3;
#  if 0
                    if (*pEndprev > 0L)
                        *pEndprev += 4L;  /* 4 byte UID/GID in local copy */
#  endif
                    break;
                case EF_TIME:
                    ef_fieldname = efTime;
                    break;
                case EF_UNIPATH:
                    ef_fieldname = efU8Path;
                    break;
                case EF_UNICOMNT:
                    ef_fieldname = efU8Commnt;
                    break;
                case EF_MAC3:
                    ef_fieldname = efMac3;
                    break;
                case EF_JLMAC:
                    ef_fieldname = efJLMac;
                    break;
                case EF_ZIPIT:
                    ef_fieldname = efZipIt;
                    break;
                case EF_ZIPIT2:
                    ef_fieldname = efZipIt2;
                    break;
                case EF_VMCMS:
                    ef_fieldname = efVMCMS;
                    break;
                case EF_MVS:
                    ef_fieldname = efMVS;
                    break;
                case EF_ATHEOS:
                    ef_fieldname = efAtheOS;
                    break;
                case EF_BEOS:
                    ef_fieldname = efBeOS;
                    break;
                case EF_QDOS:
                    ef_fieldname = efQDOS;
                    break;
                case EF_AOSVS:
                    ef_fieldname = efAOSVS;
                    break;
                case EF_SPARK:   /* from RISC OS */
                    ef_fieldname = efSpark;
                    break;
                case EF_MD5:
                    ef_fieldname = efMD5;
                    break;
                case EF_ASIUNIX:
                    ef_fieldname = efASiUnix;
                    break;
                case EF_TANDEM:
                    ef_fieldname = efTandem;
                    break;
                case EF_SMARTZIP:
                    ef_fieldname = efSmartZip;
                    break;
                case EF_THEOS:
                    ef_fieldname = efTheos;
                    break;
                default:
                    ef_fieldname = efUnknown;
                    break;
            }
            Info(slide, 0, ((char *)slide, ExtraFieldType,
                 eb_id, ef_fieldname, eb_datalen));

            /* additional, field-specific information: */
            switch (eb_id) {
                case EF_OS2:
                case EF_ACL:
                    if (eb_datalen >= EB_OS2_HLEN) {
                        if (eb_id == EF_OS2)
                            ef_fieldname = OS2EAs;
                        else
                            ef_fieldname = ACLdata;
                        Info(slide, 0, ((char *)slide,
                          ef_fieldname, makelong(ef_ptr)));
                        *pEndprev = 0L;   /* no clue about csize of local */
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_NTSD:
                    if (eb_datalen >= EB_NTSD_C_LEN) {
                        Info(slide, 0, ((char *)slide, NTSDData,
                          makelong(ef_ptr)));
                        *pEndprev = 0L;   /* no clue about csize of local */
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_IZVMS:
                    if (eb_datalen >= 8) {
                        char *p, q[8];
                        unsigned compr = makeword(ef_ptr+EB_IZVMS_FLGS)
                                        & EB_IZVMS_BCMASK;

                        *q = '\0';
                        if (compr > 3)
                            compr = 3;
                        switch (makelong(ef_ptr)) {
                            case 0x42414656: /* "VFAB" */
                                p = "FAB"; break;
                            case 0x4C4C4156: /* "VALL" */
                                p = "XABALL"; break;
                            case 0x43484656: /* "VFHC" */
                                p = "XABFHC"; break;
                            case 0x54414456: /* "VDAT" */
                                p = "XABDAT"; break;
                            case 0x54445256: /* "VRDT" */
                                p = "XABRDT"; break;
                            case 0x4F525056: /* "VPRO" */
                                p = "XABPRO"; break;
                            case 0x59454B56: /* "VKEY" */
                                p = "XABKEY"; break;
                            case 0x56534D56: /* "VMSV" */
                                p = "version";
                                if (eb_datalen >= 16) {
                                    /* put termitation first, for A_TO_N() */
                                    q[7] = '\0';
                                    q[0] = ' ';
                                    q[1] = '(';
                                    strncpy(q+2,
                                            (char *)ef_ptr+EB_IZVMS_HLEN, 4);
                                    A_TO_N(q+2);
                                    q[6] = ')';
                                }
                                break;
                            default:
                                p = "unknown";
                        }
                        Info(slide, 0, ((char *)slide,
                          izVMSdata,
                          izVMScomp[compr],
                          makeword(ef_ptr+EB_IZVMS_UCSIZ), p, q));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_TIME:
                    if (eb_datalen > 0) {
                        char types[80];
                        int num = 0, len;

                        *types = '\0';
                        if (*ef_ptr & 1) {
                            strcpy(types, UTmodification);
                            ++num;
                        }
                        if (*ef_ptr & 2) {
                            len = strlen(types);
                            if (num)
                                types[len++] = '/';
                            strcpy(types+len, UTaccess);
                            ++num;
                            if (*pEndprev > 0L)
                                *pEndprev += 4L;
                        }
                        if (*ef_ptr & 4) {
                            len = strlen(types);
                            if (num)
                                types[len++] = '/';
                            strcpy(types+len, UTcreation);
                            ++num;
                            if (*pEndprev > 0L)
                                *pEndprev += 4L;
                        }
                        if (num > 0)
                            Info(slide, 0, ((char *)slide,
                              UTdata, types,
                              num == 1? nullStr : PlurSufx));
                    }
                    break;
                case EF_UNIPATH:
                case EF_UNICOMNT:
                    if (eb_datalen >= 5) {
                        unsigned i, n;
                        ulg name_crc = makelong(ef_ptr+1);

                        if (eb_datalen <= 29) {
                            Info(slide, 0, ((char *)slide,
                                 U8PthCmnComplete,
                                 (unsigned)ef_ptr[0], name_crc));
                            n = eb_datalen;
                        } else {
                            Info(slide, 0, ((char *)slide,
                                 U8PthCmnF24,
                                 (unsigned)ef_ptr[0], name_crc));
                            n = 29;
                        }
                        for (i = 5;  i < n;  ++i)
                            Info(slide, 0, ((char *)slide,
                                 efFormat, ef_ptr[i]));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_MAC3:
                    if (eb_datalen >= EB_MAC3_HLEN) {
                        ulg eb_uc = makelong(ef_ptr);
                        unsigned mac3_flgs = makeword(ef_ptr+EB_FLGS_OFFS);
                        unsigned eb_is_uc = mac3_flgs & EB_M3_FL_UNCMPR;

                        Info(slide, 0, ((char *)slide, Mac3data,
                          eb_uc, eb_is_uc ? "un" : nullStr));
                        if (eb_is_uc) {
                            if (*pEndprev > 0L)
                                *pEndprev += makelong(ef_ptr);
                        } else {
                            *pEndprev = 0L; /* no clue about csize of local */
                        }

                        Info(slide, 0, ((char *)slide,
                          MacOSMAC3flags,
                          mac3_flgs & EB_M3_FL_DATFRK ?
                                             MacOS_DF : MacOS_RF,
                          (mac3_flgs & EB_M3_FL_TIME64 ? 64 : 32)));
                        zi_showMacTypeCreator(__G__ &ef_ptr[6]);
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_ZIPIT2:
                    if (eb_datalen >= 5 &&
                        makelong(ef_ptr) == 0x5449505A /* "ZPIT" */) {

                        if (eb_datalen >= 12) {
                            zi_showMacTypeCreator(__G__ &ef_ptr[4]);
                        }
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_ZIPIT:
                    if (eb_datalen >= 5 &&
                        makelong(ef_ptr) == 0x5449505A /* "ZPIT" */) {
                        unsigned fnlen = ef_ptr[4];

                        if ((unsigned)eb_datalen >= fnlen + (5 + 8)) {
                            uch nullchar = ef_ptr[fnlen+5];

                            ef_ptr[fnlen+5] = '\0'; /* terminate filename */
                            A_TO_N(ef_ptr+5);
                            Info(slide, 0, ((char *)slide,
                              ZipItFname, (char *)ef_ptr+5));
                            ef_ptr[fnlen+5] = nullchar;
                            zi_showMacTypeCreator(__G__ &ef_ptr[fnlen+5]);
                        }
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_JLMAC:
                    if (eb_datalen >= 40 &&
                        makelong(ef_ptr) == 0x45454C4A /* "JLEE" */)
                    {
                        zi_showMacTypeCreator(__G__ &ef_ptr[4]);

                        Info(slide, 0, ((char *)slide,
                          MacOSJLEEflags,
                          ef_ptr[31] & 1 ?
                                             MacOS_DF : MacOS_RF));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_SMARTZIP:
                    if ((eb_datalen == EB_SMARTZIP_HLEN) &&
                        makelong(ef_ptr) == 0x70695A64 /* "dZip" */) {
                        char filenameBuf[32];
                        zi_showMacTypeCreator(__G__ &ef_ptr[4]);
                        memcpy(filenameBuf, &ef_ptr[33], 31);
                        filenameBuf[ef_ptr[32]] = '\0';
                        A_TO_N(filenameBuf);
                        Info(slide, 0, ((char *)slide,
                             ZipItFname, filenameBuf));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_ATHEOS:
                case EF_BEOS:
                    if (eb_datalen >= EB_BEOS_HLEN) {
                        ulg eb_uc = makelong(ef_ptr);
                        unsigned eb_is_uc =
                          *(ef_ptr+EB_FLGS_OFFS) & EB_BE_FL_UNCMPR;

                        if (eb_id == EF_ATHEOS)
                            ef_fieldname = AtheOSdata;
                        else
                            ef_fieldname = BeOSdata;
                        Info(slide, 0, ((char *)slide,
                          ef_fieldname,
                          eb_uc, eb_is_uc ? "un" : nullStr));
                        if (eb_is_uc) {
                            if (*pEndprev > 0L)
                                *pEndprev += makelong(ef_ptr);
                        } else {
                            *pEndprev = 0L; /* no clue about csize of local */
                        }
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_QDOS:
                    if (eb_datalen >= 4) {
                        Info(slide, 0, ((char *)slide, QDOSdata,
                          ef_ptr[0], ef_ptr[1], ef_ptr[2], ef_ptr[3]));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_AOSVS:
                    if (eb_datalen >= 5) {
                        Info(slide, 0, ((char *)slide, AOSVSdata,
                          ((int)(uch)ef_ptr[4])/10, ((int)(uch)ef_ptr[4])%10));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_TANDEM:
                    if (eb_datalen == 20) {
                        unsigned type, code;

                        type = (ef_ptr[18] & 0x60) >> 5;
                        code = makeword(ef_ptr);
                        /* Arrg..., Tandem e.f. uses BigEndian byte-order */
                        code = ((code << 8) & 0xff00) | ((code >> 8) & 0x00ff);
                        if (type == NSK_UNSTRUCTURED) {
                            if (code == NSK_EDITFILECODE)
                                type = 4;
                            else if (code == NSK_OBJECTFILECODE)
                                type = 5;
                        }
                        Info(slide, 0, ((char *)slide,
                          Tandemdata,
                          TandemFileformat[type],
                          code));
                    } else {
                        goto ef_default_display;
                    }
                    break;
                case EF_MD5:
                    if (eb_datalen >= 19) {
                        char md5[33];
                        int i;

                        for (i = 0;  i < 16;  ++i)
                            sprintf(&md5[i<<1], "%02x", ef_ptr[15-i]);
                        md5[32] = '\0';
                        Info(slide, 0, ((char *)slide, MD5data,
                          md5));
                        break;
                    }   /* else, fall through */
                default:
ef_default_display:
                    if (eb_datalen > 0) {
                        unsigned i, n;

                        if (eb_datalen <= 24) {
                            Info(slide, 0, ((char *)slide,
                                 ColonIndent));
                            n = eb_datalen;
                        } else {
                            Info(slide, 0, ((char *)slide,
                                 First20));
                            n = 20;
                        }
                        for (i = 0;  i < n;  ++i)
                            Info(slide, 0, ((char *)slide,
                                 efFormat, ef_ptr[i]));
                    }
                    break;
            }
            (*G.message)((void *)&G, (uch *)".", 1L, 0);

            ef_ptr += eb_datalen;
            ef_len -= eb_datalen;
        }
        (*G.message)((void *)&G, (uch *)"\n", 1L, 0);
    }

    /* high bit == Unix/OS2/NT GMT times (mtime, atime); next bit == UID/GID */
    if ((xattr = (unsigned)((G.crec.external_file_attributes & 0xC000) >> 12))
        & 8)
    {
        if (hostnum == UNIX_ || hostnum == FS_HPFS_ || hostnum == FS_NTFS_)
        {
            Info(slide, 0, ((char *)slide, lExtraFieldType,
              "is", EF_IZUNIX, efIZUnix,
              (unsigned)(xattr&12), (xattr&4)? efIZuid : efIZnouid));
            if (*pEndprev > 0L)
                *pEndprev += (ulg)(xattr&12);
        }
        else if (hostnum == FS_FAT_ && !(xattr&4))
            Info(slide, 0, ((char *)slide, lExtraFieldType,
              "may be", EF_IZUNIX, efIZUnix, 8,
              efIZnouid));
    }

    if (!G.crec.file_comment_length)
        Info(slide, 0, ((char *)slide, NoFileComment));
    else {
        Info(slide, 0, ((char *)slide, FileCommBegin));
        if ((error = do_string(__G__ G.crec.file_comment_length, DISPL_8)) !=
            PK_COOL)
        {
            error_in_archive = error;   /* might be warning */
            if (error > PK_WARN)   /* fatal */
                return error;
        }
        Info(slide, 0, ((char *)slide, FileCommEnd));
    }

    return error_in_archive;

} /* end function zi_long() */





/*************************/
/*  Function zi_short()  */
/*************************/

static int zi_short(__G)   /* return PK-type error code */
    __GDEF
{
    iztimes     z_utime;
    time_t      *z_modtim;
    int         k, error, error_in_archive=PK_COOL;
    unsigned    hostnum, hostver, methid, methnum, xattr;
    char        *p, workspace[12], attribs[17];
    char        methbuf[5];
    static const char dtype[5]="NXFS";  /* normal, maximum, fast, superfast */
    static const char os[NUM_HOSTS+1][4] = {
        "fat", "ami", "vms", "unx", "cms", "atr", "hpf", "mac", "zzz",
        "cpm", "t20", "ntf", "qds", "aco", "vft", "mvs", "be ", "nsk",
        "ths", "osx", "???", "???", "???", "???", "???", "???", "???",
        "???", "???", "???", "ath", "???"
    };
    static const char method[NUM_METHODS+1][5] = {
        "stor", "shrk", "re:1", "re:2", "re:3", "re:4", "i#:#", "tokn",
        "def#", "d64#", "dcli", "bzp2", "lzma", "ters", "lz77", "wavp",
        "ppmd", "u###"
    };


/*---------------------------------------------------------------------------
    Print out various interesting things about the compressed file.
  ---------------------------------------------------------------------------*/

    methid = (unsigned)(G.crec.compression_method);
    methnum = find_compr_idx(G.crec.compression_method);
    hostnum = (unsigned)(G.pInfo->hostnum);
    hostver = (unsigned)(G.pInfo->hostver);
/*
    extnum = (unsigned)MIN(G.crec.version_needed_to_extract[1], NUM_HOSTS);
    extver = (unsigned)G.crec.version_needed_to_extract[0];
 */

    strcpy(methbuf, method[methnum]);
    if (methid == IMPLODED) {
        methbuf[1] = (char)((G.crec.general_purpose_bit_flag & 2)? '8' : '4');
        methbuf[3] = (char)((G.crec.general_purpose_bit_flag & 4)? '3' : '2');
    } else if (methid == DEFLATED || methid == ENHDEFLATED) {
        ush  dnum=(ush)((G.crec.general_purpose_bit_flag>>1) & 3);
        methbuf[3] = dtype[dnum];
    } else if (methnum >= NUM_METHODS) {   /* unknown */
        /* 2016-12-05 SMS.
         * https://launchpad.net/bugs/1643750
         * Unexpectedly large compression methods overflow
         * &methbuf[].  Use the old, three-digit decimal format
         * for values which fit.  Otherwise, sacrifice the "u",
         * and use four-digit hexadecimal.
         */
        if (G.crec.compression_method <= 999) {
            sprintf( &methbuf[ 1], "%03u", G.crec.compression_method);
        } else {
            sprintf( &methbuf[ 0], "%04X", G.crec.compression_method);
        }
    }

    for (k = 0;  k < 15;  ++k)
        attribs[k] = ' ';
    attribs[15] = 0;

    xattr = (unsigned)((G.crec.external_file_attributes >> 16) & 0xFFFF);
    switch (hostnum) {
        case VMS_:
            {   int    i, j;

                for (k = 0;  k < 12;  ++k)
                    workspace[k] = 0;
                if (xattr & VMS_IRUSR)
                    workspace[0] = 'R';
                if (xattr & VMS_IWUSR) {
                    workspace[1] = 'W';
                    workspace[3] = 'D';
                }
                if (xattr & VMS_IXUSR)
                    workspace[2] = 'E';
                if (xattr & VMS_IRGRP)
                    workspace[4] = 'R';
                if (xattr & VMS_IWGRP) {
                    workspace[5] = 'W';
                    workspace[7] = 'D';
                }
                if (xattr & VMS_IXGRP)
                  workspace[6] = 'E';
                if (xattr & VMS_IROTH)
                    workspace[8] = 'R';
                if (xattr & VMS_IWOTH) {
                    workspace[9] = 'W';
                    workspace[11] = 'D';
                }
                if (xattr & VMS_IXOTH)
                    workspace[10] = 'E';

                p = attribs;
                for (k = j = 0;  j < 3;  ++j) {     /* groups of permissions */
                    for (i = 0;  i < 4;  ++i, ++k)  /* perms within a group */
                        if (workspace[k])
                            *p++ = workspace[k];
                    *p++ = ',';                     /* group separator */
                }
                *--p = ' ';   /* overwrite last comma */
                if ((p - attribs) < 12)
                    sprintf(&attribs[12], "%u.%u", hostver/10, hostver%10);
            }
            break;

        case AMIGA_:
            switch (xattr & AMI_IFMT) {
                case AMI_IFDIR:  attribs[0] = 'd';  break;
                case AMI_IFREG:  attribs[0] = '-';  break;
                default:         attribs[0] = '?';  break;
            }
            attribs[1] = (xattr & AMI_IHIDDEN)?   'h' : '-';
            attribs[2] = (xattr & AMI_ISCRIPT)?   's' : '-';
            attribs[3] = (xattr & AMI_IPURE)?     'p' : '-';
            attribs[4] = (xattr & AMI_IARCHIVE)?  'a' : '-';
            attribs[5] = (xattr & AMI_IREAD)?     'r' : '-';
            attribs[6] = (xattr & AMI_IWRITE)?    'w' : '-';
            attribs[7] = (xattr & AMI_IEXECUTE)?  'e' : '-';
            attribs[8] = (xattr & AMI_IDELETE)?   'd' : '-';
            sprintf(&attribs[12], "%u.%u", hostver/10, hostver%10);
            break;

        case THEOS_:
            switch (xattr & THS_IFMT) {
                case THS_IFLIB: *attribs = 'L'; break;
                case THS_IFDIR: *attribs = 'D'; break;
                case THS_IFCHR: *attribs = 'C'; break;
                case THS_IFREG: *attribs = 'S'; break;
                case THS_IFREL: *attribs = 'R'; break;
                case THS_IFKEY: *attribs = 'K'; break;
                case THS_IFIND: *attribs = 'I'; break;
                case THS_IFR16: *attribs = 'P'; break;
                case THS_IFP16: *attribs = '2'; break;
                case THS_IFP32: *attribs = '3'; break;
                default:        *attribs = '?'; break;
            }
            attribs[1] = (xattr & THS_INHID) ? '.' : 'H';
            attribs[2] = (xattr & THS_IMODF) ? '.' : 'M';
            attribs[3] = (xattr & THS_IWOTH) ? '.' : 'W';
            attribs[4] = (xattr & THS_IROTH) ? '.' : 'R';
            attribs[5] = (xattr & THS_IEUSR) ? '.' : 'E';
            attribs[6] = (xattr & THS_IXUSR) ? '.' : 'X';
            attribs[7] = (xattr & THS_IWUSR) ? '.' : 'W';
            attribs[8] = (xattr & THS_IRUSR) ? '.' : 'R';
            sprintf(&attribs[12], "%u.%u", hostver/10, hostver%10);
            break;

        case FS_VFAT_:

        case FS_FAT_:
        case FS_HPFS_:
        case FS_NTFS_:
        case VM_CMS_:
        case MVS_:
        case ACORN_:
            if (hostnum != FS_FAT_ ||
                (unsigned)(xattr & 0700) !=
                 ((unsigned)0400 |
                  ((unsigned)!(G.crec.external_file_attributes & 1) << 7) |
                  ((unsigned)(G.crec.external_file_attributes & 0x10) << 2))
               )
            {
                xattr = (unsigned)(G.crec.external_file_attributes & 0xFF);
                sprintf(attribs, ".r.-...     %u.%u", hostver/10, hostver%10);
                attribs[2] = (xattr & 0x01)? '-' : 'w';
                attribs[5] = (xattr & 0x02)? 'h' : '-';
                attribs[6] = (xattr & 0x04)? 's' : '-';
                attribs[4] = (xattr & 0x20)? 'a' : '-';
                if (xattr & 0x10) {
                    attribs[0] = 'd';
                    attribs[3] = 'x';
                } else
                    attribs[0] = '-';
                if (IS_VOLID(xattr))
                    attribs[0] = 'V';
                else if ((p = MBSRCHR(G.filename, '.')) != (char *)NULL) {
                    ++p;
                    if (STRNICMP(p, "com", 3) == 0 ||
                        STRNICMP(p, "exe", 3) == 0 ||
                        STRNICMP(p, "btm", 3) == 0 ||
                        STRNICMP(p, "cmd", 3) == 0 ||
                        STRNICMP(p, "bat", 3) == 0)
                        attribs[3] = 'x';
                }
                break;
            } /* else, fall through */

        default:   /* assume Unix-like */
            switch ((unsigned)(xattr & UNX_IFMT)) {
                case (unsigned)UNX_IFDIR:   attribs[0] = 'd';  break;
                case (unsigned)UNX_IFREG:   attribs[0] = '-';  break;
                case (unsigned)UNX_IFLNK:   attribs[0] = 'l';  break;
                case (unsigned)UNX_IFBLK:   attribs[0] = 'b';  break;
                case (unsigned)UNX_IFCHR:   attribs[0] = 'c';  break;
                case (unsigned)UNX_IFIFO:   attribs[0] = 'p';  break;
                case (unsigned)UNX_IFSOCK:  attribs[0] = 's';  break;
                default:          attribs[0] = '?';  break;
            }
            attribs[1] = (xattr & UNX_IRUSR)? 'r' : '-';
            attribs[4] = (xattr & UNX_IRGRP)? 'r' : '-';
            attribs[7] = (xattr & UNX_IROTH)? 'r' : '-';
            attribs[2] = (xattr & UNX_IWUSR)? 'w' : '-';
            attribs[5] = (xattr & UNX_IWGRP)? 'w' : '-';
            attribs[8] = (xattr & UNX_IWOTH)? 'w' : '-';

            if (xattr & UNX_IXUSR)
                attribs[3] = (xattr & UNX_ISUID)? 's' : 'x';
            else
                attribs[3] = (xattr & UNX_ISUID)? 'S' : '-';  /* S==undefined */
            if (xattr & UNX_IXGRP)
                attribs[6] = (xattr & UNX_ISGID)? 's' : 'x';  /* == UNX_ENFMT */
            else
                /* attribs[6] = (xattr & UNX_ISGID)? 'l' : '-';  real 4.3BSD */
                attribs[6] = (xattr & UNX_ISGID)? 'S' : '-';  /* SunOS 4.1.x */
            if (xattr & UNX_IXOTH)
                attribs[9] = (xattr & UNX_ISVTX)? 't' : 'x';  /* "sticky bit" */
            else
                attribs[9] = (xattr & UNX_ISVTX)? 'T' : '-';  /* T==undefined */

            sprintf(&attribs[11], "%2u.%u", hostver/10, hostver%10);
            break;

    } /* end switch (hostnum: external attributes format) */

    Info(slide, 0, ((char *)slide, "%s %s %s ", attribs,
      os[hostnum],
      FmZofft(G.crec.ucsize, "8", "u")));
    Info(slide, 0, ((char *)slide, "%c",
      (G.crec.general_purpose_bit_flag & 1)?
      ((G.crec.internal_file_attributes & 1)? 'T' : 'B') :  /* encrypted */
      ((G.crec.internal_file_attributes & 1)? 't' : 'b'))); /* plaintext */
    k = (G.crec.extra_field_length ||
         /* a local-only "UX" (old Unix/OS2/NT GMT times "IZUNIX") e.f.? */
         ((G.crec.external_file_attributes & 0x8000) &&
          (hostnum == UNIX_ || hostnum == FS_HPFS_ || hostnum == FS_NTFS_)));
    Info(slide, 0, ((char *)slide, "%c", k?
      ((G.crec.general_purpose_bit_flag & 8)? 'X' : 'x') :  /* extra field */
      ((G.crec.general_purpose_bit_flag & 8)? 'l' : '-'))); /* no extra field */
      /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ extended local header or not */

    if (uO.lflag == 4) {
        zusz_t csiz = G.crec.csize;

        if (G.crec.general_purpose_bit_flag & 1)
            csiz -= 12;    /* if encrypted, don't count encryption header */
        Info(slide, 0, ((char *)slide, "%3d%%",
          (ratio(G.crec.ucsize,csiz)+5)/10));
    } else if (uO.lflag == 5)
        Info(slide, 0, ((char *)slide, " %s",
          FmZofft(G.crec.csize, "8", "u")));

    /* For printing of date & time, a "char d_t_buf[16]" is required.
     * To save stack space, we reuse the "char attribs[16]" buffer whose
     * content is no longer needed.
     */
#  define d_t_buf attribs
    z_modtim = G.extra_field &&
#  ifdef IZ_CHECK_TZ
               G.tz_is_valid &&
#  endif
               (ef_scan_for_izux(G.extra_field, G.crec.extra_field_length, 1,
                                 G.crec.last_mod_dos_datetime, &z_utime, NULL)
                & EB_UT_FL_MTIME)
              ? &z_utime.mtime : NULL;
    TIMET_TO_NATIVE(z_utime.mtime)     /* NOP unless MSC 7.0 or Macintosh */
    d_t_buf[0] = (char)0;              /* signal "show local time" */
    Info(slide, 0, ((char *)slide, " %s %s ", methbuf,
      zi_time(__G__ &G.crec.last_mod_dos_datetime, z_modtim, d_t_buf)));
    fnprint(__G);

/*---------------------------------------------------------------------------
    Skip the file comment, if any (the filename has already been printed,
    above).  That finishes up this file entry...
  ---------------------------------------------------------------------------*/

    SKIP_(G.crec.file_comment_length)

    return error_in_archive;

} /* end function zi_short() */





/**************************************/
/*  Function zi_showMacTypeCreator()  */
/**************************************/

static void zi_showMacTypeCreator(__G__ ebfield)
    __GDEF
    uch *ebfield;
{
    /* not every Type / Creator character is printable */
    if (isprint(native(ebfield[0])) && isprint(native(ebfield[1])) &&
        isprint(native(ebfield[2])) && isprint(native(ebfield[3])) &&
        isprint(native(ebfield[4])) && isprint(native(ebfield[5])) &&
        isprint(native(ebfield[6])) && isprint(native(ebfield[7]))) {
       Info(slide, 0, ((char *)slide, MacOSdata,
            native(ebfield[0]), native(ebfield[1]),
            native(ebfield[2]), native(ebfield[3]),
            native(ebfield[4]), native(ebfield[5]),
            native(ebfield[6]), native(ebfield[7])));
    } else {
       Info(slide, 0, ((char *)slide, MacOSdata1,
            (((ulg)ebfield[0]) << 24) +
            (((ulg)ebfield[1]) << 16) +
            (((ulg)ebfield[2]) << 8)  +
            ((ulg)ebfield[3]),
            (((ulg)ebfield[4]) << 24) +
            (((ulg)ebfield[5]) << 16) +
            (((ulg)ebfield[6]) << 8)  +
            ((ulg)ebfield[7])));
    }
} /* end function zi_showMacTypeCreator() */





/************************/
/*  Function zi_time()  */
/************************/

static char *zi_time(__G__ datetimez, modtimez, d_t_str)
    __GDEF
    const ulg *datetimez;
    const time_t *modtimez;
    char *d_t_str;
{
    unsigned yr, mo, dy, hh, mm, ss;
    char monthbuf[4];
    const char *monthstr;
    static const char month[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    struct tm *t;



/*---------------------------------------------------------------------------
    Convert the file-modification date and time info to a string of the form
    "1991 Feb 23 17:15:00", "23-Feb-91 17:15" or "19910223.171500", depending
    on values of lflag and T_flag.  If using Unix-time extra fields, convert
    to local time or not, depending on value of first character in d_t_str[].
  ---------------------------------------------------------------------------*/

    if (modtimez != NULL) {
        /* check for our secret message from above... */
        t = (d_t_str[0] == (char)1)? gmtime(modtimez) : localtime(modtimez);
        if (uO.lflag > 9 && t == (struct tm *)NULL)
            /* time conversion error in verbose listing format,
             * return string with '?' instead of data
             */
            return (strcpy(d_t_str, lngYMDHMSTimeError));
    } else
        t = (struct tm *)NULL;
    if (t != (struct tm *)NULL) {
        mo = (unsigned)(t->tm_mon + 1);
        dy = (unsigned)(t->tm_mday);
        yr = (unsigned)(t->tm_year);

        hh = (unsigned)(t->tm_hour);
        mm = (unsigned)(t->tm_min);
        ss = (unsigned)(t->tm_sec);
    } else {
        yr = ((unsigned)(*datetimez >> 25) & 0x7f) + 80;
        mo = ((unsigned)(*datetimez >> 21) & 0x0f);
        dy = ((unsigned)(*datetimez >> 16) & 0x1f);

        hh = (((unsigned)*datetimez >> 11) & 0x1f);
        mm = (((unsigned)*datetimez >> 5) & 0x3f);
        ss = (((unsigned)*datetimez << 1) & 0x3e);
    }

    if (mo == 0 || mo > 12) {
        sprintf(monthbuf, BogusFmt, mo);
        monthstr = monthbuf;
    } else
        monthstr = month[mo-1];

    if (uO.lflag > 9)   /* verbose listing format */
        sprintf(d_t_str, lngYMDHMSTime, yr+1900, monthstr, dy,
          hh, mm, ss);
    else if (uO.T_flag)
        sprintf(d_t_str, DecimalTime, yr+1900, mo, dy,
          hh, mm, ss);
    else   /* was:  if ((uO.lflag >= 3) && (uO.lflag <= 5)) */
        sprintf(d_t_str, shtYMDHMTime, yr%100, monthstr, dy,
          hh, mm);

    return d_t_str;

} /* end function zi_time() */

#endif /* !NO_ZIPINFO */
