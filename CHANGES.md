A nonexhaustive list of things I've changed:

  - External [patches](patches/README.md)
  - Added zipbomb detection (a5b558c)
  - Added simplified Makefile (4d6a7e9)
  - Automated tests (5118111)
  - zipgrep: change egrep to grep -E (f2fe7ce)
    (this prevents printing of deprecation warnings on Arch Linux)
  - Deleted code for obsolete operating systems and compilers.
  - Replaced macros like OF, zvoid, ZCONST, etc. with ANSI C equivalents.
  - Found a bug related to umask() and fixed it! (commit e8a8430)
  - Removed special macros to deal with near and far pointers.
