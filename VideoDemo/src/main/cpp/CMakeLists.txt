cmake_minimum_required(VERSION 3.10.2)

project("testffmpegdemo")

# 全部cpp文件都需要编译
file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/*.cpp)
# 导入头文件，编译后头文件不会在目标库中
include_directories("${CMAKE_SOURCE_DIR}/include")
# 导入第三方库
set(MY_LIBS_PATH ${CMAKE_SOURCE_DIR}/../../../libs/${CMAKE_ANDROID_ARCH_ABI})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L ${MY_LIBS_PATH}")

# 配置库编译后的名字
add_library(
        zffmpeg
        SHARED
        ${SOURCE}
)

find_library(
        log-lib
        log
)

# 将三方库编译到我们的库一起
target_link_libraries(
        zffmpeg
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
        ${log-lib}
        z
        OpenSLES
        android
)