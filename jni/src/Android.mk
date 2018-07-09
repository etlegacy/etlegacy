LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/minizip.mk
include $(LOCAL_PATH)/zlib.mk
include $(LOCAL_PATH)/iffaddrs.mk
include $(LOCAL_PATH)/renderer_opengl1_armv7-a.mk
include $(LOCAL_PATH)/curl.mk
include $(LOCAL_PATH)/jpeg.mk

include $(CLEAR_VARS)

SDL_PATH := ../../libs/sdl2  #FIXME
ETL_PATH := ../../src/

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include


# Add files required for tinygettext and make it work
LOCAL_SRC_FILES := $(SDL_PATH)/main/android/SDL_android_main.c
LOCAL_SRC_FILES := $(ETL_PATH)/qcommon/cmd.c \
$(ETL_PATH)/qcommon/cm_load.c \
$(ETL_PATH)/qcommon/cm_patch.c \
$(ETL_PATH)/qcommon/cm_polylib.c \
$(ETL_PATH)/qcommon/cm_test.c \
$(ETL_PATH)/qcommon/cm_trace.c \
$(ETL_PATH)/qcommon/common.c \
$(ETL_PATH)/qcommon/cvar.c \
$(ETL_PATH)/qcommon/download.c \
$(ETL_PATH)/qcommon/files.c \
$(ETL_PATH)/qcommon/htable.c \
$(ETL_PATH)/qcommon/huffman.c \
$(ETL_PATH)/qcommon/md4.c \
$(ETL_PATH)/qcommon/md5.c \
$(ETL_PATH)/qcommon/msg.c \
$(ETL_PATH)/qcommon/net_chan.c \
$(ETL_PATH)/qcommon/net_ip.c \
$(ETL_PATH)/qcommon/puff.c \
$(ETL_PATH)/qcommon/q_math.c \
$(ETL_PATH)/qcommon/q_shared.c \
$(ETL_PATH)/qcommon/q_unicode.c \
$(ETL_PATH)/qcommon/update.c \
$(ETL_PATH)/qcommon/vm.c \
$(ETL_PATH)/qcommon/vm_interpreted.c \
$(ETL_PATH)/server/sv_bot.c \
$(ETL_PATH)/server/sv_ccmds.c \
$(ETL_PATH)/server/sv_client.c \
$(ETL_PATH)/server/sv_demo.c \
$(ETL_PATH)/server/sv_demo_ext.c \
$(ETL_PATH)/server/sv_game.c \
$(ETL_PATH)/server/sv_init.c \
$(ETL_PATH)/server/sv_main.c \
$(ETL_PATH)/server/sv_net_chan.c \
$(ETL_PATH)/server/sv_snapshot.c \
$(ETL_PATH)/server/sv_tracker.c \
$(ETL_PATH)/server/sv_wallhack.c \
$(ETL_PATH)/server/sv_world.c \
$(ETL_PATH)/client/cl_avi.c \
$(ETL_PATH)/client/cl_cgame.c \
$(ETL_PATH)/client/cl_cinematic.c \
$(ETL_PATH)/client/cl_console.c \
$(ETL_PATH)/client/cl_demo.c \
$(ETL_PATH)/client/cl_input.c \
$(ETL_PATH)/client/cl_irc.c \
$(ETL_PATH)/client/cl_keys.c \
$(ETL_PATH)/client/cl_main.c \
$(ETL_PATH)/client/cl_net_chan.c \
$(ETL_PATH)/client/cl_ogv.c \
$(ETL_PATH)/client/cl_parse.c \
$(ETL_PATH)/client/cl_roq.c \
$(ETL_PATH)/client/cl_scrn.c \
$(ETL_PATH)/client/cl_ui.c \
$(ETL_PATH)/client/qal.c \
$(ETL_PATH)/client/snd_adpcm.c \
$(ETL_PATH)/client/snd_codec.c \
$(ETL_PATH)/client/snd_codec_ogg.c \
$(ETL_PATH)/client/snd_codec_wav.c \
$(ETL_PATH)/client/snd_dma.c \
$(ETL_PATH)/client/snd_main.c \
$(ETL_PATH)/client/snd_mem.c \
$(ETL_PATH)/client/snd_mix.c \
$(ETL_PATH)/client/snd_openal.c \
$(ETL_PATH)/client/snd_wavelet.c \
$(ETL_PATH)/botlib/be_interface.c \
$(ETL_PATH)/botlib/l_memory.c \
$(ETL_PATH)/botlib/l_precomp.c \
$(ETL_PATH)/botlib/l_script.c \
$(ETL_PATH)/sys/sys_main.c \
$(ETL_PATH)/sys/con_log.c \
$(ETL_PATH)/sdl/sdl_glimp.c \
$(ETL_PATH)/sdl/sdl_input.c \
$(ETL_PATH)/sdl/sdl_snd.c \
$(ETL_PATH)/qcommon/json_stubs.c \
$(ETL_PATH)/qcommon/dl_main_curl.c \
$(ETL_PATH)/sys/sys_unix.c \
$(ETL_PATH)/sys/con_tty.c


LOCAL_MODULE := etl
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -O3 -Os -mfpu=neon -mfloat-abi=softfp
LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_STATIC_LIBRARIES := curl minizip zlib ogg vorbis ifaddrs renderer_opengl1_armv7-a jpeg
LOCAL_LDLIBS := -lGLESv1_CM

# Add rest of the files too 

include $(BUILD_SHARED_LIBRARY)
