A nonexhaustive list of things I've changed:

  - External [patches](patches/README.md)
  - Added zipbomb detection (a5b558c)
  - Simplified Makefile (4d6a7e9)
  - Automated tests (5118111)
  - zipgrep: change egrep to grep -E (f2fe7ce)
  - Deleted code for obsolete operating systems and compilers.
  - Replaced macros like OF, zvoid, ZCONST, etc. with ANSI C equivalents.
  - Fixed bug: umask() was not called due to type (commit e8a8430)
  - Fixed bug: buffer overflow when parsing UNIX extra fields (commit 4e45a6c)
  - Removed special macros to deal with near and far pointers.
  - Check return value of `fgets()` for robustness (commit e30ce3a)
  - Make fnfilter() work correctly on all locales (commit 681ff5c)
  - Fixed bug: dangling pointer freed on overflow (commit 0d5ae35)
