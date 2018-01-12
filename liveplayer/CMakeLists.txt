# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)
set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../src/main/jniLibs)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
# SDL核心库(最重要的库)
add_library(
            SDL2
            SHARED
            IMPORTED)
set_target_properties(
            SDL2
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libSDL2.so)
add_library(
            SDL2_image
            SHARED
            IMPORTED)
set_target_properties(
            SDL2_image
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libSDL2_image.so)
add_library(
            SDL2_mixer
            SHARED
            IMPORTED)
set_target_properties(
            SDL2_mixer
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libSDL2_mixer.so)

# FFmpeg配置
    # 编解码(最重要的库)
    add_library(avcodec-57
                SHARED
                IMPORTED)
    set_target_properties(
                avcodec-57
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavcodec-57.so)
    # 滤镜特效处理库
    add_library(avfilter-6
                SHARED
                IMPORTED)
    set_target_properties(avfilter-6
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavfilter-6.so)

    # 封装格式处理库
    add_library(avformat-57
                SHARED
                IMPORTED)
    set_target_properties(
                avformat-57
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavformat-57.so)

   add_library(avresample-3
                SHARED
                IMPORTED)
    set_target_properties(
                avresample-3
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavresample-3.so)
    # 工具库(大部分库都需要这个库的支持)
    add_library(avutil-55
                SHARED
                IMPORTED)
    set_target_properties(avutil-55
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavutil-55.so)
    add_library(postproc-54
                   SHARED
                   IMPORTED)
       set_target_properties(
                   postproc-54
                   PROPERTIES IMPORTED_LOCATION
                   ../../../../src/main/jniLibs/armeabi/libpostproc-54.so)

    # 音频采样数据格式转换库
    add_library(swresample-2
                SHARED
                IMPORTED)
    set_target_properties(swresample-2
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libswresample-2.so)

    # 视频像素数据格式转换
    add_library(swscale-4
                SHARED
                IMPORTED)
    set_target_properties(swscale-4
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libswscale-4.so)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(src/main/jniLibs/src)
include_directories(src/main/jniLibs/include)
include_directories(src/main/cpp)
add_library( SDL2main
             SHARED
             src/main/jniLibs/src/main/android/SDL_android_main.c
             src/main/cpp/SamychenPush.c
             #src/main/cpp/SDLTest.cpp
             #src/main/cpp/sdlplayer/SDL_Lesson.c
             src/main/cpp/sdlplayer/SDL_Lesson2.c
             #src/main/cpp/sdlplayer/SyncVideo.c
             #src/main/cpp/sdlplayer/SyncAudio.c
             #src/main/cpp/sdlplayer/tutorial06.c
             #src/main/cpp/mix_player.cpp
             src/main/cpp/ANativeplayer/ANativeplayer.cpp
             src/main/cpp/ANativeplayer/Audio2.cpp
             src/main/cpp/ANativeplayer/Video2.cpp
             src/main/cpp/ANativeplayer/native-lib.cpp
             )
find_library( log-lib log )
#注意必须是SDL2main在SDL2前,注意SDL的顺序
target_link_libraries(SDL2main SDL2 SDL2_image SDL2_mixer
avcodec-57 avfilter-6 avformat-57 avutil-55 postproc-54 swresample-2 swscale-4
                       ${log-lib} OpenSLES  z log android)
