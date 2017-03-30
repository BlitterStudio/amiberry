TARGET  = lib/libguisan.a
  
AR      = ar

DIRS	=$(shell find ./src -maxdepth 3 -type d)
SOURCE	= $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
OBJS    = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

INCLUDE	=-I./include `sdl2-config --cflags`
CFLAGS  = -g -w -O3  $(INCLUDE)
CXXFLAGS= $(CFLAGS)
  
  
.PHONY : all clean
  
$(TARGET) : $(OBJS)
	$(AR) cr $(TARGET) $(OBJS)

all : $(TARGET)

clean :
	find . -name *.o |xargs rm -f
	rm -rf $(TARGET)
