LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)


ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_MODE := arm
	LOCAL_ARM_NEON  := true
	ARCH_ARM_HAVE_NEON 	:= true
endif


ifeq ($(TARGET_ARCH_ABI),armeabi)
	LOCAL_ARM_MODE := arm
endif