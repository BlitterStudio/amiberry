#!/usr/bin/env sh

USERDIR=`echo ~`

LONGVER=`cat src/osdep/target.h | grep AMIBERRYVERSION | awk -F '_T\\\(\\\"' '{print $2}' | awk -F '\\\"' '{printf $1}'`
VERSION=`echo $LONGVER | awk -F v '{printf $2}' | awk '{print $1}'`
MAJOR=`echo $VERSION | awk -F . '{printf $1}' | sed 's/[^0-9]*//g'`
MINOR=`echo $VERSION | awk -F . '{printf $2}' | sed 's/[^0-9]*//g'`

echo "Removing old App directory"
rm -Rf Amiberry.app
echo "Creating App directory"
# Make directory for App bundle
mkdir -p Amiberry.app/Contents/MacOS
mkdir -p Amiberry.app/Contents/Frameworks
mkdir -p Amiberry.app/Contents/Resources
# Copy executable into App bundle
cp amiberry Amiberry.app/Contents/MacOS/Amiberry
# Copy capsimg.so into App bundle
cp capsimg.so Amiberry.app/Contents/MacOS/capsimg.so
# Copy init script into the bundle
cp macos_init_amiberry.zsh Amiberry.app/Contents/Resources
chmod +x Amiberry.app/Contents/Resources/macos_init_amiberry.zsh
# Copy parameter list into the bundle
cat Info.plist.template | sed -e "s/LONGVERSION/$LONGVER/" | sed -e "s/VERSION/$VERSION/" | sed -e "s/MAJOR/$MAJOR/" | sed -e "s/MINOR/$MINOR/" > Amiberry.app/Contents/Info.plist
# Self-sign binary
export CODE_SIGN_ENTITLEMENTS=Entitlements.plist
codesign --entitlements=Entitlements.plist --force -s - Amiberry.app
# Copy directories into the bundle
cp -R abr Amiberry.app/Contents/Resources/abr
cp -R conf Amiberry.app/Contents/Resources/Configurations
cp -R controllers Amiberry.app/Contents/Resources/Controllers
cp -R data Amiberry.app/Contents/Resources/Data
cp -R inputrecordings Amiberry.app/Contents/Resources/Inputrecordings
cp -R kickstarts Amiberry.app/Contents/Resources/Kickstarts
cp -R nvram Amiberry.app/Contents/Resources/Nvram
cp -R savestates Amiberry.app/Contents/Resources/Savestates
cp -R screenshots Amiberry.app/Contents/Resources/Screenshots
cp -R whdboot Amiberry.app/Contents/Resources/Whdboot

# Overwrite default conf with OSX specific one
mkdir Amiberry.app/Contents/Resources/conf
cat conf/amiberry-osx.conf | sed -e "s#USERDIR#$USERDIR#g" >Amiberry.app/Contents/Resources/conf/amiberry.conf
# Use dylibbundler to install into app if exists
dylibbundler -od -b -x Amiberry.app/Contents/MacOS/Amiberry -d Amiberry.app/Contents/libs/ -s external/libguisan/dylib/
if [ $? -gt 0 ]; then
    echo "Can't find dylibbundler, use brew to install it, or manually copy external/libguisan/dylib/libguisan.dylib into /usr/local/lib (you'll need sudo)"
fi

