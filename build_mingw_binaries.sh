#!/bin/sh

TEMP_TREE=$(mktemp -d)

TEMP_ZIP=$(mktemp -d)
mkdir $TEMP_ZIP/doc $TEMP_ZIP/ui

ZIP_FILE=$(pwd)/experiment-player-win32.zip
rm -f $ZIP_FILE

autoreconf -i
./configure --prefix=/usr \
	    --with-default-ui=ui/default.ui \
	    --with-help-uri=doc/experiment-player.html \
	    CFLAGS="-g0 -O3"

make clean
make install DESTDIR=$TEMP_TREE

# copy experiment-player files
cp $TEMP_TREE/usr/bin/* $TEMP_ZIP
cp -r $TEMP_TREE/usr/share/doc/experiment-player/* $TEMP_ZIP/doc
cp $TEMP_TREE/usr/share/experiment-player/*.ui $TEMP_ZIP/ui
cp $TEMP_TREE/usr/share/libexperiment-reader/session.dtd $TEMP_ZIP

# copy required DLLs (except GTK+)
cp /mingw/bin/libvlc*.dll $TEMP_ZIP
cp -r /mingw/lib/vlc/plugins $TEMP_ZIP
cp /mingw/bin/libxml2.dll $TEMP_ZIP

# zip!
(cd $TEMP_ZIP; zip -r $ZIP_FILE *)

# clean up
rm -rf $TEMP_TREE $TEMP_ZIP
