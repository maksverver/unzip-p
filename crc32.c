/*
  Copyright (c) 1990-2007 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
 * CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
 * tables for updating the shift register in one step with three exclusive-ors
 * instead of four steps with four exclusive-ors.  This results about a factor
 * of two increase in speed on a Power PC G4 (PPC7455) using gcc -O3.
 */

/* $Id: crc32.c,v 2.0 2007/01/07 05:20:36 spc Exp $ */

#define __CRC32_C       /* identifies this source module */

#include "zip.h"

#if (!defined(USE_ZLIB) || defined(USE_OWN_CRCTAB))

#  include "crc32.h"

/* When only the table of precomputed CRC values is needed, only the basic
   system-independent table containing 256 entries is created; any support
   for "unfolding" optimization is disabled.
 */
#  if (defined(USE_ZLIB) || defined(CRC_TABLE_ONLY))
#    ifdef IZ_CRCOPTIM_UNFOLDTBL
#      undef IZ_CRCOPTIM_UNFOLDTBL
#    endif
#  endif /* (USE_ZLIB || CRC_TABLE_ONLY) */

#  if defined(IZ_CRCOPTIM_UNFOLDTBL)
#    define CRC_TBLS  4
#  else
#    define CRC_TBLS  1
#  endif


/*
  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The first (or only) table is simply the CRC of all possible eight bit values.
  This is all the information needed to generate CRC's on data a byte-at-a-time
  for all combinations of CRC register values and incoming bytes.
  The remaining 3 tables (if IZ_CRCOPTIM_UNFOLDTBL is enabled) allow for
  word-at-a-time CRC calculation, where a word is four bytes.
*/

#  ifdef DYNAMIC_CRC_TABLE

/* =========================================================================
 * Make the crc table. This function is needed only if you want to compute
 * the table dynamically.
 */

local void make_crc_table(void);

#    if (defined(DYNALLOC_CRCTAB) && defined(REENTRANT))
   error: Dynamic allocation of CRC table not safe with reentrant code.
#    endif /* DYNALLOC_CRCTAB && REENTRANT */

#    ifdef DYNALLOC_CRCTAB
   local ulg *crc_table = NULL;
#      if 0          /* not used, since sizeof("near *") <= sizeof(int) */
   /* Use this section when access to a "local int" is faster than access to
      a "local pointer" (e.g.: i86 16bit code with far pointers). */
   local int crc_table_empty = 1;
#        define CRC_TABLE_IS_EMPTY    (crc_table_empty != 0)
#        define MARK_CRCTAB_FILLED    crc_table_empty = 0
#        define MARK_CRCTAB_EMPTY     crc_table_empty = 1
#      else
   /* Use this section on systems where the size of pointers and ints is
      equal (e.g.: all 32bit systems). */
#        define CRC_TABLE_IS_EMPTY    (crc_table == NULL)
#        define MARK_CRCTAB_FILLED    crc_table = crctab_p
#        define MARK_CRCTAB_EMPTY     crc_table = NULL
#      endif
#    else /* !DYNALLOC_CRCTAB */
   local ulg crc_table[CRC_TBLS*256];
   local int crc_table_empty = 1;
#      define CRC_TABLE_IS_EMPTY    (crc_table_empty != 0)
#      define MARK_CRCTAB_FILLED    crc_table_empty = 0
#    endif /* ?DYNALLOC_CRCTAB */


local void make_crc_table()
{
  ulg c;                /* crc shift register */
  int n;                /* counter for all possible eight bit values */
  int k;                /* byte being shifted into crc apparatus */
#    ifdef DYNALLOC_CRCTAB
  ulg *crctab_p;        /* temporary pointer to allocated crc_table area */
#    else /* !DYNALLOC_CRCTAB */
#      define crctab_p crc_table
#    endif /* DYNALLOC_CRCTAB */

#    ifdef COMPUTE_XOR_PATTERN
  /* This piece of code has been left here to explain how the XOR pattern
   * used in the creation of the crc_table values can be recomputed.
   * For production versions of this function, it is more efficient to
   * supply the resultant pattern at compile time.
   */
  ulg xor;              /* polynomial exclusive-or pattern */
  /* terms of polynomial defining this crc (except x^32): */
  static const uch p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* make exclusive-or pattern from polynomial (0xedb88320L) */
  xor = 0L;
  for (n = 0; n < sizeof(p)/sizeof(uch); n++)
    xor |= 1L << (31 - p[n]);
#    else
#      define xor 0xedb88320L
#    endif

#    ifdef DYNALLOC_CRCTAB
  crctab_p = (ulg *) malloc (CRC_TBLS*256*sizeof(ulg));
  if (crctab_p == NULL) {
    ziperr(ZE_MEM, "crc_table allocation");
  }
#    endif /* DYNALLOC_CRCTAB */

  /* generate a crc for every 8-bit value */
  for (n = 0; n < 256; n++) {
    c = (ulg)n;
    for (k = 8; k; k--)
      c = c & 1 ? xor ^ (c >> 1) : c >> 1;
    crctab_p[n] = REV_BE(c);
  }

#    ifdef IZ_CRCOPTIM_UNFOLDTBL
  /* generate crc for each value followed by one, two, and three zeros */
  for (n = 0; n < 256; n++) {
      c = crctab_p[n];
      for (k = 1; k < 4; k++) {
          c = CRC32(c, 0, crctab_p);
          crctab_p[k*256+n] = c;
      }
  }
#    endif /* IZ_CRCOPTIM_UNFOLDTBL */

  MARK_CRCTAB_FILLED;
}

#  else /* !DYNAMIC_CRC_TABLE */

#    ifdef DYNALLOC_CRCTAB
   error: Inconsistent flags, DYNALLOC_CRCTAB without DYNAMIC_CRC_TABLE.
#    endif

/* ========================================================================
 * Table of CRC-32's of all single-byte values (made by make_crc_table)
 */
local const ulg crc_table[CRC_TBLS*256] = {
#    ifdef IZ_CRC_BE_OPTIMIZ
    0x00000000L, 0x96300777L, 0x2c610eeeL, 0xba510999L, 0x19c46d07L,
    0x8ff46a70L, 0x35a563e9L, 0xa395649eL, 0x3288db0eL, 0xa4b8dc79L,
    0x1ee9d5e0L, 0x88d9d297L, 0x2b4cb609L, 0xbd7cb17eL, 0x072db8e7L,
    0x911dbf90L, 0x6410b71dL, 0xf220b06aL, 0x4871b9f3L, 0xde41be84L,
    0x7dd4da1aL, 0xebe4dd6dL, 0x51b5d4f4L, 0xc785d383L, 0x56986c13L,
    0xc0a86b64L, 0x7af962fdL, 0xecc9658aL, 0x4f5c0114L, 0xd96c0663L,
    0x633d0ffaL, 0xf50d088dL, 0xc8206e3bL, 0x5e10694cL, 0xe44160d5L,
    0x727167a2L, 0xd1e4033cL, 0x47d4044bL, 0xfd850dd2L, 0x6bb50aa5L,
    0xfaa8b535L, 0x6c98b242L, 0xd6c9bbdbL, 0x40f9bcacL, 0xe36cd832L,
    0x755cdf45L, 0xcf0dd6dcL, 0x593dd1abL, 0xac30d926L, 0x3a00de51L,
    0x8051d7c8L, 0x1661d0bfL, 0xb5f4b421L, 0x23c4b356L, 0x9995bacfL,
    0x0fa5bdb8L, 0x9eb80228L, 0x0888055fL, 0xb2d90cc6L, 0x24e90bb1L,
    0x877c6f2fL, 0x114c6858L, 0xab1d61c1L, 0x3d2d66b6L, 0x9041dc76L,
    0x0671db01L, 0xbc20d298L, 0x2a10d5efL, 0x8985b171L, 0x1fb5b606L,
    0xa5e4bf9fL, 0x33d4b8e8L, 0xa2c90778L, 0x34f9000fL, 0x8ea80996L,
    0x18980ee1L, 0xbb0d6a7fL, 0x2d3d6d08L, 0x976c6491L, 0x015c63e6L,
    0xf4516b6bL, 0x62616c1cL, 0xd8306585L, 0x4e0062f2L, 0xed95066cL,
    0x7ba5011bL, 0xc1f40882L, 0x57c40ff5L, 0xc6d9b065L, 0x50e9b712L,
    0xeab8be8bL, 0x7c88b9fcL, 0xdf1ddd62L, 0x492dda15L, 0xf37cd38cL,
    0x654cd4fbL, 0x5861b24dL, 0xce51b53aL, 0x7400bca3L, 0xe230bbd4L,
    0x41a5df4aL, 0xd795d83dL, 0x6dc4d1a4L, 0xfbf4d6d3L, 0x6ae96943L,
    0xfcd96e34L, 0x468867adL, 0xd0b860daL, 0x732d0444L, 0xe51d0333L,
    0x5f4c0aaaL, 0xc97c0dddL, 0x3c710550L, 0xaa410227L, 0x10100bbeL,
    0x86200cc9L, 0x25b56857L, 0xb3856f20L, 0x09d466b9L, 0x9fe461ceL,
    0x0ef9de5eL, 0x98c9d929L, 0x2298d0b0L, 0xb4a8d7c7L, 0x173db359L,
    0x810db42eL, 0x3b5cbdb7L, 0xad6cbac0L, 0x2083b8edL, 0xb6b3bf9aL,
    0x0ce2b603L, 0x9ad2b174L, 0x3947d5eaL, 0xaf77d29dL, 0x1526db04L,
    0x8316dc73L, 0x120b63e3L, 0x843b6494L, 0x3e6a6d0dL, 0xa85a6a7aL,
    0x0bcf0ee4L, 0x9dff0993L, 0x27ae000aL, 0xb19e077dL, 0x44930ff0L,
    0xd2a30887L, 0x68f2011eL, 0xfec20669L, 0x5d5762f7L, 0xcb676580L,
    0x71366c19L, 0xe7066b6eL, 0x761bd4feL, 0xe02bd389L, 0x5a7ada10L,
    0xcc4add67L, 0x6fdfb9f9L, 0xf9efbe8eL, 0x43beb717L, 0xd58eb060L,
    0xe8a3d6d6L, 0x7e93d1a1L, 0xc4c2d838L, 0x52f2df4fL, 0xf167bbd1L,
    0x6757bca6L, 0xdd06b53fL, 0x4b36b248L, 0xda2b0dd8L, 0x4c1b0aafL,
    0xf64a0336L, 0x607a0441L, 0xc3ef60dfL, 0x55df67a8L, 0xef8e6e31L,
    0x79be6946L, 0x8cb361cbL, 0x1a8366bcL, 0xa0d26f25L, 0x36e26852L,
    0x95770cccL, 0x03470bbbL, 0xb9160222L, 0x2f260555L, 0xbe3bbac5L,
    0x280bbdb2L, 0x925ab42bL, 0x046ab35cL, 0xa7ffd7c2L, 0x31cfd0b5L,
    0x8b9ed92cL, 0x1daede5bL, 0xb0c2649bL, 0x26f263ecL, 0x9ca36a75L,
    0x0a936d02L, 0xa906099cL, 0x3f360eebL, 0x85670772L, 0x13570005L,
    0x824abf95L, 0x147ab8e2L, 0xae2bb17bL, 0x381bb60cL, 0x9b8ed292L,
    0x0dbed5e5L, 0xb7efdc7cL, 0x21dfdb0bL, 0xd4d2d386L, 0x42e2d4f1L,
    0xf8b3dd68L, 0x6e83da1fL, 0xcd16be81L, 0x5b26b9f6L, 0xe177b06fL,
    0x7747b718L, 0xe65a0888L, 0x706a0fffL, 0xca3b0666L, 0x5c0b0111L,
    0xff9e658fL, 0x69ae62f8L, 0xd3ff6b61L, 0x45cf6c16L, 0x78e20aa0L,
    0xeed20dd7L, 0x5483044eL, 0xc2b30339L, 0x612667a7L, 0xf71660d0L,
    0x4d476949L, 0xdb776e3eL, 0x4a6ad1aeL, 0xdc5ad6d9L, 0x660bdf40L,
    0xf03bd837L, 0x53aebca9L, 0xc59ebbdeL, 0x7fcfb247L, 0xe9ffb530L,
    0x1cf2bdbdL, 0x8ac2bacaL, 0x3093b353L, 0xa6a3b424L, 0x0536d0baL,
    0x9306d7cdL, 0x2957de54L, 0xbf67d923L, 0x2e7a66b3L, 0xb84a61c4L,
    0x021b685dL, 0x942b6f2aL, 0x37be0bb4L, 0xa18e0cc3L, 0x1bdf055aL,
    0x8def022dL
#      ifdef IZ_CRCOPTIM_UNFOLDTBL
    ,
    0x00000000L, 0x41311b19L, 0x82623632L, 0xc3532d2bL, 0x04c56c64L,
    0x45f4777dL, 0x86a75a56L, 0xc796414fL, 0x088ad9c8L, 0x49bbc2d1L,
    0x8ae8effaL, 0xcbd9f4e3L, 0x0c4fb5acL, 0x4d7eaeb5L, 0x8e2d839eL,
    0xcf1c9887L, 0x5112c24aL, 0x1023d953L, 0xd370f478L, 0x9241ef61L,
    0x55d7ae2eL, 0x14e6b537L, 0xd7b5981cL, 0x96848305L, 0x59981b82L,
    0x18a9009bL, 0xdbfa2db0L, 0x9acb36a9L, 0x5d5d77e6L, 0x1c6c6cffL,
    0xdf3f41d4L, 0x9e0e5acdL, 0xa2248495L, 0xe3159f8cL, 0x2046b2a7L,
    0x6177a9beL, 0xa6e1e8f1L, 0xe7d0f3e8L, 0x2483dec3L, 0x65b2c5daL,
    0xaaae5d5dL, 0xeb9f4644L, 0x28cc6b6fL, 0x69fd7076L, 0xae6b3139L,
    0xef5a2a20L, 0x2c09070bL, 0x6d381c12L, 0xf33646dfL, 0xb2075dc6L,
    0x715470edL, 0x30656bf4L, 0xf7f32abbL, 0xb6c231a2L, 0x75911c89L,
    0x34a00790L, 0xfbbc9f17L, 0xba8d840eL, 0x79dea925L, 0x38efb23cL,
    0xff79f373L, 0xbe48e86aL, 0x7d1bc541L, 0x3c2ade58L, 0x054f79f0L,
    0x447e62e9L, 0x872d4fc2L, 0xc61c54dbL, 0x018a1594L, 0x40bb0e8dL,
    0x83e823a6L, 0xc2d938bfL, 0x0dc5a038L, 0x4cf4bb21L, 0x8fa7960aL,
    0xce968d13L, 0x0900cc5cL, 0x4831d745L, 0x8b62fa6eL, 0xca53e177L,
    0x545dbbbaL, 0x156ca0a3L, 0xd63f8d88L, 0x970e9691L, 0x5098d7deL,
    0x11a9ccc7L, 0xd2fae1ecL, 0x93cbfaf5L, 0x5cd76272L, 0x1de6796bL,
    0xdeb55440L, 0x9f844f59L, 0x58120e16L, 0x1923150fL, 0xda703824L,
    0x9b41233dL, 0xa76bfd65L, 0xe65ae67cL, 0x2509cb57L, 0x6438d04eL,
    0xa3ae9101L, 0xe29f8a18L, 0x21cca733L, 0x60fdbc2aL, 0xafe124adL,
    0xeed03fb4L, 0x2d83129fL, 0x6cb20986L, 0xab2448c9L, 0xea1553d0L,
    0x29467efbL, 0x687765e2L, 0xf6793f2fL, 0xb7482436L, 0x741b091dL,
    0x352a1204L, 0xf2bc534bL, 0xb38d4852L, 0x70de6579L, 0x31ef7e60L,
    0xfef3e6e7L, 0xbfc2fdfeL, 0x7c91d0d5L, 0x3da0cbccL, 0xfa368a83L,
    0xbb07919aL, 0x7854bcb1L, 0x3965a7a8L, 0x4b98833bL, 0x0aa99822L,
    0xc9fab509L, 0x88cbae10L, 0x4f5def5fL, 0x0e6cf446L, 0xcd3fd96dL,
    0x8c0ec274L, 0x43125af3L, 0x022341eaL, 0xc1706cc1L, 0x804177d8L,
    0x47d73697L, 0x06e62d8eL, 0xc5b500a5L, 0x84841bbcL, 0x1a8a4171L,
    0x5bbb5a68L, 0x98e87743L, 0xd9d96c5aL, 0x1e4f2d15L, 0x5f7e360cL,
    0x9c2d1b27L, 0xdd1c003eL, 0x120098b9L, 0x533183a0L, 0x9062ae8bL,
    0xd153b592L, 0x16c5f4ddL, 0x57f4efc4L, 0x94a7c2efL, 0xd596d9f6L,
    0xe9bc07aeL, 0xa88d1cb7L, 0x6bde319cL, 0x2aef2a85L, 0xed796bcaL,
    0xac4870d3L, 0x6f1b5df8L, 0x2e2a46e1L, 0xe136de66L, 0xa007c57fL,
    0x6354e854L, 0x2265f34dL, 0xe5f3b202L, 0xa4c2a91bL, 0x67918430L,
    0x26a09f29L, 0xb8aec5e4L, 0xf99fdefdL, 0x3accf3d6L, 0x7bfde8cfL,
    0xbc6ba980L, 0xfd5ab299L, 0x3e099fb2L, 0x7f3884abL, 0xb0241c2cL,
    0xf1150735L, 0x32462a1eL, 0x73773107L, 0xb4e17048L, 0xf5d06b51L,
    0x3683467aL, 0x77b25d63L, 0x4ed7facbL, 0x0fe6e1d2L, 0xccb5ccf9L,
    0x8d84d7e0L, 0x4a1296afL, 0x0b238db6L, 0xc870a09dL, 0x8941bb84L,
    0x465d2303L, 0x076c381aL, 0xc43f1531L, 0x850e0e28L, 0x42984f67L,
    0x03a9547eL, 0xc0fa7955L, 0x81cb624cL, 0x1fc53881L, 0x5ef42398L,
    0x9da70eb3L, 0xdc9615aaL, 0x1b0054e5L, 0x5a314ffcL, 0x996262d7L,
    0xd85379ceL, 0x174fe149L, 0x567efa50L, 0x952dd77bL, 0xd41ccc62L,
    0x138a8d2dL, 0x52bb9634L, 0x91e8bb1fL, 0xd0d9a006L, 0xecf37e5eL,
    0xadc26547L, 0x6e91486cL, 0x2fa05375L, 0xe836123aL, 0xa9070923L,
    0x6a542408L, 0x2b653f11L, 0xe479a796L, 0xa548bc8fL, 0x661b91a4L,
    0x272a8abdL, 0xe0bccbf2L, 0xa18dd0ebL, 0x62defdc0L, 0x23efe6d9L,
    0xbde1bc14L, 0xfcd0a70dL, 0x3f838a26L, 0x7eb2913fL, 0xb924d070L,
    0xf815cb69L, 0x3b46e642L, 0x7a77fd5bL, 0xb56b65dcL, 0xf45a7ec5L,
    0x370953eeL, 0x763848f7L, 0xb1ae09b8L, 0xf09f12a1L, 0x33cc3f8aL,
    0x72fd2493L
    ,
    0x00000000L, 0x376ac201L, 0x6ed48403L, 0x59be4602L, 0xdca80907L,
    0xebc2cb06L, 0xb27c8d04L, 0x85164f05L, 0xb851130eL, 0x8f3bd10fL,
    0xd685970dL, 0xe1ef550cL, 0x64f91a09L, 0x5393d808L, 0x0a2d9e0aL,
    0x3d475c0bL, 0x70a3261cL, 0x47c9e41dL, 0x1e77a21fL, 0x291d601eL,
    0xac0b2f1bL, 0x9b61ed1aL, 0xc2dfab18L, 0xf5b56919L, 0xc8f23512L,
    0xff98f713L, 0xa626b111L, 0x914c7310L, 0x145a3c15L, 0x2330fe14L,
    0x7a8eb816L, 0x4de47a17L, 0xe0464d38L, 0xd72c8f39L, 0x8e92c93bL,
    0xb9f80b3aL, 0x3cee443fL, 0x0b84863eL, 0x523ac03cL, 0x6550023dL,
    0x58175e36L, 0x6f7d9c37L, 0x36c3da35L, 0x01a91834L, 0x84bf5731L,
    0xb3d59530L, 0xea6bd332L, 0xdd011133L, 0x90e56b24L, 0xa78fa925L,
    0xfe31ef27L, 0xc95b2d26L, 0x4c4d6223L, 0x7b27a022L, 0x2299e620L,
    0x15f32421L, 0x28b4782aL, 0x1fdeba2bL, 0x4660fc29L, 0x710a3e28L,
    0xf41c712dL, 0xc376b32cL, 0x9ac8f52eL, 0xada2372fL, 0xc08d9a70L,
    0xf7e75871L, 0xae591e73L, 0x9933dc72L, 0x1c259377L, 0x2b4f5176L,
    0x72f11774L, 0x459bd575L, 0x78dc897eL, 0x4fb64b7fL, 0x16080d7dL,
    0x2162cf7cL, 0xa4748079L, 0x931e4278L, 0xcaa0047aL, 0xfdcac67bL,
    0xb02ebc6cL, 0x87447e6dL, 0xdefa386fL, 0xe990fa6eL, 0x6c86b56bL,
    0x5bec776aL, 0x02523168L, 0x3538f369L, 0x087faf62L, 0x3f156d63L,
    0x66ab2b61L, 0x51c1e960L, 0xd4d7a665L, 0xe3bd6464L, 0xba032266L,
    0x8d69e067L, 0x20cbd748L, 0x17a11549L, 0x4e1f534bL, 0x7975914aL,
    0xfc63de4fL, 0xcb091c4eL, 0x92b75a4cL, 0xa5dd984dL, 0x989ac446L,
    0xaff00647L, 0xf64e4045L, 0xc1248244L, 0x4432cd41L, 0x73580f40L,
    0x2ae64942L, 0x1d8c8b43L, 0x5068f154L, 0x67023355L, 0x3ebc7557L,
    0x09d6b756L, 0x8cc0f853L, 0xbbaa3a52L, 0xe2147c50L, 0xd57ebe51L,
    0xe839e25aL, 0xdf53205bL, 0x86ed6659L, 0xb187a458L, 0x3491eb5dL,
    0x03fb295cL, 0x5a456f5eL, 0x6d2fad5fL, 0x801b35e1L, 0xb771f7e0L,
    0xeecfb1e2L, 0xd9a573e3L, 0x5cb33ce6L, 0x6bd9fee7L, 0x3267b8e5L,
    0x050d7ae4L, 0x384a26efL, 0x0f20e4eeL, 0x569ea2ecL, 0x61f460edL,
    0xe4e22fe8L, 0xd388ede9L, 0x8a36abebL, 0xbd5c69eaL, 0xf0b813fdL,
    0xc7d2d1fcL, 0x9e6c97feL, 0xa90655ffL, 0x2c101afaL, 0x1b7ad8fbL,
    0x42c49ef9L, 0x75ae5cf8L, 0x48e900f3L, 0x7f83c2f2L, 0x263d84f0L,
    0x115746f1L, 0x944109f4L, 0xa32bcbf5L, 0xfa958df7L, 0xcdff4ff6L,
    0x605d78d9L, 0x5737bad8L, 0x0e89fcdaL, 0x39e33edbL, 0xbcf571deL,
    0x8b9fb3dfL, 0xd221f5ddL, 0xe54b37dcL, 0xd80c6bd7L, 0xef66a9d6L,
    0xb6d8efd4L, 0x81b22dd5L, 0x04a462d0L, 0x33cea0d1L, 0x6a70e6d3L,
    0x5d1a24d2L, 0x10fe5ec5L, 0x27949cc4L, 0x7e2adac6L, 0x494018c7L,
    0xcc5657c2L, 0xfb3c95c3L, 0xa282d3c1L, 0x95e811c0L, 0xa8af4dcbL,
    0x9fc58fcaL, 0xc67bc9c8L, 0xf1110bc9L, 0x740744ccL, 0x436d86cdL,
    0x1ad3c0cfL, 0x2db902ceL, 0x4096af91L, 0x77fc6d90L, 0x2e422b92L,
    0x1928e993L, 0x9c3ea696L, 0xab546497L, 0xf2ea2295L, 0xc580e094L,
    0xf8c7bc9fL, 0xcfad7e9eL, 0x9613389cL, 0xa179fa9dL, 0x246fb598L,
    0x13057799L, 0x4abb319bL, 0x7dd1f39aL, 0x3035898dL, 0x075f4b8cL,
    0x5ee10d8eL, 0x698bcf8fL, 0xec9d808aL, 0xdbf7428bL, 0x82490489L,
    0xb523c688L, 0x88649a83L, 0xbf0e5882L, 0xe6b01e80L, 0xd1dadc81L,
    0x54cc9384L, 0x63a65185L, 0x3a181787L, 0x0d72d586L, 0xa0d0e2a9L,
    0x97ba20a8L, 0xce0466aaL, 0xf96ea4abL, 0x7c78ebaeL, 0x4b1229afL,
    0x12ac6fadL, 0x25c6adacL, 0x1881f1a7L, 0x2feb33a6L, 0x765575a4L,
    0x413fb7a5L, 0xc429f8a0L, 0xf3433aa1L, 0xaafd7ca3L, 0x9d97bea2L,
    0xd073c4b5L, 0xe71906b4L, 0xbea740b6L, 0x89cd82b7L, 0x0cdbcdb2L,
    0x3bb10fb3L, 0x620f49b1L, 0x55658bb0L, 0x6822d7bbL, 0x5f4815baL,
    0x06f653b8L, 0x319c91b9L, 0xb48adebcL, 0x83e01cbdL, 0xda5e5abfL,
    0xed3498beL
    ,
    0x00000000L, 0x6567bcb8L, 0x8bc809aaL, 0xeeafb512L, 0x5797628fL,
    0x32f0de37L, 0xdc5f6b25L, 0xb938d79dL, 0xef28b4c5L, 0x8a4f087dL,
    0x64e0bd6fL, 0x018701d7L, 0xb8bfd64aL, 0xddd86af2L, 0x3377dfe0L,
    0x56106358L, 0x9f571950L, 0xfa30a5e8L, 0x149f10faL, 0x71f8ac42L,
    0xc8c07bdfL, 0xada7c767L, 0x43087275L, 0x266fcecdL, 0x707fad95L,
    0x1518112dL, 0xfbb7a43fL, 0x9ed01887L, 0x27e8cf1aL, 0x428f73a2L,
    0xac20c6b0L, 0xc9477a08L, 0x3eaf32a0L, 0x5bc88e18L, 0xb5673b0aL,
    0xd00087b2L, 0x6938502fL, 0x0c5fec97L, 0xe2f05985L, 0x8797e53dL,
    0xd1878665L, 0xb4e03addL, 0x5a4f8fcfL, 0x3f283377L, 0x8610e4eaL,
    0xe3775852L, 0x0dd8ed40L, 0x68bf51f8L, 0xa1f82bf0L, 0xc49f9748L,
    0x2a30225aL, 0x4f579ee2L, 0xf66f497fL, 0x9308f5c7L, 0x7da740d5L,
    0x18c0fc6dL, 0x4ed09f35L, 0x2bb7238dL, 0xc518969fL, 0xa07f2a27L,
    0x1947fdbaL, 0x7c204102L, 0x928ff410L, 0xf7e848a8L, 0x3d58149bL,
    0x583fa823L, 0xb6901d31L, 0xd3f7a189L, 0x6acf7614L, 0x0fa8caacL,
    0xe1077fbeL, 0x8460c306L, 0xd270a05eL, 0xb7171ce6L, 0x59b8a9f4L,
    0x3cdf154cL, 0x85e7c2d1L, 0xe0807e69L, 0x0e2fcb7bL, 0x6b4877c3L,
    0xa20f0dcbL, 0xc768b173L, 0x29c70461L, 0x4ca0b8d9L, 0xf5986f44L,
    0x90ffd3fcL, 0x7e5066eeL, 0x1b37da56L, 0x4d27b90eL, 0x284005b6L,
    0xc6efb0a4L, 0xa3880c1cL, 0x1ab0db81L, 0x7fd76739L, 0x9178d22bL,
    0xf41f6e93L, 0x03f7263bL, 0x66909a83L, 0x883f2f91L, 0xed589329L,
    0x546044b4L, 0x3107f80cL, 0xdfa84d1eL, 0xbacff1a6L, 0xecdf92feL,
    0x89b82e46L, 0x67179b54L, 0x027027ecL, 0xbb48f071L, 0xde2f4cc9L,
    0x3080f9dbL, 0x55e74563L, 0x9ca03f6bL, 0xf9c783d3L, 0x176836c1L,
    0x720f8a79L, 0xcb375de4L, 0xae50e15cL, 0x40ff544eL, 0x2598e8f6L,
    0x73888baeL, 0x16ef3716L, 0xf8408204L, 0x9d273ebcL, 0x241fe921L,
    0x41785599L, 0xafd7e08bL, 0xcab05c33L, 0x3bb659edL, 0x5ed1e555L,
    0xb07e5047L, 0xd519ecffL, 0x6c213b62L, 0x094687daL, 0xe7e932c8L,
    0x828e8e70L, 0xd49eed28L, 0xb1f95190L, 0x5f56e482L, 0x3a31583aL,
    0x83098fa7L, 0xe66e331fL, 0x08c1860dL, 0x6da63ab5L, 0xa4e140bdL,
    0xc186fc05L, 0x2f294917L, 0x4a4ef5afL, 0xf3762232L, 0x96119e8aL,
    0x78be2b98L, 0x1dd99720L, 0x4bc9f478L, 0x2eae48c0L, 0xc001fdd2L,
    0xa566416aL, 0x1c5e96f7L, 0x79392a4fL, 0x97969f5dL, 0xf2f123e5L,
    0x05196b4dL, 0x607ed7f5L, 0x8ed162e7L, 0xebb6de5fL, 0x528e09c2L,
    0x37e9b57aL, 0xd9460068L, 0xbc21bcd0L, 0xea31df88L, 0x8f566330L,
    0x61f9d622L, 0x049e6a9aL, 0xbda6bd07L, 0xd8c101bfL, 0x366eb4adL,
    0x53090815L, 0x9a4e721dL, 0xff29cea5L, 0x11867bb7L, 0x74e1c70fL,
    0xcdd91092L, 0xa8beac2aL, 0x46111938L, 0x2376a580L, 0x7566c6d8L,
    0x10017a60L, 0xfeaecf72L, 0x9bc973caL, 0x22f1a457L, 0x479618efL,
    0xa939adfdL, 0xcc5e1145L, 0x06ee4d76L, 0x6389f1ceL, 0x8d2644dcL,
    0xe841f864L, 0x51792ff9L, 0x341e9341L, 0xdab12653L, 0xbfd69aebL,
    0xe9c6f9b3L, 0x8ca1450bL, 0x620ef019L, 0x07694ca1L, 0xbe519b3cL,
    0xdb362784L, 0x35999296L, 0x50fe2e2eL, 0x99b95426L, 0xfcdee89eL,
    0x12715d8cL, 0x7716e134L, 0xce2e36a9L, 0xab498a11L, 0x45e63f03L,
    0x208183bbL, 0x7691e0e3L, 0x13f65c5bL, 0xfd59e949L, 0x983e55f1L,
    0x2106826cL, 0x44613ed4L, 0xaace8bc6L, 0xcfa9377eL, 0x38417fd6L,
    0x5d26c36eL, 0xb389767cL, 0xd6eecac4L, 0x6fd61d59L, 0x0ab1a1e1L,
    0xe41e14f3L, 0x8179a84bL, 0xd769cb13L, 0xb20e77abL, 0x5ca1c2b9L,
    0x39c67e01L, 0x80fea99cL, 0xe5991524L, 0x0b36a036L, 0x6e511c8eL,
    0xa7166686L, 0xc271da3eL, 0x2cde6f2cL, 0x49b9d394L, 0xf0810409L,
    0x95e6b8b1L, 0x7b490da3L, 0x1e2eb11bL, 0x483ed243L, 0x2d596efbL,
    0xc3f6dbe9L, 0xa6916751L, 0x1fa9b0ccL, 0x7ace0c74L, 0x9461b966L,
    0xf10605deL
#      endif /* IZ_CRCOPTIM_UNFOLDTBL */
#    else /* !IZ_CRC_BE_OPTIMIZ */
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
#      ifdef IZ_CRCOPTIM_UNFOLDTBL
    ,
    0x00000000L, 0x191b3141L, 0x32366282L, 0x2b2d53c3L, 0x646cc504L,
    0x7d77f445L, 0x565aa786L, 0x4f4196c7L, 0xc8d98a08L, 0xd1c2bb49L,
    0xfaefe88aL, 0xe3f4d9cbL, 0xacb54f0cL, 0xb5ae7e4dL, 0x9e832d8eL,
    0x87981ccfL, 0x4ac21251L, 0x53d92310L, 0x78f470d3L, 0x61ef4192L,
    0x2eaed755L, 0x37b5e614L, 0x1c98b5d7L, 0x05838496L, 0x821b9859L,
    0x9b00a918L, 0xb02dfadbL, 0xa936cb9aL, 0xe6775d5dL, 0xff6c6c1cL,
    0xd4413fdfL, 0xcd5a0e9eL, 0x958424a2L, 0x8c9f15e3L, 0xa7b24620L,
    0xbea97761L, 0xf1e8e1a6L, 0xe8f3d0e7L, 0xc3de8324L, 0xdac5b265L,
    0x5d5daeaaL, 0x44469febL, 0x6f6bcc28L, 0x7670fd69L, 0x39316baeL,
    0x202a5aefL, 0x0b07092cL, 0x121c386dL, 0xdf4636f3L, 0xc65d07b2L,
    0xed705471L, 0xf46b6530L, 0xbb2af3f7L, 0xa231c2b6L, 0x891c9175L,
    0x9007a034L, 0x179fbcfbL, 0x0e848dbaL, 0x25a9de79L, 0x3cb2ef38L,
    0x73f379ffL, 0x6ae848beL, 0x41c51b7dL, 0x58de2a3cL, 0xf0794f05L,
    0xe9627e44L, 0xc24f2d87L, 0xdb541cc6L, 0x94158a01L, 0x8d0ebb40L,
    0xa623e883L, 0xbf38d9c2L, 0x38a0c50dL, 0x21bbf44cL, 0x0a96a78fL,
    0x138d96ceL, 0x5ccc0009L, 0x45d73148L, 0x6efa628bL, 0x77e153caL,
    0xbabb5d54L, 0xa3a06c15L, 0x888d3fd6L, 0x91960e97L, 0xded79850L,
    0xc7cca911L, 0xece1fad2L, 0xf5facb93L, 0x7262d75cL, 0x6b79e61dL,
    0x4054b5deL, 0x594f849fL, 0x160e1258L, 0x0f152319L, 0x243870daL,
    0x3d23419bL, 0x65fd6ba7L, 0x7ce65ae6L, 0x57cb0925L, 0x4ed03864L,
    0x0191aea3L, 0x188a9fe2L, 0x33a7cc21L, 0x2abcfd60L, 0xad24e1afL,
    0xb43fd0eeL, 0x9f12832dL, 0x8609b26cL, 0xc94824abL, 0xd05315eaL,
    0xfb7e4629L, 0xe2657768L, 0x2f3f79f6L, 0x362448b7L, 0x1d091b74L,
    0x04122a35L, 0x4b53bcf2L, 0x52488db3L, 0x7965de70L, 0x607eef31L,
    0xe7e6f3feL, 0xfefdc2bfL, 0xd5d0917cL, 0xcccba03dL, 0x838a36faL,
    0x9a9107bbL, 0xb1bc5478L, 0xa8a76539L, 0x3b83984bL, 0x2298a90aL,
    0x09b5fac9L, 0x10aecb88L, 0x5fef5d4fL, 0x46f46c0eL, 0x6dd93fcdL,
    0x74c20e8cL, 0xf35a1243L, 0xea412302L, 0xc16c70c1L, 0xd8774180L,
    0x9736d747L, 0x8e2de606L, 0xa500b5c5L, 0xbc1b8484L, 0x71418a1aL,
    0x685abb5bL, 0x4377e898L, 0x5a6cd9d9L, 0x152d4f1eL, 0x0c367e5fL,
    0x271b2d9cL, 0x3e001cddL, 0xb9980012L, 0xa0833153L, 0x8bae6290L,
    0x92b553d1L, 0xddf4c516L, 0xc4eff457L, 0xefc2a794L, 0xf6d996d5L,
    0xae07bce9L, 0xb71c8da8L, 0x9c31de6bL, 0x852aef2aL, 0xca6b79edL,
    0xd37048acL, 0xf85d1b6fL, 0xe1462a2eL, 0x66de36e1L, 0x7fc507a0L,
    0x54e85463L, 0x4df36522L, 0x02b2f3e5L, 0x1ba9c2a4L, 0x30849167L,
    0x299fa026L, 0xe4c5aeb8L, 0xfdde9ff9L, 0xd6f3cc3aL, 0xcfe8fd7bL,
    0x80a96bbcL, 0x99b25afdL, 0xb29f093eL, 0xab84387fL, 0x2c1c24b0L,
    0x350715f1L, 0x1e2a4632L, 0x07317773L, 0x4870e1b4L, 0x516bd0f5L,
    0x7a468336L, 0x635db277L, 0xcbfad74eL, 0xd2e1e60fL, 0xf9ccb5ccL,
    0xe0d7848dL, 0xaf96124aL, 0xb68d230bL, 0x9da070c8L, 0x84bb4189L,
    0x03235d46L, 0x1a386c07L, 0x31153fc4L, 0x280e0e85L, 0x674f9842L,
    0x7e54a903L, 0x5579fac0L, 0x4c62cb81L, 0x8138c51fL, 0x9823f45eL,
    0xb30ea79dL, 0xaa1596dcL, 0xe554001bL, 0xfc4f315aL, 0xd7626299L,
    0xce7953d8L, 0x49e14f17L, 0x50fa7e56L, 0x7bd72d95L, 0x62cc1cd4L,
    0x2d8d8a13L, 0x3496bb52L, 0x1fbbe891L, 0x06a0d9d0L, 0x5e7ef3ecL,
    0x4765c2adL, 0x6c48916eL, 0x7553a02fL, 0x3a1236e8L, 0x230907a9L,
    0x0824546aL, 0x113f652bL, 0x96a779e4L, 0x8fbc48a5L, 0xa4911b66L,
    0xbd8a2a27L, 0xf2cbbce0L, 0xebd08da1L, 0xc0fdde62L, 0xd9e6ef23L,
    0x14bce1bdL, 0x0da7d0fcL, 0x268a833fL, 0x3f91b27eL, 0x70d024b9L,
    0x69cb15f8L, 0x42e6463bL, 0x5bfd777aL, 0xdc656bb5L, 0xc57e5af4L,
    0xee530937L, 0xf7483876L, 0xb809aeb1L, 0xa1129ff0L, 0x8a3fcc33L,
    0x9324fd72L
    ,
    0x00000000L, 0x01c26a37L, 0x0384d46eL, 0x0246be59L, 0x0709a8dcL,
    0x06cbc2ebL, 0x048d7cb2L, 0x054f1685L, 0x0e1351b8L, 0x0fd13b8fL,
    0x0d9785d6L, 0x0c55efe1L, 0x091af964L, 0x08d89353L, 0x0a9e2d0aL,
    0x0b5c473dL, 0x1c26a370L, 0x1de4c947L, 0x1fa2771eL, 0x1e601d29L,
    0x1b2f0bacL, 0x1aed619bL, 0x18abdfc2L, 0x1969b5f5L, 0x1235f2c8L,
    0x13f798ffL, 0x11b126a6L, 0x10734c91L, 0x153c5a14L, 0x14fe3023L,
    0x16b88e7aL, 0x177ae44dL, 0x384d46e0L, 0x398f2cd7L, 0x3bc9928eL,
    0x3a0bf8b9L, 0x3f44ee3cL, 0x3e86840bL, 0x3cc03a52L, 0x3d025065L,
    0x365e1758L, 0x379c7d6fL, 0x35dac336L, 0x3418a901L, 0x3157bf84L,
    0x3095d5b3L, 0x32d36beaL, 0x331101ddL, 0x246be590L, 0x25a98fa7L,
    0x27ef31feL, 0x262d5bc9L, 0x23624d4cL, 0x22a0277bL, 0x20e69922L,
    0x2124f315L, 0x2a78b428L, 0x2bbade1fL, 0x29fc6046L, 0x283e0a71L,
    0x2d711cf4L, 0x2cb376c3L, 0x2ef5c89aL, 0x2f37a2adL, 0x709a8dc0L,
    0x7158e7f7L, 0x731e59aeL, 0x72dc3399L, 0x7793251cL, 0x76514f2bL,
    0x7417f172L, 0x75d59b45L, 0x7e89dc78L, 0x7f4bb64fL, 0x7d0d0816L,
    0x7ccf6221L, 0x798074a4L, 0x78421e93L, 0x7a04a0caL, 0x7bc6cafdL,
    0x6cbc2eb0L, 0x6d7e4487L, 0x6f38fadeL, 0x6efa90e9L, 0x6bb5866cL,
    0x6a77ec5bL, 0x68315202L, 0x69f33835L, 0x62af7f08L, 0x636d153fL,
    0x612bab66L, 0x60e9c151L, 0x65a6d7d4L, 0x6464bde3L, 0x662203baL,
    0x67e0698dL, 0x48d7cb20L, 0x4915a117L, 0x4b531f4eL, 0x4a917579L,
    0x4fde63fcL, 0x4e1c09cbL, 0x4c5ab792L, 0x4d98dda5L, 0x46c49a98L,
    0x4706f0afL, 0x45404ef6L, 0x448224c1L, 0x41cd3244L, 0x400f5873L,
    0x4249e62aL, 0x438b8c1dL, 0x54f16850L, 0x55330267L, 0x5775bc3eL,
    0x56b7d609L, 0x53f8c08cL, 0x523aaabbL, 0x507c14e2L, 0x51be7ed5L,
    0x5ae239e8L, 0x5b2053dfL, 0x5966ed86L, 0x58a487b1L, 0x5deb9134L,
    0x5c29fb03L, 0x5e6f455aL, 0x5fad2f6dL, 0xe1351b80L, 0xe0f771b7L,
    0xe2b1cfeeL, 0xe373a5d9L, 0xe63cb35cL, 0xe7fed96bL, 0xe5b86732L,
    0xe47a0d05L, 0xef264a38L, 0xeee4200fL, 0xeca29e56L, 0xed60f461L,
    0xe82fe2e4L, 0xe9ed88d3L, 0xebab368aL, 0xea695cbdL, 0xfd13b8f0L,
    0xfcd1d2c7L, 0xfe976c9eL, 0xff5506a9L, 0xfa1a102cL, 0xfbd87a1bL,
    0xf99ec442L, 0xf85cae75L, 0xf300e948L, 0xf2c2837fL, 0xf0843d26L,
    0xf1465711L, 0xf4094194L, 0xf5cb2ba3L, 0xf78d95faL, 0xf64fffcdL,
    0xd9785d60L, 0xd8ba3757L, 0xdafc890eL, 0xdb3ee339L, 0xde71f5bcL,
    0xdfb39f8bL, 0xddf521d2L, 0xdc374be5L, 0xd76b0cd8L, 0xd6a966efL,
    0xd4efd8b6L, 0xd52db281L, 0xd062a404L, 0xd1a0ce33L, 0xd3e6706aL,
    0xd2241a5dL, 0xc55efe10L, 0xc49c9427L, 0xc6da2a7eL, 0xc7184049L,
    0xc25756ccL, 0xc3953cfbL, 0xc1d382a2L, 0xc011e895L, 0xcb4dafa8L,
    0xca8fc59fL, 0xc8c97bc6L, 0xc90b11f1L, 0xcc440774L, 0xcd866d43L,
    0xcfc0d31aL, 0xce02b92dL, 0x91af9640L, 0x906dfc77L, 0x922b422eL,
    0x93e92819L, 0x96a63e9cL, 0x976454abL, 0x9522eaf2L, 0x94e080c5L,
    0x9fbcc7f8L, 0x9e7eadcfL, 0x9c381396L, 0x9dfa79a1L, 0x98b56f24L,
    0x99770513L, 0x9b31bb4aL, 0x9af3d17dL, 0x8d893530L, 0x8c4b5f07L,
    0x8e0de15eL, 0x8fcf8b69L, 0x8a809decL, 0x8b42f7dbL, 0x89044982L,
    0x88c623b5L, 0x839a6488L, 0x82580ebfL, 0x801eb0e6L, 0x81dcdad1L,
    0x8493cc54L, 0x8551a663L, 0x8717183aL, 0x86d5720dL, 0xa9e2d0a0L,
    0xa820ba97L, 0xaa6604ceL, 0xaba46ef9L, 0xaeeb787cL, 0xaf29124bL,
    0xad6fac12L, 0xacadc625L, 0xa7f18118L, 0xa633eb2fL, 0xa4755576L,
    0xa5b73f41L, 0xa0f829c4L, 0xa13a43f3L, 0xa37cfdaaL, 0xa2be979dL,
    0xb5c473d0L, 0xb40619e7L, 0xb640a7beL, 0xb782cd89L, 0xb2cddb0cL,
    0xb30fb13bL, 0xb1490f62L, 0xb08b6555L, 0xbbd72268L, 0xba15485fL,
    0xb853f606L, 0xb9919c31L, 0xbcde8ab4L, 0xbd1ce083L, 0xbf5a5edaL,
    0xbe9834edL
   ,
    0x00000000L, 0xb8bc6765L, 0xaa09c88bL, 0x12b5afeeL, 0x8f629757L,
    0x37def032L, 0x256b5fdcL, 0x9dd738b9L, 0xc5b428efL, 0x7d084f8aL,
    0x6fbde064L, 0xd7018701L, 0x4ad6bfb8L, 0xf26ad8ddL, 0xe0df7733L,
    0x58631056L, 0x5019579fL, 0xe8a530faL, 0xfa109f14L, 0x42acf871L,
    0xdf7bc0c8L, 0x67c7a7adL, 0x75720843L, 0xcdce6f26L, 0x95ad7f70L,
    0x2d111815L, 0x3fa4b7fbL, 0x8718d09eL, 0x1acfe827L, 0xa2738f42L,
    0xb0c620acL, 0x087a47c9L, 0xa032af3eL, 0x188ec85bL, 0x0a3b67b5L,
    0xb28700d0L, 0x2f503869L, 0x97ec5f0cL, 0x8559f0e2L, 0x3de59787L,
    0x658687d1L, 0xdd3ae0b4L, 0xcf8f4f5aL, 0x7733283fL, 0xeae41086L,
    0x525877e3L, 0x40edd80dL, 0xf851bf68L, 0xf02bf8a1L, 0x48979fc4L,
    0x5a22302aL, 0xe29e574fL, 0x7f496ff6L, 0xc7f50893L, 0xd540a77dL,
    0x6dfcc018L, 0x359fd04eL, 0x8d23b72bL, 0x9f9618c5L, 0x272a7fa0L,
    0xbafd4719L, 0x0241207cL, 0x10f48f92L, 0xa848e8f7L, 0x9b14583dL,
    0x23a83f58L, 0x311d90b6L, 0x89a1f7d3L, 0x1476cf6aL, 0xaccaa80fL,
    0xbe7f07e1L, 0x06c36084L, 0x5ea070d2L, 0xe61c17b7L, 0xf4a9b859L,
    0x4c15df3cL, 0xd1c2e785L, 0x697e80e0L, 0x7bcb2f0eL, 0xc377486bL,
    0xcb0d0fa2L, 0x73b168c7L, 0x6104c729L, 0xd9b8a04cL, 0x446f98f5L,
    0xfcd3ff90L, 0xee66507eL, 0x56da371bL, 0x0eb9274dL, 0xb6054028L,
    0xa4b0efc6L, 0x1c0c88a3L, 0x81dbb01aL, 0x3967d77fL, 0x2bd27891L,
    0x936e1ff4L, 0x3b26f703L, 0x839a9066L, 0x912f3f88L, 0x299358edL,
    0xb4446054L, 0x0cf80731L, 0x1e4da8dfL, 0xa6f1cfbaL, 0xfe92dfecL,
    0x462eb889L, 0x549b1767L, 0xec277002L, 0x71f048bbL, 0xc94c2fdeL,
    0xdbf98030L, 0x6345e755L, 0x6b3fa09cL, 0xd383c7f9L, 0xc1366817L,
    0x798a0f72L, 0xe45d37cbL, 0x5ce150aeL, 0x4e54ff40L, 0xf6e89825L,
    0xae8b8873L, 0x1637ef16L, 0x048240f8L, 0xbc3e279dL, 0x21e91f24L,
    0x99557841L, 0x8be0d7afL, 0x335cb0caL, 0xed59b63bL, 0x55e5d15eL,
    0x47507eb0L, 0xffec19d5L, 0x623b216cL, 0xda874609L, 0xc832e9e7L,
    0x708e8e82L, 0x28ed9ed4L, 0x9051f9b1L, 0x82e4565fL, 0x3a58313aL,
    0xa78f0983L, 0x1f336ee6L, 0x0d86c108L, 0xb53aa66dL, 0xbd40e1a4L,
    0x05fc86c1L, 0x1749292fL, 0xaff54e4aL, 0x322276f3L, 0x8a9e1196L,
    0x982bbe78L, 0x2097d91dL, 0x78f4c94bL, 0xc048ae2eL, 0xd2fd01c0L,
    0x6a4166a5L, 0xf7965e1cL, 0x4f2a3979L, 0x5d9f9697L, 0xe523f1f2L,
    0x4d6b1905L, 0xf5d77e60L, 0xe762d18eL, 0x5fdeb6ebL, 0xc2098e52L,
    0x7ab5e937L, 0x680046d9L, 0xd0bc21bcL, 0x88df31eaL, 0x3063568fL,
    0x22d6f961L, 0x9a6a9e04L, 0x07bda6bdL, 0xbf01c1d8L, 0xadb46e36L,
    0x15080953L, 0x1d724e9aL, 0xa5ce29ffL, 0xb77b8611L, 0x0fc7e174L,
    0x9210d9cdL, 0x2aacbea8L, 0x38191146L, 0x80a57623L, 0xd8c66675L,
    0x607a0110L, 0x72cfaefeL, 0xca73c99bL, 0x57a4f122L, 0xef189647L,
    0xfdad39a9L, 0x45115eccL, 0x764dee06L, 0xcef18963L, 0xdc44268dL,
    0x64f841e8L, 0xf92f7951L, 0x41931e34L, 0x5326b1daL, 0xeb9ad6bfL,
    0xb3f9c6e9L, 0x0b45a18cL, 0x19f00e62L, 0xa14c6907L, 0x3c9b51beL,
    0x842736dbL, 0x96929935L, 0x2e2efe50L, 0x2654b999L, 0x9ee8defcL,
    0x8c5d7112L, 0x34e11677L, 0xa9362eceL, 0x118a49abL, 0x033fe645L,
    0xbb838120L, 0xe3e09176L, 0x5b5cf613L, 0x49e959fdL, 0xf1553e98L,
    0x6c820621L, 0xd43e6144L, 0xc68bceaaL, 0x7e37a9cfL, 0xd67f4138L,
    0x6ec3265dL, 0x7c7689b3L, 0xc4caeed6L, 0x591dd66fL, 0xe1a1b10aL,
    0xf3141ee4L, 0x4ba87981L, 0x13cb69d7L, 0xab770eb2L, 0xb9c2a15cL,
    0x017ec639L, 0x9ca9fe80L, 0x241599e5L, 0x36a0360bL, 0x8e1c516eL,
    0x866616a7L, 0x3eda71c2L, 0x2c6fde2cL, 0x94d3b949L, 0x090481f0L,
    0xb1b8e695L, 0xa30d497bL, 0x1bb12e1eL, 0x43d23e48L, 0xfb6e592dL,
    0xe9dbf6c3L, 0x516791a6L, 0xccb0a91fL, 0x740cce7aL, 0x66b96194L,
    0xde0506f1L
#      endif /* IZ_CRCOPTIM_UNFOLDTBL */
#    endif /* ? IZ_CRC_BE_OPTIMIZ */
};
#  endif /* ?DYNAMIC_CRC_TABLE */

/* use "(void)" here to work around a Borland TC++ 1.0 problem */
#  ifdef USE_ZLIB
const uLongf *get_crc_table(void)
#  else
const ulg *get_crc_table(void)
#  endif
{
#  ifdef DYNAMIC_CRC_TABLE
  if (CRC_TABLE_IS_EMPTY)
    make_crc_table();
#  endif
#  ifdef USE_ZLIB
  return (const uLongf *)crc_table;
#  else
  return crc_table;
#  endif
}

#  ifdef DYNALLOC_CRCTAB
void free_crc_table()
{
  if (!CRC_TABLE_IS_EMPTY)
  {
    free((ulg *)crc_table);
    MARK_CRCTAB_EMPTY;
  }
}
#  endif

#  ifndef USE_ZLIB
#    ifndef CRC_TABLE_ONLY
#      ifndef ASM_CRC

#        define DO1(crc, buf)  crc = CRC32(crc, *buf++, crc_32_tab)
#        define DO2(crc, buf)  DO1(crc, buf); DO1(crc, buf)
#        define DO4(crc, buf)  DO2(crc, buf); DO2(crc, buf)
#        define DO8(crc, buf)  DO4(crc, buf); DO4(crc, buf)

#        if (defined(IZ_CRC_BE_OPTIMIZ) || defined(IZ_CRC_LE_OPTIMIZ))

#          ifdef IZ_CRCOPTIM_UNFOLDTBL
#            ifdef IZ_CRC_BE_OPTIMIZ
#              define DO_OPT4(c, buf4)  c ^= *(buf4)++; \
        c = crc_32_tab[c & 0xff] ^ crc_32_tab[256+((c>>8) & 0xff)] ^ \
            crc_32_tab[2*256+((c>>16) & 0xff)] ^ crc_32_tab[3*256+(c>>24)]
#            else /* !IZ_CRC_BE_OPTIMIZ */
#              define DO_OPT4(c, buf4)  c ^= *(buf4)++; \
        c = crc_32_tab[3*256+(c & 0xff)] ^ crc_32_tab[2*256+((c>>8) & 0xff)] \
           ^ crc_32_tab[256+((c>>16) & 0xff)] ^ crc_32_tab[c>>24]
#            endif /* ?IZ_CRC_BE_OPTIMIZ */
#          else /* !IZ_CRCOPTIM_UNFOLDTBL */
#            define DO_OPT4(c, buf4)  c ^= *(buf4)++; \
       c = CRC32UPD(c, crc_32_tab); \
       c = CRC32UPD(c, crc_32_tab); \
       c = CRC32UPD(c, crc_32_tab); \
       c = CRC32UPD(c, crc_32_tab)
#          endif /* ?IZ_CRCOPTIM_UNFOLDTBL */

#          define DO_OPT16(crc, buf4) DO_OPT4(crc, buf4); DO_OPT4(crc, buf4); \
                             DO_OPT4(crc, buf4); DO_OPT4(crc, buf4);

#        endif /* (IZ_CRC_BE_OPTIMIZ || IZ_CRC_LE_OPTIMIZ) */


/* ========================================================================= */
ulg crc32(crc, buf, len)
    ulg crc;                    /* crc shift register */
    const uch *buf;            /* pointer to bytes to pump through */
    extent len;                 /* number of bytes in buf[] */
/* Run a set of bytes through the crc shift register.  If buf is a NULL
   pointer, then initialize the crc shift register contents instead.
   Return the current crc in either case. */
{
  z_uint4 c;
  const ulg *crc_32_tab;

  if (buf == NULL) return 0L;

  crc_32_tab = get_crc_table();

  c = (REV_BE((z_uint4)crc) ^ 0xffffffffL);

#        if (defined(IZ_CRC_BE_OPTIMIZ) || defined(IZ_CRC_LE_OPTIMIZ))
  /* Align buf pointer to next DWORD boundary. */
  while (len && ((ptrdiff_t)buf & 3)) {
    DO1(c, buf);
    len--;
  }
  {
    const z_uint4 *buf4 = (const z_uint4 *)buf;
    while (len >= 16) {
      DO_OPT16(c, buf4);
      len -= 16;
    }
    while (len >= 4) {
      DO_OPT4(c, buf4);
      len -= 4;
    }
    buf = (const uch *)buf4;
  }
#        else /* !(IZ_CRC_BE_OPTIMIZ || IZ_CRC_LE_OPTIMIZ) */
#          ifndef NO_UNROLLED_LOOPS
  while (len >= 8) {
    DO8(c, buf);
    len -= 8;
  }
#          endif /* !NO_UNROLLED_LOOPS */
#        endif /* ?(IZ_CRC_BE_OPTIMIZ || IZ_CRC_LE_OPTIMIZ) */
  if (len) do {
    DO1(c, buf);
  } while (--len);

  return REV_BE(c) ^ 0xffffffffL;   /* (instead of ~c for 64-bit machines) */
}
#      endif /* !ASM_CRC */
#    endif /* !CRC_TABLE_ONLY */
#  endif /* !USE_ZLIB */
#endif /* !USE_ZLIB || USE_OWN_CRCTAB */
