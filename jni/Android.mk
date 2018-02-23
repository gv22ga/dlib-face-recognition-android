LOCAL_PATH := $(call my-dir)
SUB_MK_FILES := $(call all-subdir-makefiles)

## Build dlib to static library
include $(CLEAR_VARS)
LOCAL_MODULE := dlib
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../dlib

LOCAL_SRC_FILES += \
                ../$(LOCAL_PATH)/../dlib/dlib/all/source.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

TOP_LEVEL_PATH := $(abspath $(LOCAL_PATH)/..)
$(info TOP Level Path: $(TOP_LEVEL_PATH))

EXT_INSTALL_PATH = $(TOP_LEVEL_PATH)/third_party

OPENCV_PATH = $(EXT_INSTALL_PATH)/opencv/jni
OPENCV_INCLUDE_DIR = $(OPENCV_PATH)/include

MINIGLOG_LIB_TYPE := STATIC
MINI_GLOG_PATH = $(EXT_INSTALL_PATH)/miniglog
include $(MINI_GLOG_PATH)/Android.mk

include $(SUB_MK_FILES)
