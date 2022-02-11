#!/usr/bin/env sh

USERDIR=`echo ~`

LONGVER=`cat src/osdep/target.h | grep AMIBERRYVERSION | awk -F '_T\\\(\\\"' '{print $2}' | awk -F '\\\"' '{printf $1}'`
VERSION=`echo $LONGVER | awk -F v '{printf $2}' | awk '{print $1}'`
MAJOR=`echo $VERSION | awk -F . '{printf $1}'`
MINOR=`echo $VERSION | awk -F . '{printf $2}'`

echo "Removing old App directory"
rm -Rf Amiberry.app
echo "Creating App directory"
# Make directory for App bundle
mkdir -p Amiberry.app/Contents/MacOS
mkdir -p Amiberry.app/Contents/Frameworks
mkdir -p Amiberry.app/Contents/Resources
# Copy executable into App bundle
cp amiberry Amiberry.app/Contents/MacOS/Amiberry
# Copy launch script into the bundle
cp run_amiberry.zsh Amiberry.app/Contents/MacOS
chmod +x Amiberry.app/Contents/MacOS/run_amiberry.zsh
# Copy parameter list into the bundle
cat Info.plist.template | sed -e "s/LONGVERSION/$LONGVER/" | sed -e "s/VERSION/$VERSION/" | sed -e "s/MAJOR/$MAJOR/" | sed -e "s/MINOR/$MINOR/" > Amiberry.app/Contents/Info.plist
# Self-sign binary
export CODE_SIGN_ENTITLEMENTS=Entitlements.plist
codesign --entitlements=Entitlements.plist --force -s - Amiberry.app
# Copy OSX specific conf template into the bundle
cp -R conf Amiberry.app/Contents/Resources/Configurations
# Copy controllers into the bundle
cp -R controllers Amiberry.app/Contents/Resources/Controllers
# Copy kickstarts into the bundle
cp -R kickstarts Amiberry.app/Contents/Resources/Kickstarts
# Copy data into the bundle
cp -R data Amiberry.app/Contents/Resources/Data
# Copy docs into the bundle
cp -R docs Amiberry.app/Contents/Resources/Docs

# Use dylibbundler to install into app if exists
dylibbundler -od -b -x Amiberry.app/Contents/MacOS/Amiberry -d Amiberry.app/Contents/libs/ -s external/libguisan/dylib/
if [ $? -gt 0 ]; then
	echo "Can't find dylibbundler, use brew to install it, or have users manually copy external/libguisan/dylib/libguisan.dylib into /usr/local/lib (needs sudo)"
fi
