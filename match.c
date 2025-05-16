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
  characters allowed in the [..] pattern construct.  Other characters are
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

#define UNZIP_INTERNAL
#include "unzip.h"

#define BEG_RANGE  '['
#define END_RANGE  ']'
#define WILDCHR_SINGLE '?'
#define WILDCHR_MULTI '*'


/* WARNING: this only works for ASCII or supersets of ASCII (e.g. UTF-8),
   and only uppercases ASCII letters. toupper() would be better but slower. */
#define to_up(c)    ((c) >= 'a' && (c) <= 'z' ? (c)-'a'+'A' : (c))


static int recmatch(const char *, const char *, int, int);
static char *isshexp(const char *p);
static int namecmp(const char *s1, const char *s2);


/* match() is a shell to recmatch() to return only Boolean values. */

int match(string, pattern, ignore_case, sepc)
    const char *string, *pattern;
    int ignore_case;
    int sepc;
{
    return recmatch(pattern, string, ignore_case, sepc) == 1;
}


static int recmatch(p, s, ci, sepc)
const char *p;          /* sh pattern to match */
const char *s;          /* string to match it to */
int ci;                 /* flag: force case-insensitive matching */
int sepc;               /* directory sepchar for WildStopAtDir mode, or 0 */
/* Recursively compare the sh pattern p with the string s and return 1 if
   they match, and 0 or 2 if they don't or if there is a syntax error in the
   pattern.  This routine recurses on itself no deeper than the number of
   characters in the pattern. */
{
  int c;                /* pattern char or start of range in [-] loop */

  /* Get first character, the pattern for new recmatch calls follows */
  c = *POSTINCSTR(p);

  /* If that was the end of the pattern, match if string empty too */
  if (c == 0)
    return *s == 0;

  /* '?' (or '%' or '#') matches any character (but not an empty string) */
  if (c == WILDCHR_SINGLE) {
    return (*s && *s != sepc) ? recmatch(p, s + CLEN(s), ci, sepc) : 0;
  }

  /* WILDCHR_MULTI ('*') matches any number of characters, including zero */
  if (c == WILDCHR_MULTI)
  {
    if (sepc) {
      /* Check for an immediately following WILDCHR_MULTI */
      if (*p != WILDCHR_MULTI) {
        /* Single WILDCHR_MULTI ('*'): this doesn't match slashes */
        for (; *s && *s != sepc; INCSTR(s))
          if ((c = recmatch(p, s, ci, sepc)) != 0)
            return c;
        /* end of pattern: matched if at end of string, else continue */
        if (*p == 0)
          return (*s == 0);
        /* continue to match if at sepc in pattern, else give up */
        return (*p == sepc || (*p == '\\' && p[1] == sepc))
                ? recmatch(p, s, ci, sepc) : 2;
      }
      /* Two consecutive WILDCHR_MULTI ("**"): this matches sepc */
      INCSTR(p);         /* move p past the second WILDCHR_MULTI */
    }

    if (*p == 0)
      return 1;
    if (!isshexp((char *)p))
    {
      /* optimization for rest of pattern being a literal string */

      /* optimization to handle patterns like *.txt */
      /* if the first char in the pattern is '*' and there */
      /* are no other shell expression chars, i.e. a literal string */
      /* then just compare the literal string at the end */

      const char *srest;

      srest = s + (strlen(s) - strlen(p));
      if (srest - s < 0)
        /* remaining literal string from pattern is longer than rest of
           test string, there can't be a match
         */
        return 0;
      else
        /* compare the remaining literal pattern string with the last bytes
           of the test string to check for a match */
        return ((!ci ? strcmp(p, srest) : namecmp(p, srest)) == 0);
    }
    else
    {
      /* pattern contains more wildcards, continue with recursion... */
      for (; *s; INCSTR(s))
        if ((c = recmatch(p, s, ci, sepc)) != 0)
          return c;
      return 2;           /* 2 means give up--shmatch will return false */
    }
  }

  /* Parse and process the list of characters and ranges in brackets */
  if (c == BEG_RANGE)
  {
    int e;              /* flag true if next char to be taken literally */
    const char *q;      /* pointer to end of [-] group */
    int r;              /* flag true to match anything but the range */

    if (*s == 0)                        /* need a character to match */
      return 0;
    p += (r = (*p == '!' || *p == '^')); /* see if reverse */
    for (q = p, e = 0; *q; INCSTR(q))         /* find closing bracket */
      if (e)
        e = 0;
      else
        if (*q == '\\')
          e = 1;
        else if (*q == END_RANGE)
          break;
    if (*q != END_RANGE)                  /* nothing matches if bad syntax */
      return 0;
    for (c = 0, e = *p == '-'; p < q; INCSTR(p))    /* go through the list */
    {
      if (e == 0 && *p == '\\')         /* set escape flag if \ */
        e = 1;
      else if (e == 0 && *p == '-')     /* set start of range if - */
        c = *(p-1);
      else
      {
        uch cc = (!ci ? (uch)*s : to_up((uch)*s));
        uch uc = (uch) c;
        if (*(p+1) != '-')
          for (uc = uc ? uc : (uch)*p; uc <= (uch)*p; uc++)
            /* compare range */
            if ((!ci ? uc : to_up(uc)) == cc)
              return r ? 0 : recmatch(q + CLEN(q), s + CLEN(s), ci, sepc);
        c = e = 0;                      /* clear range, escape flags */
      }
    }
    return r ? recmatch(q + CLEN(q), s + CLEN(s), ci, sepc) : 0;
                                        /* bracket match failed */
  }

  /* If escape ('\'), just compare next character */
  if (c == '\\')
    if ((c = *p++) == '\0')             /* if \ at end, then syntax error */
      return 0;


  /* Just a character--compare it */
  return (!ci ? c == *s : to_up((uch)c) == to_up((uch)*s)) ?
          recmatch(p, s + CLEN(s), ci, sepc) : 0;
}




/*************************************************************************************************/
static char *isshexp(p)
const char *p;
/* If p is a sh expression, a pointer to the first special character is
   returned.  Otherwise, NULL is returned. */
{
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p+1))
            p++;
        else if (*p == WILDCHR_SINGLE || *p == WILDCHR_MULTI || *p == BEG_RANGE)
            return (char *)p;
    return NULL;
} /* end function isshexp() */



static int namecmp(s1, s2)
    const char *s1, *s2;
{
    int d;

    for (;;) {
        d = (int)tolower((uch)*s1)
          - (int)tolower((uch)*s2);

        if (d || *s1 == 0 || *s2 == 0)
            return d;

        s1++;
        s2++;
    }
} /* end function namecmp() */





int iswild(p)        /* originally only used for stat()-bug workaround in */
    const char *p;   /*  VAX C, Turbo/Borland C, Watcom C, Atari MiNT libs; */
{                    /*  now used in process_zipfiles() as well */
    for (; *p; INCSTR(p))
        if (*p == '\\' && *(p+1))
            ++p;
        else if (*p == '?' || *p == '*' || *p == '[')
            return TRUE;

    return FALSE;

} /* end function iswild() */





#ifdef TEST_MATCH

#  include <stdio.h>

#  define put(s) {fputs(s,stdout); fflush(stdout);}
#  ifdef main
#    undef main
#  endif

int main(int argc, char **argv)
{
    char pat[256], str[256];

    for (;;) {
        put("Pattern (return to exit): ");
        fgets(pat, sizeof(pat), stdin);
        if (!pat[0])
            break;
        for (;;) {
            put("String (return for new pattern): ");
            fgets(str, sizeof(str), stdin);
            if (!str[0])
                break;
            printf(
                "Match forward slash:  case sensitive: %3s  insensitive (-C): %3s\n"
                "Don't match (-W):     case sensitive: %3s  insensitive (-C): %3s\n",
                match(str, pat, 0, 0) ? "YES" : "NO ",
                match(str, pat, 1, 0) ? "YES" : "NO ",
                match(str, pat, 0, '/') ? "YES" : "NO ",
                match(str, pat, 1, '/') ? "YES" : "NO ");
        }
    }
    EXIT(0);
}

#endif /* TEST_MATCH */
