
if USE_DOXYGEN
noinst_DATA = doxygen/
endif

doxygen/ : Doxyfile
	@DOXYGEN@ $<

clean-local:
	-rm -rf doxygen/
