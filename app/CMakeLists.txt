# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.10)

set(distribution_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs)

include_directories(${CMAKE_SOURCE_DIR}/src/main/cpp/include)

add_library( avcodec-57
        SHARED
        IMPORTED )
set_target_properties( avcodec-57
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavcodec.so )

add_library( avdevice-57
        SHARED
        IMPORTED )
set_target_properties( avdevice-57
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavdevice.so )

add_library( avfilter-6
        SHARED
        IMPORTED)
set_target_properties( avfilter-6
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavfilter.so )

add_library( avformat-57
        SHARED
        IMPORTED)
set_target_properties( avformat-57
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavformat.so )


add_library( avutil-55
        SHARED
        IMPORTED )
set_target_properties( avutil-55
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libavutil.so )


add_library( swresample-2
        SHARED
        IMPORTED )
set_target_properties( swresample-2
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libswresample.so )


add_library( swscale-4
        SHARED
        IMPORTED)
set_target_properties( swscale-4
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libswscale-4.so )

add_library( yuv
        SHARED
        IMPORTED)
set_target_properties( yuv
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/libyuv.so )


add_library( ffmpeg-lib
        SHARED
        ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg-lib.cpp
        ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg/FFmpegPullStream.cpp
        ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg/AVCodecVideoLocalthread.cpp
        ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg/AVCodecAudioLocalthread.cpp
        ${CMAKE_SOURCE_DIR}/src/main/cpp/ffmpeg/VideoDecoder.cpp)

#find_library( log-lib
#              log )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

target_link_libraries(
        ffmpeg-lib
        avcodec-57
        avdevice-57
        avfilter-6
        avformat-57
        avutil-55
        swresample-2
        swscale-4
        OpenSLES
        yuv
        log
        android
)

