## Patches applied from Mark Adler's Github

https://github.com/madler/unzip

  - 0b82c20ac7375b522215b567174f370be89a4b12  Remove length integrity check in funzip.
  - 1860ba704db791db940475b1fb6ef73bdb81bcab  If ZIP64_SUPPORT is requested, then request LARGE_FILE_SUPPORT.
  - 5c572555cf5d80309a07c30cf7a54b2501493720  Fix bug in UZinflate() that incorrectly updated G.incnt.
  - d3934d015d0ce99d3588a3c97316e99fd9620ced  Fix code to compile for USE_ZLIB and old zlib version.
  - 5e2efcd633a4a1fb95a129a75508e7d769e767be  Fix bug in UZbunzip2() that incorrectly updated G.incnt

## Patches from Debian's unzip package

https://sources.debian.org/patches/unzip/6.0-29/

APPLIED:

  - 03-include-unistd-for-kfreebsd.patch
  - 04-handle-pkware-verification-bit.patch
  - 05-fix-uid-gid-handling.patch
  - 06-initialize-the-symlink-flag.patch
  - 07-increase-size-of-cfactorstr.patch
  - 08-allow-greater-hostver-values.patch
  - 09-cve-2014-8139-crc-overflow.patch
  - 10-cve-2014-8140-test-compr-eb.patch
  - 11-cve-2014-8141-getzip64data.patch
  - 12-cve-2014-9636-test-compr-eb.patch
  - 13-remove-build-date.patch
  - 14-cve-2015-7696.patch
  - 15-cve-2015-7697.patch
  - 16-fix-integer-underflow-csiz-decrypted.patch
  - 17-restore-unix-timestamps-accurately.patch
  - 18-cve-2014-9913-unzip-buffer-overflow.patch
  - 19-cve-2016-9844-zipinfo-buffer-overflow.patch
  - 20-cve-2018-1000035-unzip-buffer-overflow.patch
  - 21-fix-warning-messages-on-big-files.patch
  - 22-cve-2019-13232-fix-bug-in-undefer-input.patch
  - 25-cve-2019-13232-fix-bug-in-uzbunzip2.patch
  - 27-zipgrep-avoid-test-errors.patch
  - 28-cve-2022-0529-and-cve-2022-0530.patch
  - 29-handle-windows-zip64-files.patch
  - 30-drop-conflicting-declarations.patch
  - 31-fix-zipgrep.patch

NOT APPLIED:

Debian specific:

  - 01-manpages-in-section-1-not-in-section-1l.patch
  - 02-this-is-debian-unzip.patch

Zip bomb patches (will be addressed differently)

  - 23-cve-2019-13232-zip-bomb-with-overlapped-entries.patch
  - 24-cve-2019-13232-do-not-raise-alert-for-misplaced-central-directory.patch

Already applied from Mark Adler's github branch

  - 25-cve-2019-13232-fix-bug-in-uzbunzip2.patch
  - 26-cve-2019-13232-fix-bug-in-uzinflate.patch


## Patches from Fedora's unzip package

https://src.fedoraproject.org/rpms/unzip/tree/rawhide

Applied:

  - unzip-6.0-attribs-overflow.patch
  - unzip-6.0-fix-recmatch.patch
  - unzip-6.0-caseinsensitive.patch
  - unzip-6.0-format-secure.patch
  - unzip-6.0-valgrind.patch
  - unzip-6.0-cve-2018-18384.patch
  - unzip-6.0-close.patch

  - unzip-6.0-alt-iconv-utf8.patch
  - unzip-6.0-alt-iconv-utf8-print.patch
  - unzip-6.0-wcstombs-fortify.patch
  - unzip-6.0-COVSCAN-fix-unterminated-string.patch

Not applied (maybe TODO):

  - unzip-6.0-bzip2-configure.patch
  - unzip-gnu89-build.patch

Not applied:

  - unzip-6.0-exec-shield.patch (Fedora specific)
  - unzip-6.0-configure.patch
  - unzip-6.0-manpage-fix.patch (unimportant)
  - unzip-6.0-symlink.patch (identical to Debian 06-initialize-the-symlink-flag.patch)
  - unzip-6.0-x-option.patch (already covered by Debian 05-fix-uid-gid-handling.patch)
  - unzip-6.0-overflow.patch (already covered by 12-cve-2014-9636-test-compr-eb.patch -- DIFFERENT PATCH)

  - unzip-6.0-cve-2014-8139.patch (covered by Debian 09-cve-2014-8139-crc-overflow.patch -- DIFFERENT PATCH)
  - unzip-6.0-cve-2014-8140.patch (covered by Debian 10-cve-2014-8140-test-compr-eb.patch -- identical patch)
  - unzip-6.0-cve-2014-8141.patch (covered by Debian 11-cve-2014-8141-getzip64data.patch -- identical patch)

  - unzip-6.0-overflow-long-fsize.patch
    (covered by Debian patches:
        07-increase-size-of-cfactorstr.patch -- array size 12 instead of 13
        18-cve-2014-9913-unzip-buffer-overflow.patch -- identical)

  - unzip-6.0-heap-overflow-infloop.patch (identical to Debian patch 16-fix-integer-underflow-csiz-decrypted.patch)
  - 0001-Fix-CVE-2016-9844-rhbz-1404283.patch (identical to Debian   19-cve-2016-9844-zipinfo-buffer-overflow.patch)
  - unzip-6.0-timestamp.patch (identical to Debian 17-restore-unix-timestamps-accurately.patch)

  - unzip-6.0-cve-2018-1000035-heap-based-overflow.patch (covered by Debian 20-cve-2018-1000035-unzip-buffer-overflow.patch -- DIFFERENT PATCH)


Zip bomb patches (will be addressed differently):

  - unzip-zipbomb-part1.patch
  - unzip-zipbomb-part2.patch
  - unzip-zipbomb-part3.patch
  - unzip-zipbomb-manpage.patch
  - unzip-zipbomb-part4.patch
  - unzip-zipbomb-part5.patch
  - unzip-zipbomb-part6.patch
  - unzip-zipbomb-switch.patch (I did port this one)


## Arch Linux patches:

Applied:

  - unzip-6.0_CVE-2021-4217.patch


EOF.
