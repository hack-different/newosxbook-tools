mkdir obj sym
export DSTROOT=/  # Install directly to XCode toolchain:
xcodebuild  install -sdk macosx \
	 -target ctfconvert -target ctfdump -target ctfmerge \
	  ARCHS=x86_64 \
	   SRCROOT=$PWD OBJROOT=$PWD/obj SYMROOT=$PWD/sym  \
	    HEADER_SEARCH_PATHS="$PWD/compat/opensolaris/** $PWD/lib/**" 