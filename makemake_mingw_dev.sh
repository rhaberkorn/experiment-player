#!/bin/sh

autoreconf -i
./configure --prefix=/mingw \
	    --enable-console \
	    --enable-doxygen-doc \
	    --enable-doxygen-extract-private \
	    CFLAGS="-g -O0"
