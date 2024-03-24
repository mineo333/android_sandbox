LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH),arm64)

include $(CLEAR_VARS)

LOCAL_MODULE_TARGET_ARCH := arm64
LOCAL_MULTILIB := 64
LOCAL_MODULE := jenv
LOCAL_SRC_FILES := lib/libjenv.so 
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)

LOCAL_MODULE_TARGET_ARCH := arm64
LOCAL_MULTILIB := 64
LOCAL_MODULE := libdroidguard
LOCAL_SRC_FILES := lib/libdroidguard.so 
include $(PREBUILT_SHARED_LIBRARY)
include $(CLEAR_VARS)


LOCAL_MODULE_TARGET_ARCH := arm64
LOCAL_MULTILIB := 64
LOCAL_MODULE := play_integrity
LOCAL_SHARED_LIBRARIES :=  jenv libdroidguard
#LOCAL_SHARED_LIBRARIES += 
LOCAL_SRC_FILES := src/main.cpp src/pmparser.cpp src/elf_parser.cpp src/art_resolver.cpp
LOCAL_C_INCLUDES := include

include $(BUILD_EXECUTABLE)
endif