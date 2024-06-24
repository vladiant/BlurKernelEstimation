cmake_minimum_required(VERSION 3.22)

project(BlurKernelEstimation)

# Adapted from
# https://gitlab.psi.ch/nemu/musrfit/blob/root6/cmake/FindFFTW3.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(FFTW3 REQUIRED)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)

find_package(OpenMP REQUIRED)

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
         $<$<CXX_COMPILER_ID:Clang>:-fsanitize=address>
)

target_link_libraries(
   ${PROJECT_NAME}
   PRIVATE
      $<$<CXX_COMPILER_ID:Clang>:-fsanitize=address>
      FFTW3::FFTW3
      FFTW3::FFTW3F
    #   FFTW3::FFTW3F_OMP
    #   FFTW3::FFTW3_OMP
      PNG::PNG
      JPEG::JPEG
      TIFF::TIFF
      OpenMP::OpenMP_CXX
)
