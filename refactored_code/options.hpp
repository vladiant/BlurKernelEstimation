#pragma once

#include <string>

// TODO: Set it as a proper type name
using flt = double;

struct options {
  std::string input;
  int kernelSize;
  std::string out_kernel;
  std::string out_deconv;

  int Ninner;
  int Ntries;
  int Nouter;
  flt compensationFactor;
  int medianFilter;

  flt finalDeconvolutionWeight;
  flt intermediateDeconvolutionWeight;
  int seed;
};
