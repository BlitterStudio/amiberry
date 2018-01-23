# Amibian for Armbian / TinkerBoard | Beta 1.3

Possible Requirements:
1) I am testing with this Armbian system image:

   https://dl.armbian.com/tinkerboard/Debian_stretch_next.7z

   Its possible that any kernel that supports the Mali GPU userspace
   drivers will work, but if no display comes up try this kernel.
   If you get this running on TinkerOS let me know, I haven't
   tried that one yet.
   I HIGHLY recommend this be a fresh write of the debian-stretch-next
   image.  This WILL install the Amibian kernel right on top of it.
2) 5v 3A power, or 5.25v 2.5A, probably best hooked directly to the
   5v GPIO header on the tinkerboard.  If you get it working without this,
   cool.  But I have found I needed this to keep the Tinkerboard stable
   with or without emulation.
3) Preferably cooling, until I can roll a new kernel, it starts throttling
   at 70c, giving you a great experience for about 30 mins until slowing to
   a crawl.   Try it without it first, but if the emulator seems super slow
   please check:
   
    ```cat /sys/devices/virtual/thermal/thermal_zone0/temp```
    
   Preferably while the emulator is running.  If it is 70000 or higher, the
   Tinkerboard is overheating and is throttling.

Note: Run everything as root, including the emulator.
Note: Faster SD Card = BETTER.

Directions to get this going:
1) Get the Armbian image above, use something like Win32DiskImager or dd (unix)
   to image it onto an SD card.
2) Place in Tinkerboard, boot, and set up Armbian according to the onscreen directions.
3) Download your amibian-tinker-<date/time>.tar.gz to your /root dir (not /), then extract it with

   ```tar xzvf <name of file```

4) Run the setup command below:

   ```./setup.sh```

   Building the system will take 10-30 mins depending on SD card speed, but ensures
   that all the components on the system are using accelerated drivers.
6) Reboot your future Amiga:

   ```reboot```

Note: You need a ROM, preferably Kickstart 3.1 but anything should do.
      You need either a floppy or hardfile ready, or a bootable directory.
      Copy them into the 'amiberry' directory in the right places.

The setup script autoconfigures samba with sane parameters.
When you log in again your IP is displayed.  Navigate to it on your windows
machine and place your ROMs and kickstarts in.  This method should be usable
on Windows, Mac, Linux and even sufficiently advanced Amigas.

To run it:

 ```
 cd ~/amiberry
 ./amiberry-tinker-dev
```
To try the upcoming "GO64" support:
```
 cd ~/vice-3.1
 src/x64                           # or src/x128, or src/x<my fav commodore)
```
Some programs are provided in SMB0 to benchmark the performance and to test in
general.   Picasso96 is also provided.  You will need it to enjoy RTG.  Currently
packed are:
 * Sysinfo and AIBB, for benchmarking and stresstesting
 * Picasso 96 the installer
 * Intuitracker and some mods for audio testing
 * The Requiem demo, AGA stresstesting and video speed testing.

Features:
1) OpenGLES 2.0 acceleration by the Mali Userspace drivers provided.
2) RTG: Up to 1920x1080, 8/16/32 supported but 16bit is the native and FASTEST mode.
3) Overclock to 1.9ghz.  Feel free to use the /boot/dtb/rk3288-tinker-boost.dtb with
   other kernels with a crippled device tree definition.

Known issues:
1) (Amiga)
   The ARM and NEON assembler helper routines do not work on the RK3288. Crash RTG
   right out, illegal instruction. I implemented a byte swap and move routine in C++
   to replace it which I am sure is a little slower. Mostly this is used to move the
   Amiga RTG FB to the OpenGLES texture that renders the display. Someone who knows
   ARM assembler can recode this so that it is compatible with the RK3288.
2) (Tinkerboard)
   Big one here.. Eventually the Tinkerboard heats up to 70c causing it to throttle.
   Then the emulation gets slow. This may not occur in your situation especially if
   your tinkerboard is already cooled.   Once I cooled mine it ran nice.
3) (Amiga)
   Timing.   We are working on the ideal timing for ASUS tinkerboard.  Sound works
   but may be 16% slower than normal in some or all cases.
4) (Commodore)
   Emulation for xscpu64 is slow.   I will probably tweak the 65816 down to 5mhz so
   that it will be usable.   Cycle exact Super64 at 20mhz is probably not happening on
   tinkerboard
5) (Commodore)
   ReSID is not used nor compiled in, just regular SID emulation only.   It used up
   WAY too much CPU and made the emulation ... sad.

Please log any other bugs observed (tinkerboard related only please) to:
   https://github.com/alynna/amiberry/issues
