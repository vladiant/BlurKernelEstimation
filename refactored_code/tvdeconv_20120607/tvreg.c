/**
 * @file tvreg.c
 * @brief TV-regularized image restoration
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tvregopt.h"

/**
 * @brief Total variation based image restoration
 * @param u initial guess, overwritten with restored image
 * @param f input image
 * @param Width, Height, NumChannels dimensions of the input image
 * @param Opt tvregopt options object
 * @return 0 on failure, 1 on success, 2 on maximum iterations exceeded
 *
 * This routine implements simultaneous denoising, deconvolution, and
 * inpainting with total variation (TV) regularization, using either the
 * Gaussian (L2), Laplace (L1), or Poisson noise model, such that Kernel*u is
 * approximately f outside of the inpainting domain.
 *
 * The input image f should be a contiguous array of size Width by Height by
 * NumChannels in planar row-major order,
 *    f[x + Width*(y + Height*k)] = kth component of pixel (x,y).
 *
 * The image intensity values of f should be scaled so that the maximum
 * intensity range of the true clean image is from 0 to 1.  It is allowed that
 * f have values outside of [0,1] (as spurious noisy pixels can exceed this
 * range), but it should be scaled so that the restored image is in [0,1].
 * This scaling is especially important for the Poisson noise model.
 *
 * Typically, NumChannels is either 1 (grayscale image) or 3 (color image),
 * but NumChannels is allowed to be any positive integer.  If NumChannels > 1,
 * then vectorial TV (VTV) regularization is used in place of TV.
 *
 * Image u is both an input and output of the routine.  Image u should be
 * set by the caller to an initial guess, for example a good generic
 * initialization is to set u as a copy of f.  Image u is overwritten with the
 * restored image.
 *
 * Other options are specified through the options object Opt.  First use
 *    tvregopt Opt = TvRegNewOpt()
 * to create a new options object with default options (denoising with the
 * Gaussian noise model).  Then use the following functions to make settings.
 *
 *    - TvRegSetLambda():         set fidelity weight
 *    - TvRegSetKernel():         Kernel for deconvolution problems
 *    - TvRegSetTol():            convergence tolerance
 *    - TvRegSetMaxIter():        maximum number of iterations
 *    - TvRegSetGamma1():         constraint weight on d = grad u
 *    - TvRegSetPlotFun():        custom plotting function
 *
 * When done, call TvRegFreeOpt() to free the options object.  Setting
 * Opt = NULL uses the default options (denoising with Gaussian noise model).
 *
 * The split Bregman method is used to solve the minimization,
 *    T. Goldstein and S. Osher,  "The Split Bregman Algorithm for L1
 *    Regularized Problems", UCLA CAM Report 08-29.
 *
 * The routine automatically adapts the algorithm according to the inputs.  If
 * no deconvolution is needed, Gauss-Seidel is used to solve the u-subproblem.
 * If the kernel is symmetric, a DCT-based solver is applied and, if not, a
 * (slower) Fourier-based solver.  For the Gaussian noise model, the routine
 * uses a simpler splitting of the problem with two auxiliary variables.  For
 * non-Gaussian noise models, a splitting with three auxiliary variables is
 * applied.
 */
int TvRestore(num *u, const num *f, int Width, int Height, int NumChannels,
              tvregopt *Opt) {
  const long NumPixels = ((long)Width) * ((long)Height);
  const long NumEl = NumPixels * NumChannels;
  tvregsolver S;
  usolver USolveFun = NULL;
  zsolver ZSolveFun = NULL;
  num DiffNorm = NAN;
  int i = 0, Success = 0, DeconvFlag = 0, DctFlag = 0, Iter = 0;

  if (!u || !f || u == f || Width < 2 || Height < 2 || NumChannels <= 0)
    return 0;

  /*** Set algorithm flags ***********************************************/
  S.Opt = (Opt) ? *Opt : TvRegDefaultOpt;

  if (!TvRestoreChooseAlgorithm(&S.UseZ, &DeconvFlag, &DctFlag, &USolveFun,
                                &ZSolveFun, &S.Opt))
    return 0;

  if (!DeconvFlag) {
    if (!S.Opt.VaryingLambda)
      fprintf(stderr,
              "Please recompile with TVREG_DENOISE "
              "for denoising problems.\n");
    else
      fprintf(stderr,
              "Please recompile with TVREG_INPAINT "
              "for inpainting problems.\n");
    return 0;
  }

  if (S.Opt.VaryingLambda &&
      (S.Opt.LambdaWidth != Width || S.Opt.LambdaHeight != Height)) {
    fprintf(stderr, "Image is %dx%d but lambda is %dx%d.\n", Width, Height,
            S.Opt.LambdaWidth, S.Opt.LambdaHeight);
    return 0;
  }

  S.u = u;
  S.f = f;
  S.Width = S.PadWidth = Width;
  S.Height = S.PadHeight = Height;
  S.NumChannels = NumChannels;
  S.Alpha = ((!S.UseZ) ? S.Opt.Lambda : S.Opt.Gamma2) / S.Opt.Gamma1;

  /*** Allocate memory ***************************************************/
  S.d = S.dtilde = NULL;

  S.A = S.B = S.ATrans = S.BTrans = S.KernelTrans = S.DenomTrans = NULL;
  S.TransformA = S.TransformB = S.InvTransformA = S.InvTransformB = NULL;

  if (!(S.d = (numvec2 *)Malloc(sizeof(numvec2) * NumEl)) ||
      !(S.dtilde = (numvec2 *)Malloc(sizeof(numvec2) * NumEl)))
    goto Catch;

  if (S.UseZ)

  { /* We need z but do not have it, show error message. */
    if (S.Opt.NoiseModel != NOISEMODEL_L2)
      fprintf(stderr,
              "Please recompile with TVREG_NONGAUSSIAN "
              "for non-Gaussian noise models.\n");
    else
      fprintf(stderr,
              "Please recompile with TVREG_DECONV and "
              "TVREG_INPAINT for deconvolution-inpainting problems.\n");

    goto Catch;
  }

  if (!DeconvFlag)
    S.Ku = u;
  else
    /* The following applies only for problems with deconvolution */
    if (DctFlag) { /* Prepare for DCT-based deconvolution */
      long PadNumPixels = ((long)Width + 1) * ((long)Height + 1);

      if (!(S.ATrans = (num *)FFT(malloc)(sizeof(num) * NumEl)) ||
          !(S.BTrans = (num *)FFT(malloc)(sizeof(num) * NumEl)) ||
          !(S.A = (num *)FFT(malloc)(sizeof(num) * NumEl)) ||
          !(S.B =
                (num *)FFT(malloc)(sizeof(num) * PadNumPixels * NumChannels)) ||
          !(S.KernelTrans = (num *)FFT(malloc)(sizeof(num) * PadNumPixels)) ||
          !(S.DenomTrans = (num *)Malloc(sizeof(num) * NumPixels)) ||
          !InitDeconvDct(&S))
        goto Catch;
    } else { /* Prepare for Fourier-based deconvolution */
      long NumTransPixels = 0, NumTransEl = 0, PadNumEl = 0;
      int TransWidth = 0;

      S.PadWidth = 2 * Width;
      S.PadHeight = 2 * Height;
      TransWidth = S.PadWidth / 2 + 1;
      NumTransPixels = ((long)TransWidth) * ((long)S.PadHeight);
      NumTransEl = NumTransPixels * NumChannels;
      PadNumEl = (((long)S.PadWidth) * S.PadHeight) * NumChannels;

      if (!(S.ATrans = (num *)FFT(malloc)(sizeof(numcomplex) * NumTransEl)) ||
          !(S.BTrans = (num *)FFT(malloc)(sizeof(numcomplex) * NumTransEl)) ||
          !(S.A = (num *)FFT(malloc)(sizeof(num) * PadNumEl)) ||
          !(S.B = (num *)FFT(malloc)(sizeof(num) * PadNumEl)) ||
          !(S.KernelTrans =
                (num *)FFT(malloc)(sizeof(numcomplex) * NumTransPixels)) ||
          !(S.DenomTrans = (num *)Malloc(sizeof(num) * NumTransPixels)) ||
          !InitDeconvFourier(&S))
        goto Catch;
    }

  /*** Algorithm initializations *****************************************/

  /* Set convergence threshold scaled by norm of f */
  for (i = 0, S.fNorm = 0; i < NumEl; i++) S.fNorm += f[i] * f[i];

  S.fNorm = (num)sqrt(S.fNorm);

  if (S.fNorm == 0) /* Special case, input image is zero */
  {
    memcpy(u, f, sizeof(num) * NumEl);
    Success = 1;
    goto Catch;
  }

  /* Initialize d = dtilde = 0 */
  for (i = 0; i < NumEl; i++) S.d[i].x = S.d[i].y = 0;

  for (i = 0; i < NumEl; i++) S.dtilde[i].x = S.dtilde[i].y = 0;

  DiffNorm = (S.Opt.Tol > 0) ? 1000 * S.Opt.Tol : 1000;
  Success = 2;

  if (S.Opt.PlotFun && !S.Opt.PlotFun(0, 0, DiffNorm, u, Width, Height,
                                      NumChannels, S.Opt.PlotParam))
    goto Catch;

  /*** Algorithm main loop: Bregman iterations ***************************/
  for (Iter = 1; Iter <= S.Opt.MaxIter; Iter++) {
    /* Solve d subproblem and update dtilde */
    DSolve(&S);

    /* Solve u subproblem */
    DiffNorm = USolveFun(&S);

    if (Iter >= 2 + S.UseZ && DiffNorm < S.Opt.Tol) break;

    if (S.Opt.PlotFun && !(S.Opt.PlotFun(0, Iter, DiffNorm, u, Width, Height,
                                         NumChannels, S.Opt.PlotParam)))
      goto Catch;
  }
  /*** End of main loop **************************************************/

  Success = (Iter <= S.Opt.MaxIter) ? 1 : 2;

  if (S.Opt.PlotFun)
    S.Opt.PlotFun(Success, (Iter <= S.Opt.MaxIter) ? Iter : S.Opt.MaxIter,
                  DiffNorm, u, Width, Height, NumChannels, S.Opt.PlotParam);
Catch:
  /*** Release memory ****************************************************/
  if (S.dtilde) Free(S.dtilde);
  if (S.d) Free(S.d);

  if (DeconvFlag) {
    if (S.DenomTrans) Free(S.DenomTrans);
    if (S.KernelTrans) FFT(free)(S.KernelTrans);
    if (S.B) FFT(free)(S.B);
    if (S.A) FFT(free)(S.A);
    if (S.BTrans) FFT(free)(S.BTrans);
    if (S.ATrans) FFT(free)(S.ATrans);

#ifdef _OPENMP
#pragma omp critical(fftw)
#endif
    {
      FFT(destroy_plan)(S.InvTransformB);
      FFT(destroy_plan)(S.TransformB);
      FFT(destroy_plan)(S.InvTransformA);
      FFT(destroy_plan)(S.TransformA);
    }
    /*FFT(cleanup)();*/
  }
  return Success;
}

/** @brief Test if Kernel is whole-sample symmetric */
static int IsSymmetric(const num *Kernel, int KernelWidth, int KernelHeight) {
  int x = 0, xr = 0, y = 0, yr = 0;

  if (KernelWidth % 2 == 0 || KernelHeight % 2 == 0) return 0;

  for (y = 0, yr = KernelHeight - 1; y < KernelHeight; y++, yr--)
    for (x = 0, xr = KernelWidth - 1; x < KernelWidth; x++, xr--)
      if (Kernel[x + KernelWidth * y] != Kernel[xr + KernelWidth * y] ||
          Kernel[x + KernelWidth * y] != Kernel[x + KernelWidth * yr])
        return 0;

  return 1;
}

/** @brief Algorithm planning function */
int TvRestoreChooseAlgorithm(int *UseZ, int *DeconvFlag, int *DctFlag,
                             usolver *USolveFun, zsolver *ZSolveFun,
                             const tvregopt *Opt) {
  if (!Opt) return 0;

  /* UseZ decides between the simpler d,u splitting or the d,u,z splitting
     of the problem.  ZSolveFun selects the z-subproblem solver. */
  *UseZ = (Opt->NoiseModel != NOISEMODEL_L2);

  *ZSolveFun = NULL;

  /* If there is a kernel, set DeconvFlag */
  if (Opt->Kernel) {
    /* Must use d,u,z splitting for deconvolution with
       spatially-varying lambda */
    if (Opt->VaryingLambda) *UseZ = 1;

    *DeconvFlag = 1;
    /* Use faster DCT solver if kernel is symmetric in both dimensions */
    *DctFlag = IsSymmetric(Opt->Kernel, Opt->KernelWidth, Opt->KernelHeight);
  } else
    *DeconvFlag = *DctFlag = 0;

  /* Select the u-subproblem solver */
  if (!*DeconvFlag) /* Gauss-Seidel solver for denoising and inpainting */
    *USolveFun = NULL;
  else
    *USolveFun = (*DctFlag) ? UDeconvDct : UDeconvFourier;

  return 1;
}
