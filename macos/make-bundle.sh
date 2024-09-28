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
# Copy parameter list into the bundle
cat macos/Info.plist.template | sed -e "s/LONGVERSION/$LONGVER/" | sed -e "s/VERSION/$VERSION/" | sed -e "s/MAJOR/$MAJOR/" | sed -e "s/MINOR/$MINOR/" > Amiberry.app/Contents/Info.plist
# Self-sign binary
export CODE_SIGN_ENTITLEMENTS=Entitlements.plist
codesign --entitlements=Entitlements.plist --force -s - Amiberry.app
# Copy directories into the bundle
cp -R cdroms Amiberry.app/Contents/Resources/Cdroms
cp -R conf Amiberry.app/Contents/Resources/Configurations
cp -R controllers Amiberry.app/Contents/Resources/Controllers
cp -R data Amiberry.app/Contents/Resources/Data
cp -R floppies Amiberry.app/Contents/Resources/Floppies
cp -R harddrives Amiberry.app/Contents/Resources/Harddrives
cp -R inputrecordings Amiberry.app/Contents/Resources/Inputrecordings
cp -R roms Amiberry.app/Contents/Resources/Roms
cp -R lha Amiberry.app/Contents/Resources/Lha
cp -R nvram Amiberry.app/Contents/Resources/Nvram
cp -R plugins Amiberry.app/Contents/Resources/Plugins
cp -R savestates Amiberry.app/Contents/Resources/Savestates
cp -R screenshots Amiberry.app/Contents/Resources/Screenshots
cp -R whdboot Amiberry.app/Contents/Resources/Whdboot

# Overwrite default conf with OSX specific one
cat conf/amiberry-osx.conf | sed -e "s#USERDIR#$USERDIR#g" >Amiberry.app/Contents/Resources/Configurations/amiberry.conf
