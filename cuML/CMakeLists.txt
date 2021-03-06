#
# Copyright (c) 2018, NVIDIA CORPORATION.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(cuML VERSION 0.4.0 LANGUAGES CXX CUDA)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###################################################################################################
# - Requirements ----------------------------------------------------------------------------------

find_package(CUDA 9.0 REQUIRED)
if (NOT DISABLE_OPENMP OR NOT ${DISABLE_OPENMP})
    find_package(OpenMP)
endif(NOT DISABLE_OPENMP OR NOT ${DISABLE_OPENMP})

###################################################################################################
# - Submodules ------------------------------------------------------------------------------------

set(GTEST_DIR ${PROJECT_SOURCE_DIR}/external/ml-prims/external/googletest CACHE STRING
  "Path to the googletest repo")
set(GTEST_LIBNAME "gtest_main" CACHE STRING
  "Name of the googletest library")
set(MLPRIMS_DIR ${PROJECT_SOURCE_DIR}/external/ml-prims CACHE STRING
  "Path to the ml-prims repo")
set(CUB_DIR ${PROJECT_SOURCE_DIR}/external/ml-prims/external/cub CACHE STRING
  "Path to cub repo")
set(CUTLASS_DIR ${PROJECT_SOURCE_DIR}/external/ml-prims/external/cutlass CACHE STRING
  "Path to the cutlass repo")

###################################################################################################
# - User Options  ---------------------------------------------------------------------------------

set(GPU_ARCHS "" CACHE STRING
  "List of GPU architectures (semicolon-separated) to be compiled for")
option(LINEINFO "Enable lineinfo in nvcc" OFF)
option(KERNELINFO "Enable kernel resource usage info" OFF)
option(DEBUG "Get a debug build" OFF)

###################################################################################################
# - NVCC Options  ---------------------------------------------------------------------------------

if(OPENMP_FOUND)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler ${OpenMP_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif(OPENMP_FOUND)
if(CMAKE_CXX_STANDARD STREQUAL "11")
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-extended-lambda")
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --std=c++11")
endif()
if(LINEINFO)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo")
endif()
if(KERNELINFO)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xptxas=-v")
endif()
if(DEBUG)
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -G -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()
# Generate optimized binary for every known arch
if(NOT "${GPU_ARCHS}")
  set(GPU_ARCHS "60;61")
  # NOTE: NOTE: Add more 'if's for every new arch release!
  if((CUDA_VERSION_MAJOR EQUAL 9) OR (CUDA_VERSION_MAJOR GREATER 9))
    set(GPU_ARCHS "${GPU_ARCHS};70")
  endif()
  if((CUDA_VERSION_MAJOR EQUAL 10) OR (CUDA_VERSION_MAJOR GREATER 10))
    set(GPU_ARCHS "${GPU_ARCHS};75")
  endif()
endif()
foreach(arch ${GPU_ARCHS})
  set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_${arch},code=sm_${arch}")
endforeach()
# Generate PTX (to be JIT'd at runtime) for the latest architecture
# It is assumed that the last arch in the 'archs' is the latest!
list(GET GPU_ARCHS -1 ptx)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_${ptx},code=compute_${ptx}")
## end nvcc options

###################################################################################################
# - include paths ---------------------------------------------------------------------------------

include_directories(src
                    ${GTEST_DIR}/googletest/include
                    ${MLPRIMS_DIR}/src
                    ${MLPRIMS_DIR}/src/dbscan
                    ${MLPRIMS_DIR}/test
                    ${CUTLASS_DIR}
                    ${CUB_DIR})

add_subdirectory(${GTEST_DIR}/googletest ${PROJECT_BINARY_DIR}/googletest)

# Append source file in recursive manner, append header files to target for work with them in IDE
file(GLOB_RECURSE ml_prims_header "${MLPRIMS_DIR}/src/*.h" "${MLPRIMS_DIR}/src/*.hpp")
file(GLOB_RECURSE cuml_test_cuda_sources "test/*.cu")

###################################################################################################
# - build libcuml shared library ------------------------------------------------------------------

add_library(cuml SHARED
            src/pca/pca.cu
            src/tsvd/tsvd.cu
            src/dbscan/dbscan.cu
            src/kmeans/kmeans.cu
            src/glm/glm.cu
            )

set(CUML_LINK_LIBRARIES ${CUDA_cublas_LIBRARY} ${CUDA_curand_LIBRARY} ${CUDA_cusolver_LIBRARY} ${CUDA_CUDART_LIBRARY} z)
if(OPENMP_FOUND)
    set(CUML_LINK_LIBRARIES ${CUML_LINK_LIBRARIES} OpenMP::OpenMP_CXX pthread)
endif(OPENMP_FOUND)

target_link_libraries(cuml ${CUML_LINK_LIBRARIES})

###################################################################################################
# - build test executable -------------------------------------------------------------------------

add_executable(ml_test ${cuml_test_cuda_sources} ${ml_prims_header})

target_link_libraries(ml_test
                      ${GTEST_LIBNAME}
                      ${CUDA_cublas_LIBRARY}
                      ${CUDA_curand_LIBRARY}
                      ${CUDA_cusolver_LIBRARY}
                      ${CUDA_CUDART_LIBRARY}
                      pthread
                      z)


###################################################################################################
# - install targets -------------------------------------------------------------------------------

install(TARGETS cuml
        DESTINATION lib)
