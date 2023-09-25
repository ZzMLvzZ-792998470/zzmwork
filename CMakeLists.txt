cmake_minimum_required(VERSION 3.26)
project(ffmpeg_work)

set(ffmpeg_root "C:/msys64/home/ZZM/ffmpeg-5.0.3/build")
set(opencv_root "C:/msys64/home/ZZM/opencv-4.8.0/build")
#set(CMAKE_C_STANDARD 11)

set(include_path ${ffmpeg_root}/include ${opencv_root}/include)
set(link_path ${ffmpeg_root}/lib ${opencv_root}/lib)
set(src_path main.cpp initer.cpp decoder.cpp encoder.cpp writer.cpp utils.cpp frame_create.cpp transcoder.cpp )
#set(src_path )


set(ffmpeg_dll ${ffmpeg_root}/bin/avcodec-59.dll
        ${ffmpeg_root}/bin/avdevice-59.dll
        ${ffmpeg_root}/bin/avformat-59.dll
        ${ffmpeg_root}/bin/avutil-57.dll
        ${ffmpeg_root}/bin/swscale-6.dll
        ${ffmpeg_root}/bin/postproc-56.dll
        ${ffmpeg_root}/bin/swresample-4.dll  )

set(opencv_dll ${opencv_root}/bin/libopencv_core480.dll
        ${opencv_root}/bin/libopencv_highgui480.dll
        ${opencv_root}/bin/libopencv_imgcodecs480.dll
        ${opencv_root}/bin/libopencv_imgproc480.dll)


set(dll_src ${ffmpeg_dll} ${opencv_dll})
#set(dll_src ${ffmpeg_root}/bin/*.dll ${opencv_dll})
set(ffmpeg_link_target avutil
        avcodec
        avformat
        swscale)

set(opencv_link_target  opencv_core480
        opencv_highgui480
        opencv_imgproc480
        opencv_imgcodecs480)



add_definitions(-w)
include_directories(${ffmpeg_root}/include ${opencv_root}/include)
#include_directories(${include_path})
#link_directories(${link_path})
link_directories(${ffmpeg_root}/lib ${opencv_root}/lib)

FILE(GLOB Plugin_Runtime_Debug_DLL_FILES CONFIGURE_DEPENDS ${dll_src})
FILE(GLOB Plugin_Runtime_Release_DLL_FILES CONFIGURE_DEPENDS ${dll_src})
FILE(GLOB Plugin_Runtime_Debug_Resources_FILES CONFIGURE_DEPENDS ${dll_src})
FILE(GLOB Plugin_Runtime_Release_Resources_FILES CONFIGURE_DEPENDS ${dll_src})
add_custom_target(CopyRuntimeFiles  ALL
        VERBATIM
        COMMAND_EXPAND_LISTS
        COMMAND ${CMAKE_COMMAND} -E
        make_directory "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:.//>/"
        COMMAND ${CMAKE_COMMAND} -E
        copy_if_different
        "$<$<CONFIG:Release>:${Plugin_Runtime_Release_DLL_FILES}>"
        "$<$<CONFIG:Debug>:${Plugin_Runtime_Debug_DLL_FILES}>"
        "${PROJECT_BINARY_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E
        make_directory "${PROJECT_BINARY_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E
        copy_if_different
        "$<$<CONFIG:Release>:${Plugin_Runtime_Release_Resources_FILES}>"
        "$<$<CONFIG:Debug>:${Plugin_Runtime_Debug_Resources_FILES}>"
        "${PROJECT_BINARY_DIR}/")


add_executable(ffmpeg_work ${src_path})
#target_link_libraries(merge_t avutil avcodec avfilter avformat swscale
#        opencv_core480 opencv_highgui480 opencv_imgproc480 opencv_imgcodecs480)
#
target_link_libraries(ffmpeg_work ${ffmpeg_link_target} ${opencv_link_target})
