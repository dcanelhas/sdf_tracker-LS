cmake_minimum_required(VERSION 2.4.6)
SET(PROJECT_NAME sdf_tracker)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0053 OLD)
endif(COMMAND cmake_policy)

#Workaround for GCC / G++ 5.3 and CUDA 
if( (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC) AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.4)
  message(FATAL_ERROR "Current CUDA versions do not support GCC > 5.4. Compilation is likely to fail. TIP: Your distro may have package compatible compilers with CUDA, if so they may be enabled by the following environment variables: \nexport CC=/opt/cuda/bin/gcc\nexport CXX=/opt/cuda/bin/g++ ")
endif()

SET(CMAKE_BUILD_TYPE Release)

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# check for SSE flags
include(${PROJECT_SOURCE_DIR}/CMakeModules/Find_SSE.cmake)
CHECK_FOR_SSE()
message(STATUS "SSE instructions supported and enabled: ${SSE_FLAGS}")

find_package(CUDA REQUIRED)
LIST(APPEND CMAKE_CXX_FLAGS " -fopenmp -g -std=c++11 -O3 -Wall -DLINUX_ -DOC_NEW_STYLE_INCLUDES ")
set(CUDA_PROPAGATE_HOST_FLAGS off)

LIST(APPEND CUDA_NVCC_FLAGS --compiler-options -lineinfo)
# LIST(APPEND CUDA_NVCC_FLAGS -gencode arch=compute_20,code=sm_20)
LIST(APPEND CUDA_NVCC_FLAGS -gencode arch=compute_30,code=sm_30)
set(CUDA_VERBOSE_BUILD ON CACHE BOOL "nvcc verbose" FORCE)
CUDA_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
CUDA_INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
SET(LIB_TYPE STATIC) #set the lib type


set( pca_dictionary_path ${CMAKE_CURRENT_SOURCE_DIR}/pca_dictionaries )
set( nn_definitions_path ${CMAKE_CURRENT_SOURCE_DIR}/network_definitions )
configure_file(config.h.in config.h @ONLY)


find_package(Eigen3 REQUIRED)
find_package(OpenNI2 REQUIRED)
find_package(Boost 1.46 COMPONENTS system REQUIRED) #Set your own version
find_package(X11 REQUIRED)

set(PTOOLS_PATH ${PROJECT_SOURCE_DIR}/deps/PicklingTools170Release)
include_directories(${PTOOLS_PATH}/C++)
include_directories(${PTOOLS_PATH}/C++/opencontainers_1_8_5/include)

add_library(ptools SHARED 
${PTOOLS_PATH}/C++/m2pythontools
${PTOOLS_PATH}/C++/valpython
${PTOOLS_PATH}/C++/midassocket
${PTOOLS_PATH}/C++/valprotocol2
${PTOOLS_PATH}/C++/m2ser
${PTOOLS_PATH}/C++/m2streamdataenc
${PTOOLS_PATH}/C++/m2convertrep
${PTOOLS_PATH}/C++/timeconv
${PTOOLS_PATH}/C++/fdtools)

include_directories(${PTOOLS_PATH}/C++/opencontainers_1_7_6/include)
include_directories(${PTOOLS_PATH}/C++)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${X11_INCLUDE_DIR})
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIR})
# include_directories(${VTK_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${OPENNI2_INCLUDE_DIR})
include_directories(./)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_directories(${OPENNI2_LIBRARIES})
link_directories(${Boost_LIBRARY_DIRS})
link_directories(${PTOOLS_PATH}/C++)


###############################################################################

CUDA_ADD_LIBRARY(vector_functions ${LIB_TYPE} src/vector_functions.cu)

###############################################################################

CUDA_ADD_LIBRARY(principal_components ${LIB_TYPE} src/principal_components.cu )
CUDA_ADD_CUBLAS_TO_TARGET(principal_components)
target_link_libraries(principal_components ptools vector_functions)

###############################################################################

CUDA_ADD_LIBRARY(neural_network ${LIB_TYPE} src/neural_network.cu )
CUDA_ADD_CUBLAS_TO_TARGET(neural_network)
target_link_libraries(neural_network ptools vector_functions)

###############################################################################
add_library(${PROJECT_NAME} src/sdf_tracker.cpp include/sdf_tracker.h)

target_link_libraries(${PROJECT_NAME}
  ${X11_LIBRARIES}
  # ${VTK_LIBRARIES}
  ${Boost_LIBRARIES}
  principal_components
  neural_network
  vector_functions
)

###############################################################################

add_executable(
  sdf_tracker_app
  src/tracker_app.cpp)

SET_TARGET_PROPERTIES(sdf_tracker_app PROPERTIES INSTALL_RPATH "${OPENNI2_INCLUDE_DIR}/Redist/")
target_link_libraries(sdf_tracker_app
  ${PROJECT_NAME}
  ${OPENNI2_LIBRARIES}
)