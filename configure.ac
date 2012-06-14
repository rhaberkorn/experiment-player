#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([Experiment Player], [dev],
	[robin.haberkorn@st.ovgu.de],
	[experiment-player],
	[https://github.com/rhaberkorn/experiment-player])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Experiment player data directory
player_datadir=$datarootdir/$PACKAGE_TARNAME
AC_SUBST(player_datadir)

# GtkVlcPlayer data directory
gtk_vlc_player_datadir=$datarootdir/gtk-vlc-player
AC_SUBST(gtk_vlc_player_datadir)

#
# Checks for programs.
#
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CC_C99
if [[ $ac_cv_prog_cc_c99 = no ]]; then
	AC_MSG_ERROR([C compiler does not support C99 mode!])
fi
AM_PROG_CC_C_O

AC_CHECK_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)
if [[ x$GLIB_GENMARSHAL = x ]]; then
	AC_MSG_ERROR([glib-genmarshal not found!])
fi

# not necessarily required
AC_CHECK_PROG(GTESTER, gtester, gtester)
AC_CHECK_PROG(GTESTER_REPORT, gtester-report, gtester-report)
AM_CONDITIONAL(USE_GTESTER, [test x$GTESTER != x -a x$GTESTER_REPORT != x])
AM_COND_IF(USE_GTESTER, , [
	AC_MSG_WARN([gtester not found! Cannot automatically run test suites.])
])

# not necessarily required (depends on options)
AC_CHECK_PROG(DOXYGEN, doxygen, doxygen)
AC_CHECK_PROG(XSLTPROC, xsltproc, xsltproc)
XSLT_FLAGS="--xinclude"
AC_SUBST(XSLT_FLAGS)

case $host in
*-*-mingw*)
	AC_CHECK_PROG(RC, windres, windres)
	if [[ x$RC = x ]]; then
		AC_MSG_WARN([Resource compiler not found... Binary will not have an icon!])
	fi
esac
AM_CONDITIONAL(USE_W32_RES, [test x$RC != x])

#
# Checks for libraries.
#
PKG_CHECK_MODULES(LIBGLIB, [gobject-2.0 glib-2.0])

PKG_CHECK_MODULES(LIBGTK, [gtk+-2.0])

PKG_CHECK_EXISTS([gladeui-1.0],
		 [glade3_catalogsdir=`$PKG_CONFIG --variable=catalogdir gladeui-1.0`])
if [[ x$glade3_catalogsdir = x ]]; then
	AC_MSG_WARN([Glade-3 catalog directory not found, installing Gtk+ widget catalogs into custom directories!])
fi
# TODO: with-option to overwrite the catalogsdir
gtk_experiment_widgets_catalogsdir=${glade3_catalogsdir:-$player_datadir/catalogs}
AC_SUBST(gtk_experiment_widgets_catalogsdir)
gtk_vlc_player_catalogsdir=${glade3_catalogsdir:-$gtk_vlc_player_datadir/catalogs}
AC_SUBST(gtk_vlc_player_catalogsdir)

# FIXME: further restrict libvlc version
PKG_CHECK_MODULES(LIBVLC, [libvlc >= 1.1.13 vlc-plugin])

PKG_CHECK_MODULES(LIBXML2, [libxml-2.0], , [
	AC_CHECK_PROG(XML2_CONFIG, xml2-config, xml2-config)
	if [[ x$XML2_CONFIG != x ]]; then
		LIBXML2_CFLAGS="`$XML2_CONFIG --cflags`"
		AC_SUBST(LIBXML2_CFLAGS)
		LIBXML2_LIBS="`$XML2_CONFIG --libs`"
		AC_SUBST(LIBXML2_LIBS)
	else
		temp_CFLAGS="$CFLAGS"
		temp_LIBS="$LIBS"
		CFLAGS=
		LIBS=

		AC_CHECK_LIB(xml2, xmlParseFile, , [
			AC_MSG_ERROR([Required libxml-2.0 package or library missing!])
		])

		AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h \
				  libxml/xpath.h libxml/xpathInternals.h], , [
			AC_MSG_ERROR([Required libxml headers are missing!])
		])

		LIBXML2_CFLAGS="$CFLAGS"
		AC_SUBST(LIBXML2_CFLAGS)
		LIBXML2_LIBS="$LIBS"
		AC_SUBST(LIBXML2_LIBS)

		CFLAGS="$temp_CFLAGS"
		LIBS="$temp_LIBS"
	fi
])

#
# Checks for header files.
#
AC_HEADER_STDC

case $host in
*-*-linux*)
	AC_CHECK_HEADERS([X11/Xlib.h], , [
		AC_MSG_ERROR([Missing X11/Xlib.h!])
	])
	;;
*-*-mingw*)
	AC_CHECK_HEADERS([windows.h], , [
		AC_MSG_ERROR([Missing Windows headers!])
	])
	AC_CHECK_HEADERS([shellapi.h], , [
		AC_MSG_ERROR([Missing Windows headers!])
	], [
		#include <windows.h>
	])
	;;
esac

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

#
# Config options
#
AC_ARG_WITH(default-ui,
	AS_HELP_STRING([--with-default-ui=PATH],
		       [Specify default UI definition to use [default=DATAROOTDIR/experiment-player/default.ui]]),
	[DEFAULT_UI=$withval], [DEFAULT_UI=$player_datadir/default.ui])
# NOTE: cannot use AC_DEFINE_UNQUOTED since installation directories
# must be expanded by the Makefiles
AC_SUBST(DEFAULT_UI)

AC_ARG_WITH(help-uri,
	AS_HELP_STRING([--with-help-uri=URI],
		       [Specify help (manual) URI [default=file://DOCDIR/experiment-player.html]]),
	[HELP_URI=$withval], [HELP_URI=file://$docdir/experiment-player.html])
# NOTE: cannot use AC_DEFINE_UNQUOTED since installation directories
# must be expanded by the Makefiles
AC_SUBST(HELP_URI)

AC_ARG_ENABLE(doxygen-doc,
	AS_HELP_STRING([--enable-doxygen-doc],
		       [Generate Doxygen documentation [default=no]]),
	[doxygen_doc=$enableval], [doxygen_doc=no])
if [[ $doxygen_doc = yes -a x$DOXYGEN = x ]]; then
        AC_MSG_ERROR([Enabled generating Doxygen documentation, but Doxygen not found! Try --disable-doxygen-doc.])
fi
AM_CONDITIONAL(BUILD_DOXYGEN, [test $doxygen_doc = yes])

AC_ARG_ENABLE(doxygen-extract-private,
	AS_HELP_STRING([--enable-doxygen-extract-private],
		       [Include private and static members in Doxygen documentation [default=no]]),
[
	case $enableval in
	yes)	DOXYGEN_EXTRACT_PRIVATE=YES ;;
	*)	DOXYGEN_EXTRACT_PRIVATE=NO ;;
	esac
], [
	DOXYGEN_EXTRACT_PRIVATE=NO
])
AC_SUBST(DOXYGEN_EXTRACT_PRIVATE)

AC_ARG_ENABLE(html-doc,
	AS_HELP_STRING([--enable-html-doc],
		       [Generate HTML documentation [default=yes]]),
	[html_doc=$enableval], [html_doc=yes])
if [[ $html_doc = yes -a x$XSLTPROC = x ]]; then
        AC_MSG_ERROR([Enabled generating HTML documentation, but XSLTProc not found! Try --disable-html-doc.])
fi
AM_CONDITIONAL(BUILD_HTML, [test $html_doc = yes])

AC_ARG_ENABLE(console,
	AS_HELP_STRING([--enable-console],
		       [Enable console-mode binaries [default=no]]),
	[console=$enableval], [console=no])
if [[ $console = no ]]; then
	case $host in
	*-*-mingw*) GTKAPP_LDFLAGS="$GTKAPP_LDFLAGS -mwindows" ;;
	esac
fi

AC_SUBST(GTKAPP_CFLAGS)
AC_SUBST(GTKAPP_LDFLAGS)

# Some constants
AC_DEFINE_UNQUOTED(CONFIG_KEY_FILE, [".$PACKAGE_TARNAME"], [Configuration filename (relative)])

AC_DEFINE(GTK_VLC_PLAYER_TIME_ADJ_STEP,	[1000.],	[VLC Player time adjustment step increment])
AC_DEFINE(GTK_VLC_PLAYER_TIME_ADJ_PAGE,	[30000.],	[VLC Player time adjustment page increment])

AC_DEFINE(GTK_VLC_PLAYER_VOL_ADJ_STEP,	[0.02],		[VLC Player volume adjustment step increment])
AC_DEFINE(GTK_VLC_PLAYER_VOL_ADJ_PAGE,	[0.],		[VLC Player volume adjustment page increment])

AC_DEFINE(GTK_EXPERIMENT_TRANSCRIPT_BACKDROP, [16],	[Experiment Transcript backdrop area color change (percent)])

AC_DEFINE(DEFAULT_QUICKOPEN_DIR,	["."],		[Default directory for listing experiments])
AC_DEFINE(EXPERIMENT_MOVIE_FILTER,	["*.mp4;*.avi"], [Filters for (quick) opening movies])
AC_DEFINE(EXPERIMENT_TRANSCRIPT_EXT,	["xml"],	[File extension of experiment transcripts])

AC_DEFINE(DEFAULT_FORMATS_DIR,		["."],		[Default directory for selecting formats])
AC_DEFINE(EXPERIMENT_FORMATS_FILTER,	["*.fmt"],	[Format file filter])

AC_DEFINE(DEFAULT_INTERACTIVE_FORMAT_FONT,	["bold"],	[Default interactive format font description])
### AC_DEFINE(DEFAULT_INTERACTIVE_FORMAT_FGCOLOR,	["white"],	[Default interactive format foreground color])
### AC_DEFINE(DEFAULT_INTERACTIVE_FORMAT_BGCOLOR,	["red"],	[Default interactive format background color])

AC_CONFIG_FILES([Makefile lib/Makefile src/Makefile])
AC_CONFIG_FILES([lib/gtk-vlc-player/Makefile])
AC_CONFIG_FILES([lib/experiment-reader/Makefile lib/experiment-reader/tests/Makefile])
AC_CONFIG_FILES([lib/gtk-experiment-widgets/Makefile])
AC_CONFIG_FILES([doc/Makefile doc/Doxyfile])

AC_OUTPUT
