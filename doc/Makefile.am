
#
# Docbook processing
# TODO: might also generate a man page
#
DB_URI = http://docbook.sourceforge.net/release/xsl/current

if BUILD_HTML
dist_html_DATA = experiment-player.html
endif
CLEANFILES = experiment-player.html html_titlepage.xsl
EXTRA_DIST = experiment-player.xml html_custom.xsl html_titlepage.spec.xml

experiment-player.html : experiment-player.xml html_custom.xsl html_titlepage.xsl
	@XSLTPROC@ @XSLT_FLAGS@ -o $@ html_custom.xsl $<

html_titlepage.xsl : html_titlepage.spec.xml
	@XSLTPROC@ @XSLT_FLAGS@ -o $@ $(DB_URI)/template/titlepage.xsl $<

#
# Doxygen processing (do not install or distribute)
#
if BUILD_DOXYGEN
noinst_DATA = doxygen/
endif

doxygen/ : Doxyfile
	@DOXYGEN@ $<

clean-local:
	-rm -rf doxygen/
