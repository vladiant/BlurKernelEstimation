#pragma once

#include "image.hpp"
#include "options.hpp"

template <typename T>
void estimateKernel(img_t<T>& kernel, const img_t<T>& img, int kernelSize,
                    const options& opts);

// TODO: Fix
#include "estimateKernel.cpp"
