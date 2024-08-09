#!/bin/bash
VERSION=$(git describe --exact-match --tags)
LAST=$(git describe --abbrev=0 --tags ${VERSION}^)
CHANGELOG=$(git log --pretty=format:%s ${LAST}..HEAD | grep ':' | sed -re 's/([^:]*)\:/- \`\1\`\:/' | sort)
cat <<NOTES
# SiriusOS ${VERSION}

Put a screenshot here.

## What's New in ${VERSION}?

Describe the release here.

## What is SiriusOS?

SiriusOS is a hobbyist, operating system for x86-64 and aarch64 PCs, focused primarily on desktop and embedded systems. It provides a Unix-like environment, complete with a graphical desktop interface, shared libraries, feature-rich terminal emulator, and support for running, ~~GCC,~~ Quake, DOOM and several other ports. The core of SiriusOS, provided by the CD images in this release, is built completely from scratch. The bootloader, kernel, drivers, C standard library, and userspace applications are all original software created by the authors, as are the graphical assets.

## Who wrote SiriusOS?

SiriusOS is primarily written by a single maintainer, with several contributions from others. A complete list of contributors is available from [AUTHORS](AUTHORS).

## Release Files

\`image.iso\` is the standard build of SiriusOS, built by ~~the Github Actions CI workflow~~ my laptop :cold_face:. It uses SiriusOS's native bootloaders and should work in most virtual machines using BIOS.

## Changelog
${CHANGELOG}

## Known Issues
- The SMP scheduler is known to have performance issues.
- Several utilities, libc functions, and hardware drivers are missing functionality.
- There are many known security issues with SiriusOS. NOTE THAT SIRIUS OS IS AN PRE-ALPHA SOFTWARE IS NOT MEAN TO BE USED ON PRODUCTION. If you find security issues in SiriusOS and would like to responsibly report them, please file a regular issue report here on GitHub.
NOTES
