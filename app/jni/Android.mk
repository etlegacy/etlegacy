LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS    := -ldl -llog -landroid
LOCAL_MODULE    := q3eloader
LOCAL_SRC_FILES := q3e.c

include $(BUILD_SHARED_LIBRARY)
