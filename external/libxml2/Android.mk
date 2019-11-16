LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libxml2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include 
LOCAL_CFLAGS := -DLIBXML_THREAD_ENABLED -DSTATIC_LIBXML

LOCAL_SRC_FILES := \
	SAX.c entities.c encoding.c error.c \
	parserInternals.c parser.c tree.c hash.c list.c xmlIO.c \
	xmlmemory.c uri.c valid.c xlink.c \
	debugXML.c xpath.c xpointer.c xinclude.c \
	DOCBparser.c catalog.c globals.c threads.c c14n.c xmlstring.c \
	buf.c xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
	xmlreader.c relaxng.c dict.c SAX2.c xmlwriter.c legacy.c \
	chvalid.c pattern.c xmlsave.c xmlmodule.c schematron.c \

LOCAL_SHARED_LIBRARIES :=
LOCAL_STATIC_LIBRARIES :=

include $(BUILD_STATIC_LIBRARY)
