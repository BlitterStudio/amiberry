%global _hardened_build 1
%global debug_package %{nil}

Name:           amiberry
Version:        8.1.4
Release:        1%{?dist}
Summary:        Optimized Amiga emulator for ARM64, AMD64 and RISC-V platforms

License:        GPL-3.0-or-later
URL:            https://amiberry.com
Source0:        https://github.com/BlitterStudio/amiberry/archive/refs/tags/v%{version}.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  flac-devel
BuildRequires:  libmpeg2-devel
BuildRequires:  libpng-devel
BuildRequires:  SDL3-devel
BuildRequires:  SDL3_image-devel
BuildRequires:  alsa-lib-devel
BuildRequires:  mpg123-devel
BuildRequires:  portmidi-devel
BuildRequires:  libserialport-devel
BuildRequires:  enet-devel
BuildRequires:  libpcap-devel
BuildRequires:  libzstd-devel
BuildRequires:  libcurl-devel
BuildRequires:  json-devel

Requires:       glibc
Requires:       libstdc++
Requires:       SDL3
Requires:       SDL3_image
Requires:       flac
Requires:       mpg123
Requires:       libpng
Requires:       zlib
Requires:       libcurl
Requires:       zstd
Requires:       libserialport
Requires:       portmidi
Requires:       enet

%description
Amiberry is an optimized Amiga emulator for ARM64, AMD64 and RISC-V platforms.
It is based on the latest WinUAE and supports various Amiga models, including
the A4000T, A4000D, A1200, A3000 and A600. Features include support for WHDLoad
titles, RetroArch integration, custom controller mappings, and more.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix}
%cmake_build

%install
%cmake_install

%post
%{_bindir}/update-desktop-database 2>/dev/null || :
%{_bindir}/update-mime-database %{_datadir}/mime 2>/dev/null || :
touch --no-create %{_datadir}/icons/hicolor 2>/dev/null || :
%{_bindir}/gtk-update-icon-cache %{_datadir}/icons/hicolor 2>/dev/null || :

%postun
%{_bindir}/update-desktop-database 2>/dev/null || :
%{_bindir}/update-mime-database %{_datadir}/mime 2>/dev/null || :
if [ $1 -eq 0 ] ; then
    touch --no-create %{_datadir}/icons/hicolor 2>/dev/null || :
    %{_bindir}/gtk-update-icon-cache %{_datadir}/icons/hicolor 2>/dev/null || :
fi

%files
%license LICENSE
%doc %{_docdir}/amiberry/
%{_bindir}/amiberry
%{_libdir}/amiberry/
%{_datadir}/amiberry/
%{_datadir}/applications/Amiberry.desktop
%{_datadir}/icons/hicolor/*/apps/amiberry.png
%{_datadir}/metainfo/Amiberry.metainfo.xml
%{_datadir}/mime/packages/amiberry.xml
%{_mandir}/man1/amiberry.1.gz

%changelog
* Wed Apr 01 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.4-1
- Bugfix release with improved bootstrap and portable mode handling
- Fixed macOS Intel JIT startup crashes and KMSDRM fullscreen/drawable size issues
- Added Vulkan render-thread support, GUI window state persistence, and Android UX improvements

* Mon Mar 30 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.3-1
- Stable v8.1.3 release

* Thu Mar 26 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.2-1
- Stable v8.1.2 release

* Tue Mar 25 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.1-1
- Stable v8.1.1 release

* Sun Mar 23 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.0-1
- Stable v8.1.0 release

* Sun Mar 15 2026 Dimitris Panokostas <midwan@gmail.com> - 8.0.0-1
- Stable v8.0.0 release

* Sat Mar 14 2026 Dimitris Panokostas <midwan@gmail.com> - 8.0.0-0.pre30
- Initial COPR packaging
