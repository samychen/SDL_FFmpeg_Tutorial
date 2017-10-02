# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)


# SDL核心库(最重要的库)
add_library(
            SDL2
            SHARED
            IMPORTED)
set_target_properties(
            SDL2
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libSDL2.so)



# FFmpeg配置
    # 编解码(最重要的库)
    add_library(
                avcodec-57
                SHARED
                IMPORTED)
    set_target_properties(
                avcodec-57
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavcodec-57.so)


    # 滤镜特效处理库
    add_library(
                avfilter-6
                SHARED
                IMPORTED)
    set_target_properties(
                avfilter-6
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavfilter-6.so)

    # 封装格式处理库
    add_library(
                avformat-57
                SHARED
                IMPORTED)
    set_target_properties(
                avformat-57
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavformat-57.so)

    # 工具库(大部分库都需要这个库的支持)
    add_library(
                avutil-55
                SHARED
                IMPORTED)
    set_target_properties(
                avutil-55
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libavutil-55.so)

    # 音频采样数据格式转换库
    add_library(
                swresample-2
                SHARED
                IMPORTED)
    set_target_properties(
                swresample-2
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libswresample-2.so)

    # 视频像素数据格式转换
    add_library(
                swscale-4
                SHARED
                IMPORTED)
    set_target_properties(
                swscale-4
                PROPERTIES IMPORTED_LOCATION
                ../../../../src/main/jniLibs/armeabi/libswscale-4.so)



if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(src/main/jniLibs/src)
include_directories(src/main/jniLibs/include)


# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             SDL2main

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             # Associated headers in the same location as their source
             # file are automatically included.
             #配置SDL程序入口函数
             src/main/jniLibs/src/main/android/SDL_android_main.c
             src/main/cpp/SDLTest.cpp )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because system libraries are included in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in the
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       SDL2main SDL2 avcodec-57 avfilter-6 avformat-57 avutil-55 swresample-2 swscale-4

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
