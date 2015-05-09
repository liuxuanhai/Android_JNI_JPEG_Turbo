LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

NDKDEBUG = 0

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_MODE := arm
	LOCAL_MODULE   := libjpegturbo_arm_v7a
endif

ifeq ($(TARGET_ARCH_ABI),armeabi)
	LOCAL_ARM_MODE := arm
	LOCAL_MODULE   := libjpegturbo_arm_v6
endif

ifeq ($(TARGET_ARCH_ABI),x86)
	LOCAL_MODULE := libjpegturbo_x86
endif

LOCAL_CFLAGS   := -O3 -fvisibility=hidden -fstrict-aliasing #-fprefetch-loop-arrays
LOCAL_CFLAGS   += -ffast-math -DNDEBUG -DANDROID -DANDROID_TILE_BASED_DECODE -DENABLE_ANDROID_NULL_CONVERT

LOCAL_CPPFLAGS := -O3 -fvisibility=hidden -fstrict-aliasing #-fprefetch-loop-arrays
LOCAL_CPPFLAGS += -ffast-math -DNDEBUG -DANDROID -DANDROID_TILE_BASED_DECODE -DENABLE_ANDROID_NULL_CONVERT

ifneq ($(TARGET_ARCH_ABI),x86)
	LOCAL_CFLAGS += -DFPM_ARM
endif

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON 		:= true
	ARCH_ARM_HAVE_NEON 	:= true

	LOCAL_CPPFLAGS		+= -mfpu=vfpv3-d16 -mfloat-abi=softfp -march=armv7-a -mfpu=neon -fprefetch-loop-arrays
	LOCAL_CFLAGS   		+= -mfpu=vfpv3-d16 -mfloat-abi=softfp -march=armv7-a -mfpu=neon -fprefetch-loop-arrays
endif

LOCAL_MODULE_TAGS := release

LOCAL_SRC_FILES := jcapimin.c jcapistd.c jccoefct.c \
				   jccolor.c jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c \
				   jcmaster.c jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c \
				   jctrans.c jdapimin.c jdapistd.c jdatadst.c jdatasrc.c \
				   jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c \
				   jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c \
				   jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c \
				   jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c \
				   jquant2.c jutils.c jmemmgr.c jmemnobs.c jaricom.c jcarith.c \
				   jdarith.c turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c

ifeq ($(TARGET_ARCH_ABI),x86)
	LOCAL_SRC_FILES += jsimd_none.c
else
	LOCAL_SRC_FILES += simd/jsimd_arm.c simd/jsimd_arm_neon.S
endif

include $(BUILD_STATIC_LIBRARY)
