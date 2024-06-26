#ifndef SHEAR_IMAGE_H
#define SHEAR_IMAGE_H

#include <vector>

#include "angleSet.hpp"
#include "image.hpp"

template <typename T>
void projectImage(img_t<T>& projections, const img_t<T>& imgX,
                  const img_t<T>& imgY, const std::vector<angle_t>& angleSet);

template <typename T>
void projectImage(img_t<T>& projections, const img_t<T>& img,
                  const std::vector<angle_t>& angleSet);

#include "projectImage.cpp"

#endif
