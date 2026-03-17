#!/bin/sh
# Only run on full uninstall ($1 = 0), not on upgrades
if [ "$1" -eq 0 ] 2>/dev/null; then
    update-desktop-database -q 2>/dev/null || true
    update-mime-database /usr/share/mime 2>/dev/null || true
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
fi
