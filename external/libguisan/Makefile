TARGET  = lib/libguisan.a
  
AR      = ar
ARARGS  = cr

DIRS	=$(shell find ./src -maxdepth 3 -type d)
SOURCE	= $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
OBJS    = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
DEPS	= $(SOURCE:%.cpp=%.d)

ifneq (,$(findstring osx,$(PLATFORM)))
	# Create DYLIB on OS X
	TARGET=dylib/libguisan.dylib
        LDFLAGS += -lsdl2 -no_branch_islands
        AR=$(CXX) -dynamiclib  -std=c++14 -install_name libguisan.dylib -fvisibility=hidden -undefined dynamic_lookup -o 
	ARARGS=
endif

CPPFLAGS +=-I./include $(SDL_CFLAGS) -MD -MT $@ -MF $(@:%.o=%.d)
  
.PHONY : all clean
  
$(TARGET) : $(OBJS)
	$(AR) $(ARARGS) $(TARGET) $(OBJS)

all : $(TARGET)

clean :
	rm -f $(OBJS) $(DEPS)
	rm -f $(TARGET)
	rm -f lib/libguisan.dylib

cleanprofile:
	rm -f $(OBJS:%.o=%.gcda)

-include $(DEPS)
