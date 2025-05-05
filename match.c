/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*---------------------------------------------------------------------------

  match.c

  The match() routine recursively compares a string to a "pattern" (regular
  expression), returning TRUE if a match is found or FALSE if not.  This
  version is specifically for use with unzip.c:  as did the previous match()
  routines from SEA and J. Kercheval, it leaves the case (upper, lower, or
  mixed) of the string alone, but converts any uppercase characters in the
  pattern to lowercase if indicated by the global var pInfo->lcflag (which
  is to say, string is assumed to have been converted to lowercase already,
  if such was necessary).

  GRR:  reversed order of text, pattern in matche() (now same as match());
        added ignore_case/ic flags, Case() macro.

  PaulK:  replaced matche() with recmatch() from Zip, modified to have an
          ignore_case argument; replaced test frame with simpler one.

  ---------------------------------------------------------------------------

  Copyright on recmatch() from Zip's util.c
	 Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

	 See the accompanying file LICENSE, version 2004-May-22 or later
	 for terms of use.
	 If, for some reason, both of these files are missing, the Info-ZIP license
	 also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html  


  ---------------------------------------------------------------------------

  Match the pattern (wildcard) against the string (fixed):

     match(string, pattern, ignore_case, sepc);

  returns TRUE if string matches pattern, FALSE otherwise.  In the pattern:

     `*' matches any sequence of characters (zero or more)
     `?' matches any single character
     [SET] matches any character in the specified set,
     [!SET] or [^SET] matches any character not in the specified set.

  A set is composed of characters or ranges; a range looks like ``character
  hyphen character'' (as in 0-9 or A-Z).  [0-9a-zA-Z_] is the minimal set of
  characters ALlowed in the [..] pattern construct.  Other characters are
  allowed (i.e., 8-bit characters) if your system will support them.

  To suppress the special syntactic significance of any of ``[]*?!^-\'', in-
  side or outside a [..] construct, and match the character exactly, precede
  it with a ``\'' (backslash).

  Note that "*.*" and "*." are treated specially under MS-DOS if DOSWILD is
  defined.  See the DOSWILD section below for an explanation.  Note also
  that with VMSWILD defined, '%' is used instead of '?', and sets (ranges)
  are delimited by () instead of [].

  ---------------------------------------------------------------------------*/


#define __MATCH_C       /* identifies this source module */

/* define ToLower() in here (for Unix, define ToLower to be macro (using
 * isupper()); otherwise just use tolower() */
#define UNZIP_INTERNAL
#include "unzip.h"

#ifndef THEOS   /* the Theos port defines its own variant of match() */

#if 0  /* this is not useful until it matches Amiga names insensitively */
#ifdef AMIGA        /* some other platforms might also want to use this */
#  define ANSI_CHARSET       /* MOVE INTO UNZIP.H EVENTUALLY */
#endif
#endif /* 0 */

#ifdef ANSI_CHARSET
#  ifdef ToLower
#    undef ToLower
#  endif
   /* uppercase letters are values 41 thru 5A, C0 thru D6, and D8 thru DE */
#  define IsUpper(c) (c>=0xC0 ? c<=0xDE && c!=0xD7 : c>=0x41 && c<=0x5A)
#  define ToLower(c) (IsUpper((uch) c) ? (unsigned) c | 0x20 : (unsigned) c)
#endif
#define Case(x)  (ic? ToLower(x) : (x))

#ifdef VMSWILD
#  define WILDCHAR   '%'
#  define BEG_RANGE  '('
#  define END_RANGE  ')'
#else
#  define WILDCHAR   '?'
#  define BEG_RANGE  '['
#  define END_RANGE  ']'
#  define WILDCHR_SINGLE '?'
#  define DIRSEP_CHR '/'
#  define WILDCHR_MULTI '*'
#endif

#ifdef WILD_STOP_AT_DIR
   int wild_stop_at_dir = 1; /* default wildcards do not include / in matches */
#else
   int wild_stop_at_dir = 0; /* default wildcards do include / in matches */
#endif



/*
 * case mapping functions. case_map is used to ignore case in comparisons,
 * to_up is used to force upper case even on Unix (for dosify option).
 */
#ifdef USE_CASE_MAP
#  define case_map(c) upper[(c) & 0xff]
#  define to_up(c)    upper[(c) & 0xff]
#else
#  define case_map(c) (c)
#  define to_up(c)    ((c) >= 'a' && (c) <= 'z' ? (c)-'a'+'A' : (c))
#endif /* USE_CASE_MAP */


#if 0                /* GRR:  add this to unzip.h someday... */
#if !(defined(MSDOS) && defined(DOSWILD))
#ifdef WILD_STOP_AT_DIR
#define match(s,p,ic,sc) (recmatch((ZCONST uch *)p,(ZCONST uch *)s,ic,sc) == 1)
#else
#define match(s,p,ic)    (recmatch((ZCONST uch *)p,(ZCONST uch *)s,ic) == 1)
#endif
int recmatch OF((ZCONST uch *pattern, ZCONST uch *string,
                 int ignore_case __WDLPRO));
#endif
#endif /* 0 */
static int recmatch OF((ZCONST char *, ZCONST char *, 
                        int));
static char *isshexp OF((ZCONST char *p));
static int namecmp OF((ZCONST char *s1, ZCONST char *s2));


/* match() is a shell to recmatch() to return only Boolean values. */

int match(string, pattern, ignore_case __WDL)
    ZCONST char *string, *pattern;
    int ignore_case;
    __WDLDEF
{
#if (defined(MSDOS) && defined(DOSWILD))
    char *dospattern;
    int j = strlen(pattern);

/*---------------------------------------------------------------------------
    Optional MS-DOS preprocessing section:  compare last three chars of the
    wildcard to "*.*" and translate to "*" if found; else compare the last
    two characters to "*." and, if found, scan the non-wild string for dots.
    If in the latter case a dot is found, return failure; else translate the
    "*." to "*".  In either case, continue with the normal (Unix-like) match
    procedure after translation.  (If not enough memory, default to normal
    match.)  This causes "a*.*" and "a*." to behave as MS-DOS users expect.
  ---------------------------------------------------------------------------*/

    if ((dospattern = (char *)malloc(j+1)) != NULL) {
        strcpy(dospattern, pattern);
        if (!strcmp(dospattern+j-3, "*.*")) {
            dospattern[j-2] = '\0';                    /* nuke the ".*" */
        } else if (!strcmp(dospattern+j-2, "*.")) {
            char *p = MBSCHR(string, '.');

            if (p) {   /* found a dot:  match fails */
                free(dospattern);
                return 0;
            }
            dospattern[j-1] = '\0';                    /* nuke the end "." */
        }
        j = recmatch(dospattern, string, ignore_case);
        free(dospattern);
        return j == 1;
    } else
#endif /* MSDOS && DOSWILD */
    return recmatch(pattern, string, ignore_case) == 1;
}

#ifdef _MBCS

char *___tmp_ptr;

#endif

static int recmatch(p, s, cs)
ZCONST char *p;         /* sh pattern to match */
ZCONST char *s;         /* string to match it to */
int cs;                 /* flag: force case-sensitive matching */
/* Recursively compare the sh pattern p with the string s and return 1 if
   they match, and 0 or 2 if they don't or if there is a syntax error in the
   pattern.  This routine recurses on itself no deeper than the number of
   characters in the pattern. */
{
  int c;                /* pattern char or start of range in [-] loop */
  /* Get first character, the pattern for new recmatch calls follows */
 /* borrowed from Zip's global.c */
 int no_wild = 0; 
 int allow_regex=1;
  /* This fix provided by akt@m5.dion.ne.jp for Japanese.
     See 21 July 2006 mail.
     It only applies when p is pointing to a doublebyte character and
     things like / and wildcards are not doublebyte.  This probably
     should not be needed. */

#ifdef _MBCS
  if (CLEN(p) == 2) {
    if (CLEN(s) == 2) {
      return (*p == *s && *(p+1) == *(s+1)) ?
        recmatch(p + 2, s + 2, cs) : 0;
    } else {
      return 0;
    }
  }
#endif /* ?_MBCS */

  c = *POSTINCSTR(p);

  /* If that was the end of the pattern, match if string empty too */
  if (c == 0)
    return *s == 0;

  /* '?' (or '%' or '#') matches any character (but not an empty string) */
  if (c == WILDCHR_SINGLE) {
    if (wild_stop_at_dir)
      return (*s && *s != DIRSEP_CHR) ? recmatch(p, s + CLEN(s), cs) : 0;
    else
      return *s ? recmatch(p, s + CLEN(s), cs) : 0;
  }

  /* WILDCHR_MULTI ('*') matches any number of characters, including zero */
#ifdef AMIGA
  if (!no_wild && c == '#' && *p == '?')            /* "#?" is Amiga-ese for "*" */
    c = WILDCHR_MULTI, p++;
#endif /* AMIGA */
  if (!no_wild && c == WILDCHR_MULTI)
  {
    if (wild_stop_at_dir) {
      /* Check for an immediately following WILDCHR_MULTI */
# ifdef AMIGA
      if ((c = p[0]) == '#' && p[1] == '?') /* "#?" is Amiga-ese for "*" */
        c = WILDCHR_MULTI, p++;
      if (c != WILDCHR_MULTI) {
# else /* !AMIGA */
      if (*p != WILDCHR_MULTI) {
# endif /* ?AMIGA */
        /* Single WILDCHR_MULTI ('*'): this doesn't match slashes */
        for (; *s && *s != DIRSEP_CHR; INCSTR(s))
          if ((c = recmatch(p, s, cs)) != 0)
            return c;
        /* end of pattern: matched if at end of string, else continue */
        if (*p == 0)
          return (*s == 0);
        /* continue to match if at DIRSEP_CHR in pattern, else give up */
        return (*p == DIRSEP_CHR || (*p == '\\' && p[1] == DIRSEP_CHR))
               ? recmatch(p, s, cs) : 2;
      }
      /* Two consecutive WILDCHR_MULTI ("**"): this matches DIRSEP_CHR ('/') */
      p++;        /* move p past the second WILDCHR_MULTI */
      /* continue with the normal non-WILD_STOP_AT_DIR code */
    } /* wild_stop_at_dir */

    /* Not wild_stop_at_dir */
    if (*p == 0)
      return 1;
    if (!isshexp((char *)p))
    {
      /* optimization for rest of pattern being a literal string */

      /* optimization to handle patterns like *.txt */
      /* if the first char in the pattern is '*' and there */
      /* are no other shell expression chars, i.e. a literal string */
      /* then just compare the literal string at the end */

      ZCONST char *srest;

      srest = s + (strlen(s) - strlen(p));
      if (srest - s < 0)
        /* remaining literal string from pattern is longer than rest of
           test string, there can't be a match
         */
        return 0;
      else
        /* compare the remaining literal pattern string with the last bytes
           of the test string to check for a match */
#ifdef _MBCS
      {
        ZCONST char *q = s;

        /* MBCS-aware code must not scan backwards into a string from
         * the end.
         * So, we have to move forward by character from our well-known
         * character position s in the test string until we have advanced
         * to the srest position.
         */
        while (q < srest)
          INCSTR(q);
        /* In case the byte *srest is a trailing byte of a multibyte
         * character, we have actually advanced past the position (srest).
         * For this case, the match has failed!
         */
        if (q != srest)
          return 0;
        return ((cs ? strcmp(p, q) : namecmp(p, q)) == 0);
      }
#else /* !_MBCS */
        return ((cs ? strcmp(p, srest) : namecmp(p, srest)) == 0);
#endif /* ?_MBCS */
    }
    else
    {
      /* pattern contains more wildcards, continue with recursion... */
      for (; *s; INCSTR(s))
        if ((c = recmatch(p, s, cs)) != 0)
          return c;
      return 2;           /* 2 means give up--shmatch will return false */
    }
  }

#ifndef VMS             /* No bracket matching in VMS */
  /* Parse and process the list of characters and ranges in brackets */
  if (!no_wild && allow_regex && c == '[')
  {
    int e;              /* flag true if next char to be taken literally */
    ZCONST char *q;     /* pointer to end of [-] group */
    int r;              /* flag true to match anything but the range */

    if (*s == 0)                        /* need a character to match */
      return 0;
    p += (r = (*p == '!' || *p == '^')); /* see if reverse */
    for (q = p, e = 0; *q; q++)         /* find closing bracket */
      if (e)
        e = 0;
      else
        if (*q == '\\')
          e = 1;
        else if (*q == ']')
          break;
    if (*q != ']')                      /* nothing matches if bad syntax */
      return 0;
    for (c = 0, e = *p == '-'; p < q; p++)      /* go through the list */
    {
      if (e == 0 && *p == '\\')         /* set escape flag if \ */
        e = 1;
      else if (e == 0 && *p == '-')     /* set start of range if - */
        c = *(p-1);
      else
      {
        uch cc = (cs ? (uch)*s : case_map((uch)*s));
        uch uc = (uch) c;
        if (*(p+1) != '-')
          for (uc = uc ? uc : (uch)*p; uc <= (uch)*p; uc++)
            /* compare range */
            if ((cs ? uc : case_map(uc)) == cc)
              return r ? 0 : recmatch(q + CLEN(q), s + CLEN(s), cs);
        c = e = 0;                      /* clear range, escape flags */
      }
    }
    return r ? recmatch(q + CLEN(q), s + CLEN(s), cs) : 0;
                                        /* bracket match failed */
  }
#endif /* !VMS */

  /* If escape ('\'), just compare next character */
  if (!no_wild && c == '\\')
    if ((c = *p++) == '\0')             /* if \ at end, then syntax error */
      return 0;

#ifdef VMS
  /* 2005-11-06 SMS.
     Handle "..." wildcard in p with "." or "]" in s.
  */
  if ((c == '.') && (*p == '.') && (*(p+ CLEN( p)) == '.') &&
   ((*s == '.') || (*s == ']')))
  {
    /* Match "...]" with "]".  Continue after "]" in both. */
    if ((*(p+ 2* CLEN( p)) == ']') && (*s == ']'))
      return recmatch( (p+ 3* CLEN( p)), (s+ CLEN( s)), cs);

    /* Else, look for a reduced match in s, until "]" in or end of s. */
    for (; *s && (*s != ']'); INCSTR(s))
      if (*s == '.')
        /* If reduced match, then continue after "..." in p, "." in s. */
        if ((c = recmatch( (p+ CLEN( p)), s, cs)) != 0)
          return (int)c;

    /* Match "...]" with "]".  Continue after "]" in both. */
    if ((*(p+ 2* CLEN( p)) == ']') && (*s == ']'))
      return recmatch( (p+ 3* CLEN( p)), (s+ CLEN( s)), cs);

    /* No reduced match.  Quit. */
    return 2;
  }

#endif /* def VMS */

  /* Just a character--compare it */
  return (cs ? c == *s : case_map((uch)c) == case_map((uch)*s)) ?
          recmatch(p, s + CLEN(s), cs) : 0;
}




/*************************************************************************************************/
static char *isshexp(p)
ZCONST char *p;
/* If p is a sh expression, a pointer to the first special character is
   returned.  Otherwise, NULL is returned. */
{
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p+1))
            p++;
        else if (*p == WILDCHAR || *p == '*' || *p == BEG_RANGE)
            return (char *)p;
    return NULL;
} /* end function isshexp() */



static int namecmp(s1, s2)
    ZCONST char *s1, *s2;
{
    int d;

    for (;;) {
        d = (int)ToLower((uch)*s1)
          - (int)ToLower((uch)*s2);

        if (d || *s1 == 0 || *s2 == 0)
            return d;

        s1++;
        s2++;
    }
} /* end function namecmp() */

#endif /* !THEOS */




int iswild(p)        /* originally only used for stat()-bug workaround in */
    ZCONST char *p;  /*  VAX C, Turbo/Borland C, Watcom C, Atari MiNT libs; */
{                    /*  now used in process_zipfiles() as well */
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p+1))
            ++p;
#ifdef THEOS
        else if (*p == '?' || *p == '*' || *p=='#'|| *p == '@')
#else /* !THEOS */
#ifdef VMS
        else if (*p == '%' || *p == '*')
#else /* !VMS */
#ifdef AMIGA
        else if (*p == '?' || *p == '*' || (*p=='#' && p[1]=='?') || *p == '[')
#else /* !AMIGA */
        else if (*p == '?' || *p == '*' || *p == '[')
#endif /* ?AMIGA */
#endif /* ?VMS */
#endif /* ?THEOS */
#ifdef QDOS
            return (int)p;
#else
            return TRUE;
#endif

    return FALSE;

} /* end function iswild() */





#ifdef TEST_MATCH

#define put(s) {fputs(s,stdout); fflush(stdout);}
#ifdef main
#  undef main
#endif

int main(int argc, char **argv)
{
    char pat[256], str[256];

    for (;;) {
        put("Pattern (return to exit): ");
        gets(pat);
        if (!pat[0])
            break;
        for (;;) {
            put("String (return for new pattern): ");
            gets(str);
            if (!str[0])
                break;
            printf("Case sensitive: %s  insensitive: %s\n",
              match(str, pat, 0) ? "YES" : "NO",
              match(str, pat, 1) ? "YES" : "NO");
        }
    }
    EXIT(0);
}

#endif /* TEST_MATCH */
