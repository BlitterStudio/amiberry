#!/usr/bin/env zsh

CWD_VAR=$(cd "$(dirname "$0")"; pwd)
USERDIR=`echo ~`

if [[ ! -f "$USERDIR/Documents/Amiberry/Configurations/amiberry.conf" ]]; then
	cat $CWD_VAR/../Resources/Configurations/amiberry-osx.conf | sed -e "s#USERDIR#$USERDIR#g" > "$USERDIR/Documents/Amiberry/Configurations/amiberry.conf"
fi

for file in $CWD_VAR/../Resources/Configurations/**/*(.); do
	if [[ "$file" != "$CWD_VAR/../Resources/Configurations/amiberry-osx.conf" ]]; then
		if [[ "$file" != "$CWD_VAR/../Resources/Configurations/amiberry.conf" ]]; then
			if [[ ! -f "$USERDIR/Documents/Amiberry/Configurations${file##*/Configurations}" ]]; then
				echo "Copying $file to $USERDIR/Documents/Amiberry/Configurations${file##*/Configurations}"
				mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Configurations${file##*/Configurations}")
				cp $file "$USERDIR/Documents/Amiberry/Configurations${file##*/Configurations}"
			fi
		fi
	fi
done

for file in $CWD_VAR/../Resources/Abr/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Abr${file##*/Abr}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Abr${file##*/Abr}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Abr${file##*/Abr}")
		cp $file "$USERDIR/Documents/Amiberry/Abr${file##*/Abr}"
	fi
done

for file in $CWD_VAR/../Resources/Cdroms/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Cdroms${file##*/Cdroms}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Cdroms${file##*/Cdroms}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Cdroms${file##*/Cdroms}")
		cp $file "$USERDIR/Documents/Amiberry/Cdroms${file##*/Cdroms}"
	fi
done

for file in $CWD_VAR/../Resources/Controllers/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Controllers${file##*/Controllers}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Controllers${file##*/Controllers}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Controllers${file##*/Controllers}")
		cp $file "$USERDIR/Documents/Amiberry/Controllers${file##*/Controllers}"
	fi
done

for file in $CWD_VAR/../Resources/Kickstarts/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Kickstarts${file##*/Kickstarts}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Kickstarts${file##*/Kickstarts}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Kickstarts${file##*/Kickstarts}")
		cp $file "$USERDIR/Documents/Amiberry/Kickstarts${file##*/Kickstarts}"
	fi
done

for file in $CWD_VAR/../Resources/Data/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Data${file##*/Data}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Data${file##*/Data}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Data${file##*/Data}")
		cp $file "$USERDIR/Documents/Amiberry/Data${file##*/Data}"
	fi
done

for file in $CWD_VAR/../Resources/Floppies/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Floppies${file##*/Floppies}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Floppies${file##*/Floppies}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Floppies${file##*/Floppies}")
		cp $file "$USERDIR/Documents/Amiberry/Floppies${file##*/Floppies}"
	fi
done

for file in $CWD_VAR/../Resources/Harddrives/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Harddrives${file##*/Harddrives}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Harddrives${file##*/Harddrives}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Harddrives${file##*/Harddrives}")
		cp $file "$USERDIR/Documents/Amiberry/Harddrives${file##*/Harddrives}"
	fi
done

for file in $CWD_VAR/../Resources/Savestates/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Savestates${file##*/Savestates}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Savestates${file##*/Savestates}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Savestates${file##*/Savestates}")
		cp $file "$USERDIR/Documents/Amiberry/Savestates${file##*/Savestates}"
	fi
done

for file in $CWD_VAR/../Resources/Inputrecordings/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Inputrecordings${file##*/Inputrecordings}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Inputrecordings${file##*/Inputrecordings}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Inputrecordings${file##*/Inputrecordings}")
		cp $file "$USERDIR/Documents/Amiberry/Inputrecordings${file##*/Inputrecordings}"
	fi
done

for file in $CWD_VAR/../Resources/Screenshots/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Screenshots${file##*/Screenshots}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Screenshots${file##*/Screenshots}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Screenshots${file##*/Screenshots}")
		cp $file "$USERDIR/Documents/Amiberry/Screenshots${file##*/Screenshots}"
	fi
done

for file in $CWD_VAR/../Resources/Lha/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Lha${file##*/Lha}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Lha${file##*/Lha}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Lha${file##*/Lha}")
		cp $file "$USERDIR/Documents/Amiberry/Lha${file##*/Lha}"
	fi
done

for file in $CWD_VAR/../Resources/Nvram/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Nvram${file##*/Nvram}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Nvram${file##*/Nvram}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Nvram${file##*/Nvram}")
		cp $file "$USERDIR/Documents/Amiberry/Nvram${file##*/Nvram}"
	fi
done

for file in $CWD_VAR/../Resources/Plugins/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Plugins${file##*/Plugins}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Plugins${file##*/Plugins}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Plugins${file##*/Plugins}")
		cp $file "$USERDIR/Documents/Amiberry/Plugins${file##*/Plugins}"
	fi
done

for file in $CWD_VAR/../Resources/Whdboot/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}")
		cp $file "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}"
	fi
done
