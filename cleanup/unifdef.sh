#!/usr/bin/bash

# This script uses unifdef (https://dotat.at/prog/unifdef/) to remove code
# that requires precompiler variables that are set only for obsolete
# operating systems/compilers which are no longer supported.
#
# Unfortunately, unifdef2 by itself isn't very powerful (and unifdef3 doesn't
# work correctly!) so a bit of postprocessing is necessary to make the code look
# nice enough to commit.
#
# This script is intended to be run from the main directory:
#
# % cleanup/unifdef.sh

sources=(
    ./api.c
    ./apihelp.c
    ./consts.h
    ./crc32.c
    ./crc32.h
    ./crypt.c
    ./crypt.h
    ./ebcdic.h
    ./envargs.c
    ./explode.c
    ./extract.c
    ./fileio.c
    ./funzip.c
    ./gbloffs.c
    ./globals.c
    ./globals.h
    ./inflate.c
    ./inflate.h
    ./list.c
    ./match.c
    ./process.c
    ./timezone.c
    ./timezone.h
    ./ttyio.c
    ./ttyio.h
    ./ubz2err.c
    ./unix/unix.c
    ./unix/unxcfg.h
    ./unreduce.c
    ./unshrink.c
    ./unzip.c
    ./unzip.h
    ./unzipstb.c
    ./unzpriv.h
    ./unzvers.h
    ./zip.h
    ./zipinfo.c
)

undefined=(
    __16BIT__
    __386BSD__
    _AIX
    AMIGA
    AOS_VS
    __APPLE__
    ATARI
    ATH_BEO
    __ATHEOS__
    __BEOS__
    __BORLANDC__
    BORLAND_STAT_BUG
    CMS_MVS
    COHERENT
    __convex__
    CONVEX
    __convexc__
    cray
    CRAY
    _CRT_NON_CONFORMING_SWPRINTFS
    _CRT_NONSTDC_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    __DGUX__
    __DJGPP__
    DLL
    DNIX
    DOS_FLX_H68_NLM_OS2_W32
    DOS_FLX_H68_OS2_W32
    DOS_FLX_NLM_OS2_W32
    DOS_FLX_OS2
    DOS_FLX_OS2_W32
    DOS_H68_OS2_W32
    DOS_NLM_OS2_W32
    DOS_OS2
    DOS_OS2_W32
    DOSWILD
    __EMX__
    FLEXOS
    __GO32__
    gould
    __HIGHC__
    __HP_cc
    __hpux
    __human68k__
    __IBMC__
    i386
    __i386
    __i386__
    i486
    __i486
    __i486__
    i586
    __i586
    __i586__
    i686
    __i686
    __i686__
    INT_16BIT
    __LCC__
    Lynx
    macintosh
    MACOS
    mc68000
    __mc68020__
    mc68020
    MED_MEM
    M_I86
    __MINGW32__
    MINIX
    __MINT__
    mips
    __mpexl
    MPW
    MSC
    _MSC_VER
    __MSDOS__
    MSDOS
    __MSVCRT_VERSION__
    M_SYS5
    M_SYSV
    MTS
    M_UNIX
    MVS
    __MWERKS__
    M_XENIX
    NeXT
    NLM
    NOVELL_BUG_FAILSAFE
    NO_W32TIMES_IZFIX
    NTSD_EAS
    NX_CURRENT_COMPILER_RELEASE
    OLD_THEOS_EXTRA
    __OS2__
    OS2
    OS2DLL
    OS2_EAS
    OS2_W32
    OS390
    __osf__
    POCKET_UNZIP
    __POWERC
    __ppc__
    __ppc64__
    pyr
    pyr_bsd
    QDOS
    QLZIP
    __QNX__
    __QNXNTO__
    realix
    REALLY_SHORT_SYMS
    REGULUS
    _RELEASE
    __riscos
    RISCOS
    __RSXNT__
    __SASC
    SCO_XENIX
    __sgi
    sgi
    SHORT_NAMES
    SHORT_SYMS
    SMALL_MEM
    sparc
    sun
    sun386
    __SUNPRO_C
    __svr4__
    __SVR4
    SYS_T20
    __SYSTEM_FIVE
    SYSTEM_FIVE
    T20_VMS
    __TANDEM
    TANDEM
    THEOS
    THEOS_SUPPORT
    THINK_C
    TOPS20
    __TURBOC__
    __ultrix
    ultrix
    _UNICOS
    uts
    UTS
    V7
    vax
    VM_CMS
    __VMS
    VMS
    VMSCLI
    VMSWILD
    W32_STAT_BANDAID
    __WATCOMC__
    __WIN32__
    _WIN32
    WIN32
    WIN32_LEAN_AND_MEAN
    _WIN32_WCE
    WINDLL
    __WINNT
    __WINNT__
    ZMEM
)

unifdef $(for key in "${undefined[@]}"; do echo -U$key; done) -m "${sources[@]}"

# unifdef2 doesn't simplify complex expressions containing partially known
# subexpressions. For example, with -Ufoo "#if defined(foo)" would be
# removed, but "#if defined(foo) || defined(bar)" would be left as is,
# even though it can be simplified to "#if defined(bar)".

for key in "${undefined[@]}"; do
    # If `foo` is undefined, replace:
    #
    #   "defined(foo) || bar" with "bar"
    #   "bar || defined(foo)" with "foo"
    #   "!defined(foo) && bar" with "bar"
    #   "bar && !defined(foo)" with "bar"
    # 
    echo "s/defined($key)\s*||\s*//g"
    echo "s/\s*||\s*defined($key)//g"
    echo "s/!\s*defined($key)\s*&&\s*//g"
    echo "s/\s*&&\s*!\s*defined($key)//g"

    # In comments, replace
    #   "#endif /* foo || bar */" with "#endif /* bar */"
    # etc.
    echo "/^#/s/$key\s*||\s*//g"
    echo "/^#/s/\s*||\s*$key//g"
    echo "/^#/s/!$key\s*&&\s*//g"
    echo "/^#/s/\s*&&\s*!$key//g"

done | sed -f - -i "${sources[@]}"

# Reindent preprocessor directives, which unifdef2 doesn't do by itself,
# unfortunately.
cleanup/reindent-pp.py "${sources[@]}"
