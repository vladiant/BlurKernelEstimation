/**
 * @file tvreg.h
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
#pragma once

#include "basic.h"
#include "num.h"

/** @brief Default fidelity weight */
#define TVREGOPT_DEFAULT_LAMBDA 25
/** @brief Default convegence tolerance */
#define TVREGOPT_DEFAULT_TOL 1e-3
/** @brief Default penalty weight on the d = grad u constraint */
#define TVREGOPT_DEFAULT_GAMMA1 5
/** @brief Default penalty weight on the z = u constraint */
#define TVREGOPT_DEFAULT_GAMMA2 8
/** @brief Default maximum number of Bregman iterations */
#define TVREGOPT_DEFAULT_MAXITER 100

/* tvregopt is encapsulated by forward declaration */
typedef struct tag_tvregopt tvregopt;
typedef struct tag_tvregsolver tvregsolver;
typedef num (*usolver)(tvregsolver *);
typedef void (*zsolver)(tvregsolver *);

int TvRestore(num *u, const num *f, int Width, int Height, int NumChannels,
              tvregopt *Opt);

/** @brief Algorithm planning function */
int TvRestoreChooseAlgorithm(int *UseZ, int *DeconvFlag, int *DctFlag,
                             usolver *USolveFun, zsolver *ZSolveFun,
                             const tvregopt *Opt);

/**
 * @brief Solve the d subproblem with vectorial shrinkage
 * @param S tvreg solver state
 *
 * This routine solves the d subproblem to update d,
 * \f[ \operatorname*{arg\,min}_{d}\,\sum_{i,j}\lvert d_{i,j}\rvert+\frac{
 * \gamma}{2}\sum_{i,j}\lvert d_{i,j}-b_{i,j}-\nabla u_{i,j}\rvert^2, \f]
 * where \f$ \nabla \f$ is the discrete gradient and the second term is a
 * penalty to encourage the constraint \f$ d = \nabla u \f$.  The solution is
 * the vectorial shrinkage with shrinkage parameter \f$ 1/\gamma \f$,
 * \f[ d_{i,j}=\frac{\nabla u_{i,j}+b_{i,j}}{\lvert\nabla u_{i,j}+b_{i,j}
 * \rvert}\max\bigl\{\lvert\nabla u_{i,j}+b_{i,j}\rvert-1/\gamma,0\bigr\}. \f]
 * The discrete gradient of u is computed with forward differences.  At the
 * right and bottom boundaries, the difference is set to zero.
 *
 * The routine also updates the auxiliary variable b according to
 * \f[ b = b + \nabla u - d. \f]
 * Rather than representing b directly, we use  \f$ \tilde d = d - b \f$,
 * which is algebraically equivalent but requires less arithmetic.
 *
 * To represent the vector field d, we implement d as a numvec2 array of
 * size Width x Height x NumChannels such that
@code
    d[i + Width*(j + Height*k)].x = x-component at pixel (i,j) channel k,
    d[i + Width*(j + Height*k)].y = y-component at pixel (i,j) channel k,
@endcode
 * where i = 0, ..., Width-1, j = 0, ..., Height-1, and k = 0, ...,
 * NumChannels-1.  This structure is also used for \f$ \tilde d \f$.
 */
void DSolve(tvregsolver *S);