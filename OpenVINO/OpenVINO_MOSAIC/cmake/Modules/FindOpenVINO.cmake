# - Try to find OpenVINO
#
# The following variables are optionally searched for defaults
#  OpenVINO_DIR:            Base directory where all OpenVINO components are found
#
# The following are set after configuration is done:
#  OpenVINO_FOUND
#  OpenVINO_INCLUDE_DIRS
#  OpenVINO_LIBRARIES
#  OpenVINO_LIBRARY_DIRS

include(FindPackageHandleStandardArgs)

set(OpenVINO_DIR "/home/huanyuan/code/intel/openvino_2020.4.287" CACHE PATH "Folder contains package OpenVINO")

find_path(OpenVINO_INCLUDE_DIR inference_engine.hpp
  HINTS ${OpenVINO_DIR}
  PATH_SUFFIXES deployment_tools/inference_engine/include)

find_library(OpenVINO_LIBRARY_RELEASE inference_engine
  HINTS ${OpenVINO_DIR} 
  PATH_SUFFIXES deployment_tools/inference_engine/lib/intel64 deployment_tools/inference_engine/lib/intel64/Release)

find_library(OpenVINO_legacy_LIBRARY_RELEASE inference_engine_legacy
  HINTS ${OpenVINO_DIR} 
  PATH_SUFFIXES deployment_tools/inference_engine/lib/intel64 deployment_tools/inference_engine/lib/intel64/Release)

find_package_handle_standard_args(
  OpenVINO DEFAULT_MSG OpenVINO_INCLUDE_DIR OpenVINO_LIBRARY_RELEASE OpenVINO_legacy_LIBRARY_RELEASE)

if (WIN32)
  find_library(OpenVINO_LIBRARY_DEBUG inference_engined
    HINTS ${OpenVINO_DIR} 
    PATH_SUFFIXES deployment_tools/inference_engine/lib/intel64 deployment_tools/inference_engine/lib/intel64/Debug)

  find_library(OpenVINO_legacy_LIBRARY_DEBUG inference_engine_legacyd
    HINTS ${OpenVINO_DIR} 
    PATH_SUFFIXES deployment_tools/inference_engine/lib/intel64 deployment_tools/inference_engine/lib/intel64/Debug)

  find_package_handle_standard_args(
      OpenVINO OpenVINO_LIBRARY_DEBUG OpenVINO_legacy_LIBRARY_DEBUG)
endif(WIN32)

if(OpenVINO_FOUND)
  set(OpenVINO_INCLUDE_DIRS ${OpenVINO_INCLUDE_DIR} )
  set(OpenVINO_LIBRARY_DIRS_RELEASE ${OpenVINO_LIBRARY_RELEASE} ${OpenVINO_legacy_LIBRARY_RELEASE})
  if (WIN32)
    set(OpenVINO_LIBRARY_DIRS_DEBUG ${OpenVINO_LIBRARY_DEBUG} ${OpenVINO_legacy_LIBRARY_DEBUG})
  endif(WIN32)
  include_directories(${OpenVINO_INCLUDE_DIRS})
endif()

# include(FindPackageHandleStandardArgs)

# set(OpenVINO_DIR "/home/huanyuan/code/openvino" CACHE PATH "Folder contains package OpenVINO")

# find_path(OpenVINO_INCLUDE_DIR inference_engine.hpp
#   HINTS ${OpenVINO_DIR}
#   PATH_SUFFIXES build/install/deployment_tools/inference_engine/include)

# find_library(OpenVINO_LIBRARY inference_engine
#   HINTS ${OpenVINO_DIR} 
#   PATH_SUFFIXES bin/intel64/Release/lib)

# find_library(OpenVINO_legacy_LIBRARY inference_engine_legacy
#   HINTS ${OpenVINO_DIR} 
#   PATH_SUFFIXES bin/intel64/Release/lib)

# find_package_handle_standard_args(
#   OpenVINO DEFAULT_MSG OpenVINO_INCLUDE_DIR OpenVINO_LIBRARY OpenVINO_legacy_LIBRARY)

# if(OpenVINO_FOUND)
#   set(OpenVINO_INCLUDE_DIRS ${OpenVINO_INCLUDE_DIR} )
#   set(OpenVINO_LIBRARY_DIRS ${OpenVINO_LIBRARY} ${OpenVINO_legacy_LIBRARY} )
#   include_directories(${OpenVINO_INCLUDE_DIRS})
# endif()