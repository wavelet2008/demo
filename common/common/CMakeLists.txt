cmake_minimum_required(VERSION 3.4)

# suppress warning
cmake_policy(SET CMP0012 NEW)

# suppress windows warning from strcpy
if (WIN32)
  add_definitions("/D_CRT_SECURE_NO_WARNINGS /wd4996")
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif(WIN32)

project(common)

# 1) add c++ 11 support --std=c++11
# 2) to fix compilation problem: relocation R_X86_64_32 against
#    `.rodata' can not be used when making a shared object;
#    recompile with -fPIC
if (UNIX OR APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
endif(UNIX OR APPLE)

# add gflag
find_package(gflags REQUIRED)
if(NOT gflags_FOUND)
message(FATAL_ERROR "gflags not found. Please build gflags under md/3rd_party/gflags and set gflags_DIR")
endif(NOT gflags_FOUND)

# add glog
find_package(glog REQUIRED)
if(NOT glog_FOUND)
 message(FATAL_ERROR "glog not found. Please build glog under md/3rd_party/glog and set glog_DIR")
endif(NOT glog_FOUND)

# find OpenCV library
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
  message(STATUS "Found OpenCV Library: " ${OPENCV_INCLUDE_DIRS})
  include_directories(${OPENCV_INCLUDE_DIRS})
else (OpenCV_FOUND)
  message(FATAL_ERROR "OpenCV not found.")
endif()

# add root directory in include path
get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
include_directories(${PARENT_DIR})

# set default build type
set(CMAKE_BUILD_TYPE Release CACHE STRING "Set default build type to release")

# set default debug and release output directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib/Release)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib/Debug)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/Dependency)

# enable testing
enable_testing()

# add submodules
add_subdirectory(utils)
add_subdirectory(test)
