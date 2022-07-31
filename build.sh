#!/bin/sh

DEBUG=1

if [ "$DRMBUILD" = 1 ]; then
	WSPATH=Drm
elif [ "$X11BUILD" = 1 ]; then
	WSPATH=X11
elif [ "$EWSBUILD" = 1 ]; then
	WSPATH=EWS
else [ "$X11BUILD" = 1 ]
	WSPATH=NullWS
fi

DEMOS="BinaryShader EvilSkull Fur Particles ChameleonMan Coverflow ExampleUI FilmTV Fractal MagicLantern Skybox Vase\
		DeferredShading Fractal Glass Mouse ParticleSystem PolyBump Navigation Navigation3D PhantomMask Shaders Skybox2 Water"

echo "Making SDKPackage..."
cd $SDKDIR/Tools/OGLES/Build/LinuxGeneric
make PLATFORM=$PLATFORM Debug=$DEBUG
cd $SDKDIR/Tools/OGLES2/Build/LinuxGeneric
make PLATFORM=$PLATFORM Debug=$DEBUG

for i in $DEMOS; do
		if test -d $SDKDIR/Examples/Advanced/$i/OGLES2; then
			cd $SDKDIR/Examples/Advanced/$i/OGLES2/Build/LinuxGeneric
		else
			cd $SDKDIR/Examples/Advanced/$i/OGLES/Build/LinuxGeneric
		fi
		make PLATFORM=$PLATFORM Debug=$DEBUG
done

echo "Installing SDKPackage..."
mkdir -p $INSTALLDIR;
for i in $DEMOS; do
		if [ -z "$DEBUG" ]; then
			if test -d $SDKDIR/Examples/Advanced/$i/OGLES2; then
				cp $SDKDIR/Examples/Advanced/$i/OGLES2/Build/$PLATFORM/Release$WSPATH/OGLES2$i $INSTALLDIR
			else
				cp $SDKDIR/Examples/Advanced/$i/OGLES/Build/$PLATFORM/Release$WSPATH/OGLES$i $INSTALLDIR
			fi
		else
			if test -d $SDKDIR/Examples/Advanced/$i/OGLES2; then
				cp $SDKDIR/Examples/Advanced/$i/OGLES2/Build/$PLATFORM/Debug$WSPATH/OGLES2$i $INSTALLDIR
			else
				cp $SDKDIR/Examples/Advanced/$i/OGLES/Build/$PLATFORM/Debug$WSPATH/OGLES$i $INSTALLDIR
			fi
		fi
		echo $INSTALLDIR/$i installed
done
cp $SDKDIR/bin/run.sh $INSTALLDIR/
echo $INSTALLDIR/Demos/run.sh installed
