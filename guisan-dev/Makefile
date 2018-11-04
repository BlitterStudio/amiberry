TARGET  = lib/libguisan.a
  
AR      = ar

DIRS	=$(shell find ./src -maxdepth 3 -type d)
SOURCE	= $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
OBJS    = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

CPPFLAGS +=-I./include -std=gnu++14
  
.PHONY : all clean
  
$(TARGET) : $(OBJS)
	$(AR) cr $(TARGET) $(OBJS)

all : $(TARGET)

clean :
	find . -name *.o |xargs rm -f
	rm -rf $(TARGET)
