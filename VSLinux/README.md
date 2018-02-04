Visual Studio solution using VC++ for Linux.
===========================================
With this solution you can use Visual Studio to edit the sources, remote debug and remote build the project running on a Pi.
For now, no cross compilation is supported in this project but if that is added from Microsoft in the future this solution will be updated accordingly.

The project is configured to be deployed on a standard Raspbian distro, under the folder ~/projects/ (it will create a subfolder named Amiberry there). The sources are copied to the destination Pi (make sure you edit the connection details so they are correct!), then built using the Remote Build commands available in the project Properties.

If you want to use Intellisense, you will need to copy the include files from the Pi locally and point the project to them.

You will need Visual Studio 2015 or later and VC++ for Linux installed.
Look here for more information on VC++ for Linux: 
https://blogs.msdn.microsoft.com/vcblog/2016/03/30/visual-c-for-linux-development/

Here you can find a demo video of Linux Development with C++: 
https://channel9.msdn.com/Series/Visual-Studio-2017-Demo-Videos/Visual-Studio-2017-Linux-Development-with-C
