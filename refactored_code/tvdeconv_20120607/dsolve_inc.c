/**
 * @file dsolve_inc.c
 * @brief Solve the d subproblem
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

#include "tvreg.h"
#include "tvregopt.h"

void DSolve(tvregsolver *S) {
  numvec2 *d = S->d;
  numvec2 *dtilde = S->dtilde;
  const num *u = S->u;
  const int Width = S->Width;
  const int Height = S->Height;
  const int NumChannels = S->NumChannels;
  const num Thresh = 1 / S->Opt.Gamma1;
  const num ThreshSquared = Thresh * Thresh;
  const long ChannelStride = ((long)Width) * ((long)Height);
  const long NumEl = NumChannels * ChannelStride;
  numvec2 dnew;
  num Magnitude;
  long i;
  int x, y;

  for (y = 0; y < Height - 1; y++, d++, dtilde++, u++) {
    /* Perform vectorial shrinkage for interior points */
    for (x = 0; x < Width - 1; x++, d++, dtilde++, u++) {
      for (i = 0, Magnitude = 0; i < NumEl; i += ChannelStride) {
        d[i].x += (u[i + 1] - u[i]) - dtilde[i].x;
        d[i].y += (u[i + Width] - u[i]) - dtilde[i].y;
        Magnitude += d[i].x * d[i].x + d[i].y * d[i].y;
      }

      if (Magnitude > ThreshSquared) {
        Magnitude = 1 - Thresh / (num)sqrt(Magnitude);

        for (i = 0; i < NumEl; i += ChannelStride) {
          dnew.x = Magnitude * d[i].x;
          dnew.y = Magnitude * d[i].y;
          dtilde[i].x = 2 * dnew.x - d[i].x;
          dtilde[i].y = 2 * dnew.y - d[i].y;
          d[i] = dnew;
        }
      } else
        for (i = 0; i < NumEl; i += ChannelStride) {
          dtilde[i].x = -d[i].x;
          dtilde[i].y = -d[i].y;
          d[i].x = 0;
          d[i].y = 0;
        }
    }

    /* Right edge */
    for (i = 0, Magnitude = 0; i < NumEl; i += ChannelStride) {
      d[i].y += (u[i + Width] - u[i]) - dtilde[i].y;
      Magnitude += d[i].y * d[i].y;
      d[i].x = dtilde[i].x = 0;
    }

    if (Magnitude > ThreshSquared) {
      Magnitude = 1 - Thresh / (num)sqrt(Magnitude);

      for (i = 0; i < NumEl; i += ChannelStride) {
        dnew.y = Magnitude * d[i].y;
        dtilde[i].y = 2 * dnew.y - d[i].y;
        d[i].y = dnew.y;
      }
    } else
      for (i = 0; i < NumEl; i += ChannelStride) {
        dtilde[i].y = -d[i].y;
        d[i].y = 0;
      }
  }

  /* Bottom edge */
  for (x = 0; x < Width - 1; x++, d++, dtilde++, u++) {
    for (i = 0, Magnitude = 0; i < NumEl; i += ChannelStride) {
      d[i].x += (u[i + 1] - u[i]) - dtilde[i].x;
      Magnitude += d[i].x * d[i].x;
      d[i].y = dtilde[i].y = 0;
    }

    if (Magnitude > ThreshSquared) {
      Magnitude = 1 - Thresh / (num)sqrt(Magnitude);

      for (i = 0; i < NumEl; i += ChannelStride) {
        dnew.x = Magnitude * d[i].x;
        dtilde[i].x = 2 * dnew.x - d[i].x;
        d[i].x = dnew.x;
      }
    } else
      for (i = 0; i < NumEl; i += ChannelStride) {
        dtilde[i].x = -d[i].x;
        d[i].x = 0;
      }
  }

  /* Bottom-right corner */
  for (i = 0; i < NumEl; i += ChannelStride)
    d[i].x = d[i].y = dtilde[i].x = dtilde[i].y = 0;
}
