This tool generates the following files:
- blit.h
- blitfunc.cpp
- blitfunc.h
- blittable.cpp

To use it, compile it for the target platform, then execute it there as follows:

genblitter.exe i > blit.h

genblitter.exe f > blitfunc.cpp

genblitter.exe h > blitfunc.h

genblitter.exe t > blittable.cpp

Copy the resulting files back in the "src" directory of Amiberry