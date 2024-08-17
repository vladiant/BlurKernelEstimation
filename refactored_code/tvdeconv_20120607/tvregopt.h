/**
 * @file tvregopt.h
 * @brief tvreg options handling and internal definitions
 * @author Pascal Getreuer <getreuer@gmail.com>
 *
 * Copyright (c) 2010-2012, Pascal Getreuer
 * All rights reserved.
 *
 * This program is free software: you can use, modify and/or
 * redistribute it under the terms of the simplified BSD License. You
 * should have received a copy of this license along this program. If
 * not, see <http://www.opensource.org/licenses/bsd-license.html>.
 */
#pragma once

#include <fftw3.h>
#include <string.h>

#include "tvreg.h"

/** @brief Size of the string buffer for holding the algorithm description */
#define ALGSTRING_SIZE 128

/**
 * @brief  Token concatenation macro
 *
 * This extra level of indirection is needed so that macros expand before
 * token pasting.  The use of this concatenation is for a name mangling
 * macro FFT(S) for using num with the FFTW library.  The macro is defined
 * such that FFT(functionname) expands to
 *    fftwf_functionname  if num is single,
 * or
 *    fftw_functionname   if num is double,
 * according to whether NUM_SINGLE is defined.
 */
#define _TVREG_CONCAT(A, B) A##B

#ifdef NUM_SINGLE
#define FFT(S) _TVREG_CONCAT(fftwf_, S)
#else
#define FFT(S) _TVREG_CONCAT(fftw_, S)
#endif

/* Internal type definitions */

/** @brief 2D vector with num components */
typedef struct {
  num x; /**< x-component */
  num y; /**< y-component */
} numvec2;

/** @brief Complex value type */
typedef num numcomplex[2];

/** @brief Enum of the noise models supported by TvRestore */
typedef enum { NOISEMODEL_L2, NOISEMODEL_L1, NOISEMODEL_POISSON } noisemodel;

/** @brief Options handling for TvRestore */
struct tag_tvregopt {
  num Lambda;
  const num *VaryingLambda;
  int LambdaWidth;
  int LambdaHeight;
  const num *Kernel;
  int KernelWidth;
  int KernelHeight;
  num Tol;
  num Gamma1;
  num Gamma2;
  int MaxIter;
  noisemodel NoiseModel;
  int (*PlotFun)(int, int, num, const num *, int, int, int, void *);
  void *PlotParam;
  char *AlgString;
};

/**
 * @brief TvRestore solver state
 *
 * This struct represents the TvRestore solver state.  It holds all variables
 * and parameters to be passed between TvRestore() and the solver subroutines.
 */
typedef struct tag_tvregsolver {
  num *u;          /**< Current restoration solution       */
  const num *f;    /**< Input image                        */
  numvec2 *d;      /**< Current solution of d              */
  numvec2 *dtilde; /**< Bregman variable for d constraint  */
  num *Ku;         /**< Convolution of kernel with u       */

  num fNorm;               /**< L2 norm of f                       */
  num Alpha;               /**< Lambda/Gamma1 or Gamma2/Gamma1     */
  int Width;               /**< Image width                        */
  int Height;              /**< Image height                       */
  int PadWidth;            /**< Padded image width                 */
  int PadHeight;           /**< Padded image height                */
  int NumChannels;         /**< Number of image channels           */
  struct tag_tvregopt Opt; /**< Solver options                     */
  int UseZ;                /**< True if selected algorithm uses z  */

  num *A, *B;              /**< Spatial FFTW buffers               */
  num *ATrans, *BTrans;    /**< Spectral FFTW buffers              */
  num *DenomTrans;         /**< Precomputation for u subproblem    */
  num *KernelTrans;        /**< Convolution kernel transform       */
  FFT(plan) TransformA;    /**< Forward transform plan A -> ATrans */
  FFT(plan) TransformB;    /**< Forward transform plan B -> BTrans */
  FFT(plan) InvTransformA; /**< Inverse transform plan ATrans -> A */
  FFT(plan) InvTransformB; /**< Inverse transform plan BTrans -> B */
} tvregsolver;

typedef num (*usolver)(tvregsolver *);
typedef void (*zsolver)(tvregsolver *);

/* If GNU C language extensions are available, apply the "unused" attribute
   to avoid warnings.  TvRestoreSimplePlot is a plotting callback function
   for TvRestore, so the unused arguments are indeed required. */
static int TvRestoreSimplePlot(int State, int Iter, num Delta,
                               ATTRIBUTE_UNUSED const num *u,
                               ATTRIBUTE_UNUSED int Width,
                               ATTRIBUTE_UNUSED int Height,
                               ATTRIBUTE_UNUSED int NumChannels,
                               ATTRIBUTE_UNUSED void *Param) {
  switch (State) {
    case 0: /* TvRestore is running */
      /* We print to stderr so that messages are displayed on the console
         immediately, during the TvRestore computation.  If we use stdout,
         messages might be buffered and not displayed until after TvRestore
         completes, which would defeat the point of having this real-time
         plot callback. */
      fprintf(stderr, "   Iteration %4d     Delta %7.4f\r", Iter, Delta);
      break;
    case 1: /* Converged successfully */
      fprintf(stderr, "Converged in %d iterations.           \n", Iter);
      break;
    case 2: /* Maximum iterations exceeded */
      fprintf(stderr, "Maximum number of iterations exceeded.\n");
      break;
  }
  return 1;
}

/** @brief Default options struct */
static tvregopt TvRegDefaultOpt = {TVREGOPT_DEFAULT_LAMBDA,
                                   NULL,
                                   0,
                                   0,
                                   NULL,
                                   0,
                                   0,
                                   (num)(TVREGOPT_DEFAULT_TOL),
                                   TVREGOPT_DEFAULT_GAMMA1,
                                   TVREGOPT_DEFAULT_GAMMA2,
                                   TVREGOPT_DEFAULT_MAXITER,
                                   NOISEMODEL_L2,
                                   TvRestoreSimplePlot,
                                   NULL,
                                   NULL};

// TODO: Test replace static with inline
// TODO: Remove unused functions

/**
 * @brief Create a new tvregopt options object
 * @return tvregopt pointer, or NULL if out of memory
 *
 * This routine creates a new tvregopt options object and initializes it to
 * default values.  It is the caller's responsibility to call TvRegFreeOpt()
 * to free the tvregopt object when done.
 */
static tvregopt *TvRegNewOpt() {
  tvregopt *Opt;

  if ((Opt = (tvregopt *)Malloc(sizeof(tvregopt)))) *Opt = TvRegDefaultOpt;

  if (!(Opt->AlgString = (char *)Malloc(sizeof(char) * ALGSTRING_SIZE))) {
    Free(Opt);
    return NULL;
  }

  return Opt;
}

/**
 * @brief Free tvregopt options object
 * @param Opt tvregopt options object
 */
static void TvRegFreeOpt(tvregopt *Opt) {
  if (Opt) {
    if (Opt->AlgString) Free(Opt->AlgString);
    Free(Opt);
  }
}

/**
 * @brief Specify fidelity weight lambda
 * @param Opt tvregopt options object
 * @param Lambda fidelity weight (positive scalar)
 */
static void TvRegSetLambda(tvregopt *Opt, num Lambda) {
  if (Opt) Opt->Lambda = Lambda;
}

/**
 * @brief Specify kernel for a deconvolution problem
 * @param Opt tvregopt options object
 * @param Kernel pointer to convolution kernel
 * @param KernelWidth, KernelHeight dimensions of the kernel
 *
 * Kernel should be a contiguous array of size KernelWidth by KernelHeight
 * in row-major order,
 *    Kernel[x + KernelWidth*y] = K(x,y).
 * If Kernel = NULL, then no deconvolution is performed.
 */
static void TvRegSetKernel(tvregopt *Opt, const num *Kernel, int KernelWidth,
                           int KernelHeight) {
  if (Opt) {
    Opt->Kernel = Kernel;
    Opt->KernelWidth = KernelWidth;
    Opt->KernelHeight = KernelHeight;
  }
}

/**
 * @brief Specify convergence tolerance
 * @param Opt tvregopt options object
 * @param Tol convergence tolerance (positive scalar)
 */
static void TvRegSetTol(tvregopt *Opt, num Tol) {
  if (Opt) Opt->Tol = Tol;
}

/**
 * @brief Specify d = grad u penalty weight
 * @param Opt tvregopt options object
 * @param Gamma1 penalty (positive scalar)
 */
static void TvRegSetGamma1(tvregopt *Opt, num Gamma1) {
  if (Opt) Opt->Gamma1 = Gamma1;
}

/**
 * @brief Specify the maximum number of iterations
 * @param Opt tvregopt options object
 * @param MaxIter maximum number of iterations
 */
static void TvRegSetMaxIter(tvregopt *Opt, int MaxIter) {
  if (Opt) Opt->MaxIter = MaxIter;
}

/**
 * @brief Specify plotting function
 * @param Opt tvregopt options object
 * @param PlotFun plotting function
 * @param PlotParam void pointer for passing addition parameters
 *
 * Specifying the plotting function gives control over how TvRestore displays
 * information.  Setting PlotFun = NULL disables all normal display (error
 * messages are still displayed).
 *
 * An example PlotFun is
@code
    int ExamplePlotFun(int State, int Iter, num Delta,
        const num *u, int Width, int Height, int NumChannels, void *PlotParam)
    {
        switch(State)
        {
        case 0:
            fprintf(stderr, " RUNNING   Iter=%4d, Delta=%7.4f\r", Iter, Delta);
            break;
        case 1:
            fprintf(stderr, " CONVERGED Iter=%4d, Delta=%7.4f\n", Iter, Delta);
            break;
        case 2:
            fprintf(stderr, " Maximum number of iterations exceeded!\n");
            break;
        }
        return 1;
    }
@endcode
 * The State argument is either 0, 1, or 2, and indicates TvRestore's status.
 * Iter is the number of Bregman iterations completed, Delta is the change in
 * the solution Delta = ||u^cur - u^prev||_2 / ||f||_2.  Argument u gives a
 * pointer to the current solution, which can be used to plot an animated
 * display of the solution progress.  PlotParam is a void pointer that can be
 * used to pass additional information to PlotFun if needed.
 */
static void TvRegSetPlotFun(tvregopt *Opt,
                            int (*PlotFun)(int, int, num, const num *, int, int,
                                           int, void *),
                            void *PlotParam) {
  if (Opt) {
    Opt->PlotFun = PlotFun;
    Opt->PlotParam = PlotParam;
  }
}
