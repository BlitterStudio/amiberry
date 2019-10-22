LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := guisan

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/../SDL/include $(LOCAL_PATH)/include \
					$(LOCAL_PATH)/../SDL_image $(LOCAL_PATH)/../SDL_ttf/include
LOCAL_CFLAGS := -DHAVE_CONFIG_H -D_GNU_SOURCE=1 -D_REENTRANT -fexceptions -frtti

LOCAL_SRC_FILES := \
            $(wildcard $(LOCAL_PATH)/src/*.cpp) \
            $(wildcard $(LOCAL_PATH)/src/sdl/*.cpp) \
            $(wildcard $(LOCAL_PATH)/src/widgets/*.cpp)

#$(wildcard $(LOCAL_PATH)/src/opengl/*.cpp) \

LOCAL_SHARED_LIBRARIES += SDL2 SDL2_image SDL2_ttf

LOCAL_STATIC_LIBRARIES := 

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)