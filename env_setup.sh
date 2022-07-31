#!/bin/sh

PLATFORM_NAME=$1

export SDKDIR=$PWD
export OUTPUTDIR=$SDKDIR/Binaries/root/sgx_sdk

# Get Cross Compile Toolchain
if [ ! -d "$SDKDIR/sv-ab02-system-sdk" ]; then
	git clone git@10.219.68.248:a-b02/sv-ab02-system-sdk.git
fi

if [ "$PLATFORM_NAME" = "" ] || [ "$PLATFORM_NAME" = "ab01" ]; then
	echo "use ab01 config..."
	PLATFORM_NAME="ab01"
	export PLATFORM=Linux_armv7hf
	export TOOLCHAIN=$SDKDIR/sv-ab02-system-sdk/gcc-linaro-arm-linux-gnueabihf-4.8-2013.05_linux
	unset DRMBUILD


elif [ "$PLATFORM_NAME" = "ab02" ]; then
	echo "use "$PLATFORM_NAME" config..."
	export PLATFORM=Linux_armv7omap
	export TOOLCHAIN=$SDKDIR/sv-ab02-system-sdk/gcc-linaro-arm-linux-gnueabihf-4.8-2013.05_linux
	export DRMBUILD=1
else
	echo ""$PLATFORM_NAME" platform not support, Please make sure the platform !!"
	return
fi

# Setup EGL and GLES include path for different platform
rm $SDKDIR/Builds/Include -rf
ln -s $SDKDIR/Builds/Include-$PLATFORM_NAME $SDKDIR/Builds/Include

export INSTALLDIR=$OUTPUTDIR/$PLATFORM_NAME

export PS1="\[\e[32;1m\][$PLATFORM_NAME]\[\e[0m\]:\w> "
