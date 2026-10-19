%global _hardened_build 1
%global debug_package %{nil}

Name:           amiberry
Version:        8.4.2
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
BuildRequires:  ffmpeg-free-devel
BuildRequires:  libwebp-devel
BuildRequires:  libpng-devel
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
Requires:       libavcodec-free
Requires:       libavformat-free
Requires:       libavutil-free
Requires:       libswresample-free
Requires:       libswscale-free
Requires:       libwebp
Requires:       mpg123
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
* Mon Oct 19 2026 Dimitris Panokostas <midwan@gmail.com> - 8.4.2-1
- Bugfix release v8.4.2
- Linux AppImages for x86_64 and aarch64; Ubuntu 26.10 packages; install.sh stops on unpublished apt suites
- Multithreaded CPU: restored JIT performance and fixed keyboard reset crashes and missed reset holds
- WHDBooter: exact SHA1 matches win, outdated game databases are refreshed, and 68030/68040 CPU and cycle-exact settings from the database are applied
- Fixed FPS adj. in the Display panel, Chipset Extra changes on hard reset, live .uaem toggle and Host Only cursor hotspot drift
- IPC: Windows named pipe support, GDB monitor commands, no longer limited to one request per frame
- ZZ9000: WebP, FLAC, Ogg Vorbis, WebM and MPEG-2 services
- Merged WinUAE display, NTSC long field and GDB monitor updates; new Always stretch PAL as NTSC option
- Android: device-shaped RTG modes, AAudio at 48 kHz with a configurable sound buffer, steady stick mouse speed

* Wed Oct 14 2026 Dimitris Panokostas <midwan@gmail.com> - 8.4.1-1
- Bugfix release v8.4.1
- Libretro: save states load through the regular restore path, auto-loaded states apply after boot, left stick moves the mouse in Joystick As Mouse
- Fixed buffer sizes truncating restored directory paths and saved strings such as hardfile geometry
- Fixed Host Only cursor height on the right half of native screens and ROM folder handling with relative paths
- Merged WinUAE fixes: MMU cache-inhibited pages, filesystem rename with open files, CIA TOD timing, A2024/Graffiti/genlock overscan
- Added optional GDB remote debugging server
- Packaging: MacPorts support on macOS, FreeBSD pkg release assets, libavcodec-extra alternatives in DEB dependencies
- Build fixes for GLES-only and USE_PPC=OFF configurations; updated AROS ROMs

* Sun Oct 11 2026 Dimitris Panokostas <midwan@gmail.com> - 8.4.0-1
- Feature release v8.4.0
- ZZ9000: AHI audio, SANA-II network, SDK mailbox/MHI audio, MPEGA streams, image/ZZPlay services, firmware 2.8 register ABI, direct VRAM mapping
- Support for an external MiniGL renderer plugin (available separately) via uaelib trap 120, with dma-buf frame compositing; user plugin search locations
- Custom cartridge ROM support, .A1000 ROMs, relative media paths, --dump-config CLI option, ipc_disable_logging option
- RTG: dirty-tracking race/split-screen and direct JIT store fixes, hardware sprite on by default, warm reset fixes
- JIT: ARM64 trap-block demotion by opcode class, register shift count fixes, JIT FPU setting respected, KMSDRM fault handler ordering
- Performance: RPi4 zero-copy native frames and faster AGA line drawing, duty-cycled auto-crop scan
- Graphics: runtime GL capability detection, KMSDRM GLES-first context ladder, autocrop and centering fixes
- Networking: bsdsocket blocking/timeout fixes, SANA-II receive/multicast fixes, uaenet MTU/MAC and macOS BPF fixes
- Libretro: on-screen keyboard, zip/7z disk control, HDF without RDB, Windows CHD fix, core info generation
- Android: launcher/touch/TV fixes, controller OSK toggle, analog stick mouse, ChromeOS desktop improvements
- Merged latest WinUAE and ImGui updates; FreeBSD amd64/aarch64 CI; updated AROS ROMs

* Wed Aug 05 2026 Dimitris Panokostas <midwan@gmail.com> - 8.3.0-1
- Feature release v8.3.0
- Added ZZ9000 RTG emulation, RP9 package support, Global Settings panel, and configuration category grouping
- Separate GUI window on Windows; host-only cursor hotspot, jumps, and alignment fixes
- Autocrop improvements: non-black border detection, live refresh, libretro crop overscan
- Android: touch tap-to-click, Files panel layout, native shader selection, touch trackpad mouse, ADPF adaptive CPU tuning, config save-as UX
- Performance: RPi4 KMSDRM/Copper/PGO, renderer/machdep/P96 hot paths, PPC QEMU flush tuning
- KMSDRM display timing and GUI resume fixes; SCSI/SDMAC/53C710 emulation fixes
- Networking: Ariadne II, X-Surf, Ethernet receive, and serial RBF fixes
- Libretro audio starvation, ROM lookup, and build fixes; SDL updated to 3.4.12
- Merged latest WinUAE updates; added libretro buildbot CI; Capsimg 5.2 support

* Fri Jun 19 2026 Dimitris Panokostas <midwan@gmail.com> - 8.2.2-1
- Stable v8.2.2 bugfix release
- Fixed libretro/MHI audio starvation and playback backpressure, raw CD reads, and OS4 examinedata fallback
- Improved RTG/P96 presentation and dirty tracking, PPC QEMU JIT flushes, KMSDRM mouse capture, config saves, and derivative distro packaging

* Mon Jun 15 2026 Dimitris Panokostas <midwan@gmail.com> - 8.2.1-1
- Stable v8.2.1 bugfix release
- Improved libretro audio underrun recovery
- Fixed sampler-device handling for capture and refreshed WinUAE main-thread callback handling

* Fri Jun 12 2026 Dimitris Panokostas <midwan@gmail.com> - 8.2.0-1
- Stable v8.2.0 release
- Slow ARM host performance diagnostics, defaults, and Raspberry Pi 4 tuning
- PPC/QEMU-UAE, JIT, libretro, Android input, networking, GUI, and packaging fixes

* Sat May 09 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.6-1
- Stable v8.1.6 release
- Windows ARM64 and llvm-mingw build improvements
- JIT, RTG, OpenGL, VSync, Android input, libretro path, and config healing fixes

* Tue Apr 14 2026 Dimitris Panokostas <midwan@gmail.com> - 8.1.5-1
- Stable v8.1.5 release

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
