# set core files
file(GLOB OPENVINO_CORE_FILES inference_detection_openvino.hpp inference_detection_openvino.cpp)
file(GLOB MODEL_CORE_FILES inference_detection_model.hpp inference_detection_model.cpp)
file(GLOB MOSAIC_API_CORE_FILES RMAI_MOSAIC_API.h RMAI_MOSAIC_API.cpp)

# rmai_mosaic dll library
add_library(rmai_mosaic SHARED ${OPENVINO_CORE_FILES} ${MOSAIC_API_CORE_FILES} ${MODEL_CORE_FILES})
target_link_libraries(rmai_mosaic ${OpenCV_LIBS} )
target_link_libraries(rmai_mosaic optimized ${OpenVINO_LIBRARY_DIRS_RELEASE})
if (WIN32)
    target_link_libraries(rmai_mosaic debug ${OpenVINO_LIBRARY_DIRS_DEBUG})
endif(WIN32)
target_link_libraries(rmai_mosaic glog::glog)

# add test
add_executable(test_detection_picture test_detection_picture.cpp)
target_link_libraries(test_detection_picture rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_detection_picture debug ${COMMON_UTILITY_LIBRARY_DEBUG} optimized ${COMMON_UTILITY_LIBRARY_RELEASE})
target_link_libraries(test_detection_picture glog::glog)

add_executable(test_detection_picture_thread test_detection_picture_thread.cpp)
target_link_libraries(test_detection_picture_thread rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_detection_picture_thread debug ${COMMON_UTILITY_LIBRARY_DEBUG} optimized ${COMMON_UTILITY_LIBRARY_RELEASE})
target_link_libraries(test_detection_picture_thread glog::glog)

add_executable(test_detection_picture_thread_init test_detection_picture_thread_init.cpp)
target_link_libraries(test_detection_picture_thread_init rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_detection_picture_thread_init debug ${COMMON_UTILITY_LIBRARY_DEBUG} optimized ${COMMON_UTILITY_LIBRARY_RELEASE})
target_link_libraries(test_detection_picture_thread_init glog::glog)

add_executable(test_detection_picture_opencvlap test_detection_picture_opencvlap.cpp)
target_link_libraries(test_detection_picture_opencvlap ${OpenCV_LIBS})
target_link_libraries(test_detection_picture_opencvlap debug ${COMMON_UTILITY_LIBRARY_DEBUG} optimized ${COMMON_UTILITY_LIBRARY_RELEASE})
target_link_libraries(test_detection_picture_opencvlap glog::glog)

add_executable(test_detection_yuv test_detection_yuv.cpp)
target_link_libraries(test_detection_yuv rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_detection_yuv glog::glog)

add_executable(test_detection_avi test_detection_avi.cpp)
target_link_libraries(test_detection_avi rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_detection_avi glog::glog)

add_executable(test_mosaic_api test_mosaic_api.cpp)
target_link_libraries(test_mosaic_api rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_mosaic_api glog::glog)

add_executable(test_mosaic_api_thread test_mosaic_api_thread.cpp)
target_link_libraries(test_mosaic_api_thread rmai_mosaic ${OpenCV_LIBS})
target_link_libraries(test_mosaic_api_thread glog::glog)