#!/bin/sh

USERDIR=`echo ~`

LONGVER=`cat src/osdep/target.h | grep AMIBERRYVERSION | awk -F '_T\\\(\\\"' '{print $2}' | awk -F '\\\"' '{printf $1}'`
VERSION=`echo $LONGVER | awk -F v '{printf $2}' | awk '{print $1}'`
MAJOR=`echo $VERSION | awk -F . '{printf $1}'`
MINOR=`echo $VERSION | awk -F . '{printf $2}'`


cat Info.plist.template | sed -e "s/LONGVERSION/$LONGVER/" | sed -e "s/VERSION/$VERSION/" | sed -e "s/MAJOR/$MAJOR/" | sed -e "s/MINOR/$MINOR/" >Info.plist
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
cp Info.plist Amiberry.app/Contents/Info.plist
# Self-sign binary
export CODE_SIGN_ENTITLEMENTS=Entitlements.plist
codesign --entitlements=Entitlements.plist --force -s - Amiberry.app
# Copy Guisan library into the bundle
cp external/libguisan/dylib/libguisan.dylib Amiberry.app/Contents/MacOS
# Install libguisan lib into app bundle
# Copy docs into the bundle
cp -R docs Amiberry.app/Contents/Resources
# Copy data into the bundle
cp -R data Amiberry.app/Contents/Resources
# Copy kickstarts into the bundle
cp -R kickstarts Amiberry.app/Contents/Resources
# Overwrite default conf with OSX specific one
mkdir Amiberry.app/Contents/resources/conf
cat conf/amiberry-osx.conf | sed -e "s#USERDIR#$USERDIR#g" >Amiberry.app/Contents/Resources/conf/amiberry.conf

# Create application directories
mkdir -p ~/Documents/Amiberry/Hard\ Drives ~/Documents/Amiberry/Configurations ~/Documents/Amiberry/Controllers ~/Documents/Amibery/Logfiles ~Documents/Amiberry/Kickstarts ~Documents/Amiberry/RP9 ~/Documents/Amiberry/Data/Floppy_Sounds ~/Documents/Amiberry/Savestates ~/Documents/Amiberry/Screenshots ~/Documents/Amiberry/Docs

# Copy files to their destination
cp -a conf/* ~/Documents/Amiberry/Configurations
cp -a controllers/* ~/Documents/Amiberry/Controllers
cp -a kickstarts/* ~/Documents/Amiberry/Kickstarts
cp -a data/* ~/Documents/Amiberry/Data
cp -a docs/* ~/Documents/Amiberry/Docs
