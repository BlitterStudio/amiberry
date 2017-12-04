## NetBeans Project

This folder contains the project files to enable remote compiling and debugging of Amiberry. Currently tested on Windows version of the NetBeans IDE.


### IDE Setup

* Download Netbeans C++ IDE from: https://netbeans.org/
* Download JDK from : http://www.oracle.com/technetwork/java/javase/downloads
* Update Netbeans Shortcut Target to the following provided default paths are used: `"C:\Program Files\NetBeans 8.2\bin\netbeans64.exe" --jdkhome "C:\Program Files\Java\jdk1.8.0_121"`
* Launch NetBeans from the modified shortcut
* Open Project > Navigate to this folder on your local system
* Assumed your RPi has the hostname of `retropie`, if not, add a new C/C++ Build Host under the Services tab, entering Hostname/IP and username/password, leaving the rest as default


### Pi Setup
* On the remote RaspberryPi
* Setup a script for executing gdb as root
	* `sudo nano /home/pi/debug_script`
	* Copy the following in to this new file:
```shell
#!/bin/bash
PROG=$(which gdb)
sudo $PROG "$@" 2>&1 /tmp/error.log
```
* Set new script as excutable
	* `sudo chmod +x /home/pi/debug_script`


### Remote Debugging
Make sure the Debug profile is selected in NetBeans to ensure `-g` switch is enabled when compiling. If breakpoints don't hit when you expect them to do a make clean from NetBeans.

