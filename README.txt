Building ivm64-gcc from sources
===============================

    export GCC_SRCDIR=/path/to/gcc-8.3.0-ivm64/
    export GCC_INSTALLDIR=/path/to/installdir/gcc-8.3.0-ivm64/
    export GCC_PKGVERSION=$(grep "#define\s\+IVM64_GCC_VERSION" $GCC_SRCDIR/gcc/config/ivm64/ivm64.h|gawk '{print $3}' | tr -d '"')


Prerequisites
-------------

    EITHER install the required packages described in
           $GCC_SRCDIR/INSTALL/prerequisites.html for your system,

    OR build them from sources:
        cd "$GCC_SRCDIR"
        ./contrib/download_prerequisites

Configuration
-------------

    cd "$GCC_SRCDIR"
    mkdir -p build
    cd build

    CFLAGS="-O2 -g" \
    CXXFLAGS="-O2 -g" \
    CFLAGS_FOR_TARGET="-O2" \
    CXXFLAGS_FOR_TARGET="-O2" \
    RANLIB_FOR_TARGET=/usr/bin/ranlib  \
    AR_FOR_TARGET=/usr/bin/ar \
    AS_FOR_TARGET=${GCC_SRCDIR}/gcc/config/ivm64/as \
    LD_FOR_TARGET=${GCC_SRCDIR}/gcc/config/ivm64/ld \
    ../configure \
    --target=ivm64 \
    --prefix="${GCC_INSTALLDIR}" \
    --with-pkgversion="ivm64-gcc ${GCC_PKGVERSION}" \
    --enable-languages=c \
    --disable-threads --disable-tls --disable-libgomp \
    --disable-bootstrap --disable-nls --disable-shared \
    --disable-multilib --disable-decimal-float \
    --disable-libmudflap --disable-libssp --disable-libquadmath \
    --disable-target-libiberty --disable-target-zlib --disable-libmpx \
    --disable-libatomic --disable-gcov --disable-plugin \
    --without-headers \
    --enable-libgcc \
    --with-newlib \
    --with-libgloss \
    --enable-newlib-mb \
    --disable-newlib-multithread

Compile all
-------------------
    make
    make -j4 # in parallel

Compile only gcc without libraries
----------------------------------
    make all-gcc

Install
-------
    make install     # gcc and libraries
    make install-gcc # gcc only

Cleaning
--------
    make distclean # clean distribution, reconfiguration needed
    make clean     # clean all
    make clean-target-libgcc   # clean only libgcc
    make clean-target-newlib   # clean only newlib (libc)
    make clean-target-libgloss # clean only libgloss


