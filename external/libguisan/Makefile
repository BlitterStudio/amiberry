TARGET  = lib/libguisan.a
DYLIB_TARGET = dylib/libguisan.dylib
  
AR      = ar

DIRS	=$(shell find ./src -maxdepth 3 -type d)
SOURCE	= $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
OBJS    = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
DEPS	= $(SOURCE:%.cpp=%.d)

ifeq ($(PLATFORM),osx)
	# Create DYLIB on OS X
        LDFLAGS += -lsdl2 -no_branch_islands
        DYLIB=$(CXX) -dynamiclib  -std=c++14 -install_name libguisan.dylib -fvisibility=hidden -undefined dynamic_lookup -o $(DYLIB_TARGET) $(OBJS)
        CXX=/usr/bin/c++
else
        DYLIB=
endif

CPPFLAGS +=-I./include $(SDL_CFLAGS) -MD -MT $@ -MF $(@:%.o=%.d)
  
.PHONY : all clean
  
$(TARGET) : $(OBJS)
	$(AR) cr $(TARGET) $(OBJS)
	$(DYLIB)

all : $(TARGET)

clean :
	rm -f $(OBJS) $(DEPS)
	rm -f $(TARGET)
	rm -f lib/libguisan.dylib

cleanprofile:
	rm -f $(OBJS:%.o=%.gcda)

-include $(DEPS)
