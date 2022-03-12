#!/usr/bin/env zsh

CWD_VAR=$(cd "$(dirname "$0")"; pwd)
USERDIR=`echo ~`

if [[ ! -d "$USERDIR/Documents/Amiberry/Hard Drives" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Hard Drives"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Configurations" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Configurations"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Controllers" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Controllers"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Logfiles" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Logfiles"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Kickstarts" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Kickstarts"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/game-data" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/game-data"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/save-data" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/save-data"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/save-data/Autoboots" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/save-data/Autoboots"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/save-data/Debugs" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/save-data/Debugs"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/save-data/Kickstarts" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/save-data/Kickstarts"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Whdboot/save-data/Savegames" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Whdboot/save-data/Savegames"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Data/Floppy_Sounds" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Data/Floppy_Sounds"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Savestates" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Savestates"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Screenshots" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Screenshots"
fi

if [[ ! -d "$USERDIR/Documents/Amiberry/Docs" ]]; then
	mkdir -p "$USERDIR/Documents/Amiberry/Docs"
fi

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

for file in $CWD_VAR/../Resources/Docs/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Docs${file##*/Docs}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Docs${file##*/Docs}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Docs${file##*/Docs}")
		cp $file "$USERDIR/Documents/Amiberry/Docs${file##*/Docs}"
	fi
done

for file in $CWD_VAR/../Resources/Whdboot/**/*(.); do
	if [[ ! -f "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}" ]]; then
		echo "Copying $file to $USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}"
		mkdir -p $(dirname "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}")
		cp $file "$USERDIR/Documents/Amiberry/Whdboot${file##*/Whdboot}"
	fi
done

echo "Running Amiberry"

$CWD_VAR/Amiberry
