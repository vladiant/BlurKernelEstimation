cmake_minimum_required(VERSION 3.22)

project(BlurKernelEstimation)

# Adapted from
# https://gitlab.psi.ch/nemu/musrfit/blob/root6/cmake/FindFFTW3.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(FFTW3 REQUIRED)

# TODO: define FFTW_HAS_THREADS if library found

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_package(OpenMP REQUIRED)
endif()

add_executable(
    ${PROJECT_NAME}
    tvdeconv_20120607/basic.c
    tvdeconv_20120607/basic.h
    tvdeconv_20120607/tvreg.c
    tvdeconv_20120607/tvreg.h
    tvdeconv_20120607/num.h
    angleSet.cpp
    angleSet.hpp
    args.hxx
    computeProjectionsAutocorrelation.cpp
    computeProjectionsAutocorrelation.hpp
    conjugate_gradient.hpp
    deconvBregman.cpp
    deconvBregman.hpp
    estimateKernel.cpp
    estimateKernel.hpp
    fftw_allocator.hpp
    iio.c
    iio.h
    iio.hpp
    image.hpp
    options.hpp
    phaseRetrieval.cpp
    phaseRetrieval.hpp
    projectImage.cpp
    projectImage.hpp
    reconstructPowerspectrum.cpp
    reconstructPowerspectrum.hpp
    main.cpp
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        .
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
         $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
         $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
         # optimize for speed
         $<$<CXX_COMPILER_ID:MSVC>:/Ox /O2>
         $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>
         # target architecture x86
         $<$<CXX_COMPILER_ID:MSVC>:/arch:IA32 /arch:SSE /arch:SSE2 /arch:AVX /arch:AVX2 /arch:AVX512>
         $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-march=native -mtune=native>
)

if(FFTW3_THREAD_LIBRARY)
    add_compile_definitions(FFTW_HAS_THREADS)
endif()

# Required for tvdeconv_20120607
add_compile_definitions(TVREG_DECONV)

target_link_libraries(
   ${PROJECT_NAME}
   PRIVATE
      $<$<NOT:$<CXX_COMPILER_ID:Clang>>:FFTW3::FFTW3_OMP> # use OpenMP only if not clang
      $<$<NOT:$<CXX_COMPILER_ID:Clang>>:FFTW3::FFTW3F_OMP> # use OpenMP only if not clang
      FFTW3::FFTW3
      FFTW3::FFTW3F
      PNG::PNG
      JPEG::JPEG
      TIFF::TIFF
      $<$<NOT:$<CXX_COMPILER_ID:Clang>>:OpenMP::OpenMP_CXX> # use OpenMP only if not clang
)
