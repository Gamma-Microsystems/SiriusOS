#!/bin/bash
# GPLv3 License
# Thanks to cavOS project by malwarepad <https://github.com/malwarepad/cavOS>
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
TRIPLET=elf
KARCH=x86_64

MUSL_RELEASE="musl-1.2.5"

cd "${SCRIPTPATH}"

# --noreplace -> won't re-compile if it finds libc
if test -f "$SCRIPTPATH/cavos-out/lib/libc.a"; then
	if [ "$#" -eq 1 ]; then
		exit 0
	fi
fi

# Ensure we have the sources!
if ! test -f "$SCRIPTPATH/$MUSL_RELEASE/README"; then
	wget -nc "https://musl.libc.org/releases/$MUSL_RELEASE.tar.gz"
	tar xpvf "$MUSL_RELEASE.tar.gz"

	# No patches needed!
	# cd "$MUSL_RELEASE"
	# patch -p1 < xyz
	# cd ../
fi

# Ensure the ELF/SiriusOS toolchain's in PATH
if [[ ":$PATH:" != *":$HOME/srs/cross/bin:"* ]]; then
	export PATH=$HOME/srs/cross/bin:$PATH
fi

# Booo! Scary!
export PREFIX="../base"
mkdir -p sirius-build
cd sirius-build
CC=$KARCH-$TRIPLET-gcc ARCH=$KARCH CROSS_COMPILE=$KARCH-$TRIPLET- "../$MUSL_RELEASE/configure" --target=$KARCH-$TRIPLET --build=$KARCH-$TRIPLET --host=$KARCH-$TRIPLET --prefix="$PREFIX" --syslibdir="/lib" --enable-debug
make clean
make all -j$(nproc)
make install

# Copy libraries (and update headers)
mkdir -p "$SCRIPTPATH/../base/usr/"
cp -r "$PREFIX/lib" "$PREFIX/include" "$SCRIPTPATH/../../../base/usr/"

# libc.so (dynamic deafult) fixup (just use -static man)
# mv "$SCRIPTPATH/../../../target/usr/lib/libc.so" "$SCRIPTPATH/../../../target/usr/lib/libc.1.so"

# crt0 fixup
rm -f "$SCRIPTPATH/../../../base/usr/lib/crt0.o"
cp "$SCRIPTPATH/../../../base/usr/lib/crt1.o" "$SCRIPTPATH/../../../target/usr/lib/crt0.o"

# required for proper dynamic linking
mkdir -p "$SCRIPTPATH/../../../base/lib/"
cp "$SCRIPTPATH/../../../base/usr/lib/libc.so" "$SCRIPTPATH/../../../base/lib/ld64.so.1"
