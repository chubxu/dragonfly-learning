# 指定CMAKE最低版本
# FATAL_ERROR选项在CMake 2.6+版本会被忽视，当使用CMake 2.4-版本时，应该指定它，出现错误时，应该直接失败而不仅仅是告警。
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# 设置变量：PROJECT_CONTACT=romange@gmail.com
set(PROJECT_CONTACT romange@gmail.com)

# 执行CTest测试
enable_testing()

# 设置变量：CMAKE_EXPORT_COMPILE_COMMANDS=1
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# 设置全局属性：USE_FOLDERS=ON
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 指定当前project的名称，同时自动创建两个变量PROJECT_SOURCE_DIR和PROJECT_NAME
# PROJECT_SOURCE_DIR=当前CMakeLists.txt所在的文件夹路径
# PROJECT_NAME=当前project的名称
project(DRAGONFLY C CXX)

# 设置变量：CMAKE_CXX_STANDARD=17
set(CMAKE_CXX_STANDARD 17)

# We must define all the required variables from the root cmakefile, otherwise
# they just disappear.

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/helio/cmake" ${CMAKE_MODULE_PATH})

# 设置选项：BUILD_SHARED_LIBS=OFF，"Build shared libraries"为help_text
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 设置选项：DF_USE_SSL=ON，"Provide support for SSL connections"为help_text
option(DF_USE_SSL "Provide support for SSL connections" ON)


include(third_party)
include(internal)

# 指定src构建目录
include_directories(src)

# 指定helio构建目录
include_directories(helio)

# 编译helio子文件夹的CMakeLists.txt
add_subdirectory(helio)

# 编译src子文件夹的CMakeLists.txt
add_subdirectory(src)
