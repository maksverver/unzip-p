A nonexhaustive list of things I've changed:

  - External [patches](patches/README.md)
  - Added zipbomb detection (a5b558c)
  - Simplified Makefile (4d6a7e9)
  - Automated tests (5118111)
  - zipgrep: change egrep to grep -E (f2fe7ce)
    (this prevents printing of deprecation warnings on Arch Linux)
  - Deleted code for obsolete operating systems and compilers.
  - Replaced macros like OF, zvoid, ZCONST, etc. with ANSI C equivalents.
  - Fixed a bug related to umask() (commit e8a8430)
  - Fixed bugs in parsing of UNIX extra fields (commit 4e45a6c)
  - Removed special macros to deal with near and far pointers.
