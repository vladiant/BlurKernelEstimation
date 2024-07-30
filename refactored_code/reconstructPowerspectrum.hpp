#pragma once

#include "angleSet.hpp"
#include "image.hpp"

template <typename T>
void reconstructPowerspectrum(img_t<T>& powerSpectrum,
                              const img_t<T> acProjections,
                              const std::vector<angle_t>& angleSet, int psSize);

// TODO: Fix
#include "reconstructPowerspectrum.cpp"
