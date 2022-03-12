#!/bin/sh

if [[ $# != 2 ]]; then

	echo Usage: tbdize shared_cache private_framework
	echo
	echo This will create a file which \*should\* be compatible with Apple\'s \"tbd\" file format,
        echo which is now what they use in place of dylib stubs. Apple removed pretty much everything
        echo in Xcode 7, and it\'s time to bring it back.
	echo
	echo This MAY be buggy in some cases, AND it doesn\'t export Obj-c classes just yet. I know.
	echo Obj-c will come soon. But I use this all too often, so you might find it useful as well.
	echo 
	echo For comments or bug-reports, please use http://NewOSXBook.com/forum/
	echo
	exit 1
fi

cache=$1
fw=$2


##
## Ensure we have jtool
## 
jtoolTest=`jtool -l /bin/ls 2>/dev/null`
if  [[ -z "$jtoolTest" ]]; then
	echo Where\'s jtool\? Please make sure it\'s in your path \(Recommended: /usr/local/bin\)
	exit 3
fi
##
## Ensure file is a shared cache:
##

cacheTest=`jtool  $cache 2>/dev/null| grep "File is a shared cache" 2>/dev/null`
if  [[ -z "$cacheTest" ]]; then
    echo $cache is not recognized by jtool to be a shared cache. Sorry.
    exit 2
else
# cacheTest also has the architecture
   arch=`echo $cacheTest | cut -d'(' -f2 | cut -d')' -f1`
fi


##
## Now see if found in cache
##
uuidOutput=`jtool -l $cache:$fw 2>/dev/null| grep LC_UUID`

# Output of LC_UUID should be "LC 09: LC_UUID UUID: 419BCF22-D977-32BD-99F6-F7BB6B50E133"

uuid=`echo $uuidOutput | cut -d':' -f3`

if [[ -z "$uuid" ]]; then
	echo Framework $fw not found in cache. 
	exit 3

fi

syms=`jtool -S $cache:$fw 2>/dev/null|grep " [TS] " | cut -d' ' -f3`

objcc=`jtool -d objc $cache:$fw 2>/dev/null`


dir=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/PrivateFrameworks/$fw.framework

if [ ! -d $dir ]; then
	if mkdir -p $dir; then
	echo Created framework directory in the iPhone SDK
	else 
	echo Unable to create framework directory for some reason, will put file in /tmp
	DIR=/tmp
	fi
fi

if touch $dir/$fw.tbd 2>/dev/null; then
   true;
else
   echo Unable to create $dir/$fw.tbd - defaulting to /tmp instead.
   dir=/tmp
fi

## At this point all is in readiness:
#echo "--- !tapi-tbd-v2" 
echo "---"		> $dir/$fw.tbd
echo "archs:           [ $arch ]"  >> $dir/$fw.tbd
#echo "uuids:           [ '$arch': $uuid ]" >> $dir/$fw.tbd
echo "platform:        ios"  >> $dir/$fw.tbd
echo "install-name:    /System/Library/PrivateFrameworks/$fw.framework/$fw" >> $dir/$fw.tbd
#echo "current-version: 0"
#echo "objc-constraint: none"
echo "exports:         "    >> $dir/$fw.tbd
echo "  - archs:           [ $arch ]" >> $dir/$fw.tbd
echo "    symbols:         [ "   >> $dir/$fw.tbd

	for i in $syms; do
		echo "	$i," >> $dir/$fw.tbd
	done
# Add end symbol, because I'm lazy and don't want to remove the "," on last symbol...
echo "  end ]" >> $dir/$fw.tbd

if [[ -z "$objcc" ]]; then
 	true;	# No objc classes here
else
	echo "  objc-classes:    [ " >> $dir/$fw.tbd
	for i in $objcc; do
		echo "	_$i,"  >> $dir/$fw.tbd
	done
	echo "  end ]"  >> $dir/$fw.tbd
fi

echo ...  >> $dir/$fw.tbd


echo Created $dir/$fw.tbd
echo If the file somehow causes compilation error, PLEASE NOTIFY J via http://NewOSXBook.com/forum
