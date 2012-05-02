#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([Experiment Player], [dev], [robin.haberkorn@st.ovgu.de],
	[experiment-player])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

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
AC_PROG_CC_C_O

AC_CHECK_PROG(DOXYGEN, doxygen, doxygen)
AC_CHECK_PROG(XSLTPROC, xsltproc, xsltproc)
XSLT_FLAGS="--xinclude"
AC_SUBST(XSLT_FLAGS)

#
# Checks for libraries.
#
PKG_CHECK_MODULES(LIBGTK, [gtk+-2.0])
CFLAGS="$CFLAGS $LIBGTK_CFLAGS"
LIBS="$LIBS $LIBGTK_LIBS"

# FIXME: further restrict libvlc version
PKG_CHECK_MODULES(LIBVLC, [libvlc >= 1.1.13 vlc-plugin])
CFLAGS="$CFLAGS $LIBVLC_CFLAGS"
LIBS="$LIBS $LIBVLC_LIBS"

PKG_CHECK_MODULES(LIBXML2, [libxml-2.0], [
	CFLAGS="$CFLAGS $LIBXML2_CFLAGS"
	LIBS="$LIBS $LIBXML2_LIBS"
], [
	AC_CHECK_PROG(XML2_CONFIG, xml2-config, xml2-config)
	if [[ x$XML2_CONFIG != x ]]; then
		CFLAGS="$CFLAGS `$XML2_CONFIG --cflags`"
		LIBS="$LIBS `$XML2_CONFIG --libs`"
	else
		AC_CHECK_LIB(xml2, xmlParseFile, , [
			AC_MSG_ERROR([Required libxml-2.0 package or library missing!])
		])

		AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h \
				  libxml/xpath.h libxml/xpathInternals.h], , [
			AC_MSG_ERROR([Required libxml headers are missing!])
		])
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
AC_ARG_ENABLE(doxygen-doc,
	AS_HELP_STRING([--enable-doxygen-doc],
		       [Generate Doxygen documentation [default=no]]),
	[doxygen_doc=$enableval], [doxygen_doc=no])
if [[ $doxygen_doc = yes -a x$DOXYGEN = x ]]; then
        AC_MSG_ERROR([Enabled generating Doxygen documentation, but Doxygen not found! Try --disable-doxygen-doc.])
fi
AM_CONDITIONAL(BUILD_DOXYGEN, test $doxygen_doc = yes)

AC_ARG_ENABLE(html-doc,
	AS_HELP_STRING([--enable-html-doc],
		       [Generate HTML documentation [default=yes]]),
	[html_doc=$enableval], [html_doc=yes])
if [[ $html_doc = yes -a x$XSLTPROC = x ]]; then
        AC_MSG_ERROR([Enabled generating HTML documentation, but XSLTProc not found! Try --disable-html-doc.])
fi
AM_CONDITIONAL(BUILD_HTML, test $html_doc = yes)

# FIXME
AC_DEFINE(USE_BUILDER, , [foo])
AC_DEFINE(TRANSCRIPT_JUSTIFY, 1, [foo])

# Export symbols
# necessary for auto-registering GTK+ signal handlers by GTK+ builder
case $host in
*-*-linux*)	LIBS="$LIBS -Wl,--export-dynamic" ;;
*-*-windows*)	LIBS="$LIBS -Wl,--export-all-symbols" ;;
esac

AC_CONFIG_FILES([Makefile \
		 src/Makefile \
		 doc/Makefile doc/Doxyfile])
AC_OUTPUT
