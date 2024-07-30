#pragma once

#include "angleSet.hpp"
#include "image.hpp"

template <typename T>
void computeProjectionsAutocorrelation(img_t<T>& acProjections,
                                       const img_t<T>& imgBlur,
                                       const std::vector<angle_t>& angleSet,
                                       int psSize, T compensationFactor);

// TODO: Fix
#include "computeProjectionsAutocorrelation.cpp"
