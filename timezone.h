/*
  Copyright (c) 1990-2001 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
#ifndef __timezone_h
#  define __timezone_h

#  ifndef IZ_MKTIME_ONLY

/* limits for our timezone info data:
 * we support only basic standard and daylight time, with max 2 transitions
 * per year, but for the maximum range of years a 32-bit second counter
 * can cover (these are 136 years plus a bit more than one month)
 */
#    define TZ_MAX_TIMES    272 /* (=2*(LastGoodYr + 1 - FirstGoodYr) */
#    define TZ_MAX_TYPES    2   /* We only support basic standard and daylight */
#    define TZ_MAX_CHARS    50  /* Maximum number of abbreviation characters */

/* supported types of transition rules */
#    define JULIAN_DAY              0   /* Jn - Julian day */
#    define DAY_OF_YEAR             1   /* n - day of year */
#    define MONTH_NTH_DAY_OF_WEEK   2   /* Mm.n.d - month, week, day of week */


struct ttinfo {
    long            tt_gmtoff;  /* UTC offset in seconds */
    int             tt_isdst;   /* used to set tm_isdst */
    int             tt_abbrind; /* abbreviation list index */
};

struct state {
    int             timecnt;
    int             typecnt;
    int             charcnt;
    time_t          ats[TZ_MAX_TIMES];
    unsigned char   types[TZ_MAX_TIMES];
    struct ttinfo   ttis[TZ_MAX_TYPES];
    char            chars[TZ_MAX_CHARS];
};

struct rule {
    int             r_type;     /* type of rule--JULIAN_DAY etc */
    int             r_day;      /* day number of rule */
    int             r_week;     /* week number of rule */
    int             r_mon;      /* month number of rule */
    long            r_time;     /* transition time of rule */
};

extern int real_timezone_is_set;        /* set by tzset() */


/* prototypes of functions not in time.h */

void __tzset(void);

#    ifdef NEED__ISINDST
int _isindst(struct tm *tb);
#    endif

#    ifdef IZTZ_SETLOCALTZINFO
void set_TZ(long time_zone, int day_light);
#    endif

#  endif /* !IZ_MKTIME_ONLY */

time_t mkgmtime(struct tm *tm);

#endif
