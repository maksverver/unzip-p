# POSIX-only fork of Info-ZIP UnZip 6.0 with bug fixes and security patches

This repository contains a fork of
[Info-ZIP's UnZip](https://infozip.sourceforge.net/UnZip.html).
It is based on the latest official release (UnZip 6.0) but
with improvements to make the code more robust, secure and maintainable.

See [patches/](patches/) for a list of security patches I applied, and [CHANGES.md](CHANGES.md) for a high-level description of other changes. Detailed changes area available in the git repository.

This package will install the following five tools: [`funzip`](funzip.txt), [`unzip`](unzip.txt), [`unzipsfx`](unzipsfx.txt), [`zipgrep`](zipgrep.txt), and [`zipinfo`](zipinfo.txt).

Note that the packaging tools from [Info-Zip's zip](https://infozip.sourceforge.net/Zip.html) (`zip`, `zipcloak`, `zipnote` and `zipsplit`) are not included.


## Building, testing, and installing.

```
% make all          # add -j for parallel builds
% make check
% make install
```

To create a package, the `prefix` and `DESTDIR` variables are useful.
Consult the comments in [Makefile](Makefile) for details.


## Motivation

Info-ZIP's `unzip` is a popular tool on Linux and other POSIX systems (FreeBSD, OS X, etc) for handling ZIP files, which, despite being ancient technology at this point, are still quite widely used.

Unfortunately, development of the official Info-ZIP `zip` and `unzip` tools has stalled around 2009, and new releases are unlikely to happen. In the meantime, a number of bugs and security vulnerabilities have been identified. This has required Linux distributions to apply an ever-growing list of patches, which creates an unreasonable burden on packagers.

The goal of this fork is to create a secure code base that includes all important bug fixes and security patches, that is easy to compile and package, and that works on all modern POSIX systems. This allows packagers to simply package my fork instead of the ancient UnZip 6.0 release with a slew of patches on top.

To simplify maintenance, I removed code that is only used to support archaic or esoteric platforms. The guiding philosophy is that code that I cannot easily test must be removed.


## Advantages for packagers

Packagers should consider installing my fork over using the official `unzip60.zip` release for the following reasons:

  * I have included patches for all major bugs and security vulnerabilities. No need to maintain huge patch sets!

  * I will adress new security vulnerabilities if/when they are discovered. Simply watch my repository for new releases.

  * I added a test suite, which, while far from comprehensive, provides basic quality assurance while packaging.


## Random notes

### UnZip beta 6.10b

This repository is forked from UnZip 6.0, the last official release, but there is also an unreleased beta version of UnZip 6.1:
https://sourceforge.net/projects/infozip/files/unreleased%20Betas/UnZip%20betas/

I looked at the changes from unzip60.zip and unzip610b.zip, and a rough summary is as follows:

  - changes for platforms that I don't care about (mostly Win32)
  - major overhaul of command line processing
  - more convoluted implementation of the isprint() fix
  - signature comparisons changed from memcmp() to strncmp(), though reason is unclear, since signatures contain no NUL bytes so these should be equivalent here.
  - changed `fgets(G.answerbuf, sizeof(G.answerbuf), stdin)` to `fgets(G.answerbuf, sizeof(G.answerbuf) - 1, stdin)`; this is more consistent with other places where `fgets(G.answerbuf, 9, stdin)` is used, but it's not clear why this is necessary, since this buffer is never appended to and fgets() already accounts for the terminating NUL byte (i.e., when passed 9 as size, it will read at most 8 characters and always zero-terminates the buffer).

I concluded that none of these need to patched into my fork, because either I don't care about them (e.g., Windows support, new command line parsing) or I already fixed them in a different way (e.g., `fnfilter()`).

### 32-bit timestamps

The ZIP file format stores UNIX timestamps in 32-bit format, though it's not clear if that's a signed or unsigned value. More work is necessary to determine if the tool will work correctly after the year 2038. I'll revisit the topic around the year 2035, if anyone still cares by then.
