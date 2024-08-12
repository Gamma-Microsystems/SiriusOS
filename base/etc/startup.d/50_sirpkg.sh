#!/bin/nesh

if kcmdline -q no-startup-sirpkg then exit 0

echo -n "Checking for package updates..." >> /dev/pex/splash
sirpkg update &
