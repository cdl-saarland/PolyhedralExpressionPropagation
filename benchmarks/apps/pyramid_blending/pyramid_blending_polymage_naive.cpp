/*
  Copyright (c) 2015 Indian Institute of Science
  All rights reserved.

  Written and provided by:
  Ravi Teja Mullapudi, Vinay Vasista, Uday Bondhugula
  Dept of Computer Science and Automation
  Indian Institute of Science
  Bangalore 560012
  India

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  3. Neither the name of the Indian Institute of Science nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS MATERIAL IS PROVIDED BY Ravi Teja Mullapudi, Vinay Vasista, and Uday
  Bondhugula, Indian Institute of Science ''AS IS'' AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  EVENT SHALL Ravi Teja Mullapudi, Vinay Vasista, CSA Indian Institute of Science
  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <cmath>
#include <string.h>
#define isl_min(x,y) ((x) < (y) ? (x) : (y))
#define isl_max(x,y) ((x) > (y) ? (x) : (y))
#define isl_floord(n,d) (((n)<0) ? -((-(n)+(d)-1)/(d)) : (n)/(d))
extern "C" void  pipeline_blend(long  C, long  R, void * img1_void_arg, void * img2_void_arg, void * mask_void_arg, void * blend_void_arg)
{
  float * img1;
  img1 = (float *) (img1_void_arg);
  float * img2;
  img2 = (float *) (img2_void_arg);
  float * mask;
  mask = (float *) (mask_void_arg);
  float * blend;
  blend = (float *) (blend_void_arg);
  float * Dx_1_img1;
  Dx_1_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 2) - 2) - 1) + 1)) * (-1 + C)))));
  float * Dx_1_img2;
  Dx_1_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 2) - 2) - 1) + 1)) * (-1 + C)))));
  float * Dx_1_mask;
  Dx_1_mask = (float *) (malloc((sizeof(float ) * (((((R / 2) - 2) - 1) + 1) * (-1 + C)))));
  float * Dy_1_img1;
  Dy_1_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 2) - 2) - 1) + 1)) * ((((C / 2) - 2) - 1) + 1)))));
  float * Dy_1_img2;
  Dy_1_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 2) - 2) - 1) + 1)) * ((((C / 2) - 2) - 1) + 1)))));
  float * Dy_1_mask;
  Dy_1_mask = (float *) (malloc((sizeof(float ) * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1)))));
  float * Dx_2_img1;
  Dx_2_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 4) - 2) - 2) + 1)) * ((((C / 2) - 2) - 1) + 1)))));
  float * Dx_2_img2;
  Dx_2_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 4) - 2) - 2) + 1)) * ((((C / 2) - 2) - 1) + 1)))));
  float * Dx_2_mask;
  Dx_2_mask = (float *) (malloc((sizeof(float ) * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1)))));
  float * Ux_0_img1;
  Ux_0_img1 = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * Ux_0_img2;
  Ux_0_img2 = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * Dy_2_img1;
  Dy_2_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 4) - 2) - 2) + 1)) * ((((C / 4) - 2) - 2) + 1)))));
  float * Dy_2_img2;
  Dy_2_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 4) - 2) - 2) + 1)) * ((((C / 4) - 2) - 2) + 1)))));
  float * Dy_2_mask;
  Dy_2_mask = (float *) (malloc((sizeof(float ) * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1)))));
  float * Uy_0_img1;
  Uy_0_img1 = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (-60 + C)))));
  float * Uy_0_img2;
  Uy_0_img2 = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (-60 + C)))));
  float * Dx_3_img1;
  Dx_3_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 8) - 2) - 3) + 1)) * ((((C / 4) - 2) - 2) + 1)))));
  float * Dx_3_img2;
  Dx_3_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 8) - 2) - 3) + 1)) * ((((C / 4) - 2) - 2) + 1)))));
  float * Dx_3_mask;
  Dx_3_mask = (float *) (malloc((sizeof(float ) * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1)))));
  float * Res_0;
  Res_0 = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (-60 + C)))));
  float * Ux_1_img1;
  Ux_1_img1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Ux_1_img2;
  Ux_1_img2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Dy_3_img1;
  Dy_3_img1 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 8) - 2) - 3) + 1)) * ((((C / 8) - 2) - 3) + 1)))));
  float * Dy_3_img2;
  Dy_3_img2 = (float *) (malloc((sizeof(float ) * ((3 * ((((R / 8) - 2) - 3) + 1)) * ((((C / 8) - 2) - 3) + 1)))));
  float * Dy_3_mask;
  Dy_3_mask = (float *) (malloc((sizeof(float ) * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1)))));
  float * Uy_1_img1;
  Uy_1_img1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * Uy_1_img2;
  Uy_1_img2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * Res_1;
  Res_1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * Res_3;
  Res_3 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 8) - 4) + 2) - 3) + 1)) * (((((C / 8) - 4) + 2) - 3) + 1)))));
  float * Ux_2_img1;
  Ux_2_img1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 8) - 4) + 2) - 3) + 1)))));
  float * Ux_2_img2;
  Ux_2_img2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 8) - 4) + 2) - 3) + 1)))));
  float * Col_2_x;
  Col_2_x = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 8) - 4) + 2) - 3) + 1)))));
  float * Uy_2_img1;
  Uy_2_img1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Uy_2_img2;
  Uy_2_img2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Res_2;
  Res_2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Col_2;
  Col_2 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 4) - 8) + 2) - 7) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Col_1_x;
  Col_1_x = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 4) - 8) + 2) - 7) + 1)))));
  float * Col_1;
  Col_1 = (float *) (malloc((sizeof(float ) * ((3 * (((((R / 2) - 16) + 2) - 15) + 1)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  float * blend_x;
  blend_x = (float *) (malloc((sizeof(float ) * ((3 * (-60 + R)) * (((((C / 2) - 16) + 2) - 15) + 1)))));
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 0; (_i2 <= 2106); _i2 = (_i2 + 1))
      {
        Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((_i1 - 1) * (-1 + C))) + _i2)] = (((((img2[(((_i0 * (R * C)) + ((-2 + (2 * _i1)) * C)) + _i2)] + (4 * img2[(((_i0 * (R * C)) + ((-1 + (2 * _i1)) * C)) + _i2)])) + (6 * img2[(((_i0 * (R * C)) + ((2 * _i1) * C)) + _i2)])) + (4 * img2[(((_i0 * (R * C)) + ((1 + (2 * _i1)) * C)) + _i2)])) + img2[(((_i0 * (R * C)) + ((2 + (2 * _i1)) * C)) + _i2)]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 0; (_i2 <= 2106); _i2 = (_i2 + 1))
      {
        Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((_i1 - 1) * (-1 + C))) + _i2)] = (((((img1[(((_i0 * (R * C)) + ((-2 + (2 * _i1)) * C)) + _i2)] + (4 * img1[(((_i0 * (R * C)) + ((-1 + (2 * _i1)) * C)) + _i2)])) + (6 * img1[(((_i0 * (R * C)) + ((2 * _i1) * C)) + _i2)])) + (4 * img1[(((_i0 * (R * C)) + ((1 + (2 * _i1)) * C)) + _i2)])) + img1[(((_i0 * (R * C)) + ((2 + (2 * _i1)) * C)) + _i2)]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
      {
        Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((_i1 - 1) * ((((C / 2) - 2) - 1) + 1))) + (_i2 - 1))] = (((((Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (-2 + (2 * _i2)))] + (4 * Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (-1 + (2 * _i2)))])) + (6 * Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (2 * _i2))])) + (4 * Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (1 + (2 * _i2)))])) + Dx_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (2 + (2 * _i2)))]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
      {
        Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((_i1 - 1) * ((((C / 2) - 2) - 1) + 1))) + (_i2 - 1))] = (((((Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (-2 + (2 * _i2)))] + (4 * Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (-1 + (2 * _i2)))])) + (6 * Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (2 * _i2))])) + (4 * Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (1 + (2 * _i2)))])) + Dx_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * (-1 + C))) + ((-1 + _i1) * (-1 + C))) + (2 + (2 * _i2)))]) * 0.0625f);
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 0; (_i2 <= 2106); _i2 = (_i2 + 1))
    {
      Dx_1_mask[(((_i1 - 1) * (-1 + C)) + _i2)] = (((((mask[(((-2 + (2 * _i1)) * C) + _i2)] + (4 * mask[(((-1 + (2 * _i1)) * C) + _i2)])) + (6 * mask[(((2 * _i1) * C) + _i2)])) + (4 * mask[(((1 + (2 * _i1)) * C) + _i2)])) + mask[(((2 + (2 * _i1)) * C) + _i2)]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
      {
        Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((_i1 - 2) * ((((C / 2) - 2) - 1) + 1))) + (_i2 - 1))] = (((((Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-3 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] + (4 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + (6 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + (4 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((2 * _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
      {
        Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((_i1 - 2) * ((((C / 2) - 2) - 1) + 1))) + (_i2 - 1))] = (((((Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-3 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] + (4 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + (6 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + (4 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((2 * _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) * 0.0625f);
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 1; (_i1 <= 1052); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
    {
      Dy_1_mask[(((_i1 - 1) * ((((C / 2) - 2) - 1) + 1)) + (_i2 - 1))] = (((((Dx_1_mask[(((-1 + _i1) * (-1 + C)) + (-2 + (2 * _i2)))] + (4 * Dx_1_mask[(((-1 + _i1) * (-1 + C)) + (-1 + (2 * _i2)))])) + (6 * Dx_1_mask[(((-1 + _i1) * (-1 + C)) + (2 * _i2))])) + (4 * Dx_1_mask[(((-1 + _i1) * (-1 + C)) + (1 + (2 * _i2)))])) + Dx_1_mask[(((-1 + _i1) * (-1 + C)) + (2 + (2 * _i2)))]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
      {
        Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((_i1 - 2) * ((((C / 4) - 2) - 2) + 1))) + (_i2 - 2))] = (((((Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-3 + (2 * _i2)))] + (4 * Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-2 + (2 * _i2)))])) + (6 * Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + (2 * _i2)))])) + (4 * Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (2 * _i2))])) + Dx_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (1 + (2 * _i2)))]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
      {
        Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((_i1 - 2) * ((((C / 4) - 2) - 2) + 1))) + (_i2 - 2))] = (((((Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-3 + (2 * _i2)))] + (4 * Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-2 + (2 * _i2)))])) + (6 * Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + (2 * _i2)))])) + (4 * Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (2 * _i2))])) + Dx_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-2 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (1 + (2 * _i2)))]) * 0.0625f);
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 1; (_i2 <= 1052); _i2 = (_i2 + 1))
    {
      Dx_2_mask[(((_i1 - 2) * ((((C / 2) - 2) - 1) + 1)) + (_i2 - 1))] = (((((Dy_1_mask[(((-3 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))] + (4 * Dy_1_mask[(((-2 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))])) + (6 * Dy_1_mask[(((-1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))])) + (4 * Dy_1_mask[(((2 * _i1) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))])) + Dy_1_mask[(((1 + (2 * _i1)) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
      {
        Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((_i1 - 3) * ((((C / 4) - 2) - 2) + 1))) + (_i2 - 2))] = (((((Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-4 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] + (4 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + (6 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + (4 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-1 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((2 * _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
      {
        Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((_i1 - 3) * ((((C / 4) - 2) - 2) + 1))) + (_i2 - 2))] = (((((Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-4 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] + (4 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + (6 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + (4 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-1 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((2 * _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) * 0.0625f);
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 2; (_i1 <= 525); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
    {
      Dy_2_mask[(((_i1 - 2) * ((((C / 4) - 2) - 2) + 1)) + (_i2 - 2))] = (((((Dx_2_mask[(((-2 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (-3 + (2 * _i2)))] + (4 * Dx_2_mask[(((-2 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (-2 + (2 * _i2)))])) + (6 * Dx_2_mask[(((-2 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (-1 + (2 * _i2)))])) + (4 * Dx_2_mask[(((-2 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (2 * _i2))])) + Dx_2_mask[(((-2 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (1 + (2 * _i2)))]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((_i1 - 3) * ((((C / 8) - 2) - 3) + 1))) + (_i2 - 3))] = (((((Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-4 + (2 * _i2)))] + (4 * Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-3 + (2 * _i2)))])) + (6 * Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + (2 * _i2)))])) + (4 * Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-1 + (2 * _i2)))])) + Dx_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (2 * _i2))]) * 0.0625f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((_i1 - 3) * ((((C / 8) - 2) - 3) + 1))) + (_i2 - 3))] = (((((Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-4 + (2 * _i2)))] + (4 * Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-3 + (2 * _i2)))])) + (6 * Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + (2 * _i2)))])) + (4 * Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-1 + (2 * _i2)))])) + Dx_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-3 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (2 * _i2))]) * 0.0625f);
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 2; (_i2 <= 525); _i2 = (_i2 + 1))
    {
      Dx_3_mask[(((_i1 - 3) * ((((C / 4) - 2) - 2) + 1)) + (_i2 - 2))] = (((((Dy_2_mask[(((-4 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))] + (4 * Dy_2_mask[(((-3 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))])) + (6 * Dy_2_mask[(((-2 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))])) + (4 * Dy_2_mask[(((-1 + (2 * _i1)) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))])) + Dy_2_mask[(((2 * _i1) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((_i1 - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((4 * Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 - 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))]) + (4 * Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))])) * 0.125f);
      }
      if ((_i1 <= 519))
      {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
        {
          Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i1 + 1) - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + (((((_i1 + 1) / 2) - 1) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))] + (6 * Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))])) + Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + (((((_i1 + 1) / 2) + 1) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))]) * 0.125f);
        }
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((_i1 - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((4 * Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 - 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))]) + (4 * Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))])) * 0.125f);
      }
      if ((_i1 <= 519))
      {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
        {
          Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i1 + 1) - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + (((((_i1 + 1) / 2) - 1) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))] + (6 * Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))])) + Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + (((((_i1 + 1) / 2) + 1) - 3) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))]) * 0.125f);
        }
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
    {
      Dy_3_mask[(((_i1 - 3) * ((((C / 8) - 2) - 3) + 1)) + (_i2 - 3))] = (((((Dx_3_mask[(((-3 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-4 + (2 * _i2)))] + (4 * Dx_3_mask[(((-3 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-3 + (2 * _i2)))])) + (6 * Dx_3_mask[(((-3 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-2 + (2 * _i2)))])) + (4 * Dx_3_mask[(((-3 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-1 + (2 * _i2)))])) + Dx_3_mask[(((-3 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (2 * _i2))]) * 0.0625f);
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 2))
      {
        Uy_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] - (((4 * Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 - 1) / 2) - 3))]) + (4 * Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) * 0.125f));
        if ((_i2 <= 519))
        {
          Uy_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i2 + 1) - 7))] = (Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + (_i2 + 1)))] - (((Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) - 1) - 3))] + (6 * Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) + Ux_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) + 1) - 3))]) * 0.125f));
        }
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 2))
      {
        Uy_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] - (((4 * Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 - 1) / 2) - 3))]) + (4 * Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) * 0.125f));
        if ((_i2 <= 519))
        {
          Uy_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i2 + 1) - 7))] = (Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((-2 + _i1) * ((((C / 4) - 2) - 2) + 1))) + (-2 + (_i2 + 1)))] - (((Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) - 1) - 3))] + (6 * Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) + Ux_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) + 1) - 3))]) * 0.125f));
        }
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 3; (_i1 <= 261); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((_i1 - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = ((Dy_3_img1[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((-3 + _i1) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))] * Dy_3_mask[(((-3 + _i1) * ((((C / 8) - 2) - 3) + 1)) + (-3 + _i2))]) + (Dy_3_img2[(((_i0 * (((((R / 8) - 2) - 3) + 1) * ((((C / 8) - 2) - 3) + 1))) + ((-3 + _i1) * ((((C / 8) - 2) - 3) + 1))) + (-3 + _i2))] * (1 - Dy_3_mask[(((-3 + _i1) * ((((C / 8) - 2) - 3) + 1)) + (-3 + _i2))])));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Res_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = ((Uy_2_img1[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-7 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))] * Dy_2_mask[(((-2 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))]) + (Uy_2_img2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-7 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))] * (1 - Dy_2_mask[(((-2 + _i1) * ((((C / 4) - 2) - 2) + 1)) + (-2 + _i2))])));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
      {
        Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((_i1 - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((4 * Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i1 - 1) / 2) - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (-3 + _i2))]) + (4 * Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (-3 + _i2))])) * 0.125f);
      }
      if ((_i1 <= 519))
      {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long  _i2 = 3; (_i2 <= 261); _i2 = (_i2 + 1))
        {
          Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i1 + 1) - 7) * (((((C / 8) - 4) + 2) - 3) + 1))) + (_i2 - 3))] = (((Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((((_i1 + 1) / 2) - 1) - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (-3 + _i2))] + (6 * Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i1 + 1) / 2) - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (-3 + _i2))])) + Res_3[(((_i0 * ((((((R / 8) - 4) + 2) - 3) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((((_i1 + 1) / 2) + 1) - 3) * (((((C / 8) - 4) + 2) - 3) + 1))) + (-3 + _i2))]) * 0.125f);
        }
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1039); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((4 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 - 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) + (4 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 + 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i1 + 1) - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + (((((_i1 + 1) / 2) - 1) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] + (6 * Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 + 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + Dy_2_img2[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + (((((_i1 + 1) / 2) + 1) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1039); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((4 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 - 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) + (4 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 + 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i1 + 1) - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + (((((_i1 + 1) / 2) - 1) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))] + (6 * Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + ((((_i1 + 1) / 2) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))])) + Dy_2_img1[(((_i0 * (((((R / 4) - 2) - 2) + 1) * ((((C / 4) - 2) - 2) + 1))) + (((((_i1 + 1) / 2) + 1) - 2) * ((((C / 4) - 2) - 2) + 1))) + (-2 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 7; (_i1 <= 521); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 2))
      {
        Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (Res_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-7 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))] + (((4 * Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 - 1) / 2) - 3))]) + (4 * Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) * 0.125f));
        if ((_i2 <= 519))
        {
          Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i2 + 1) - 7))] = (Res_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-7 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + (_i2 + 1)))] + (((Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) - 1) - 3))] + (6 * Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + (((_i2 + 1) / 2) - 3))])) + Col_2_x[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((-7 + _i1) * (((((C / 8) - 4) + 2) - 3) + 1))) + ((((_i2 + 1) / 2) + 1) - 3))]) * 0.125f));
        }
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1040); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1039); _i2 = (_i2 + 2))
      {
        Uy_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] - (((4 * Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 - 1) / 2) - 7))]) + (4 * Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) * 0.125f));
        Uy_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i2 + 1) - 15))] = (Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + (_i2 + 1)))] - (((Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) - 1) - 7))] + (6 * Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) + Ux_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) + 1) - 7))]) * 0.125f));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1040); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1039); _i2 = (_i2 + 2))
      {
        Uy_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] - (((4 * Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 - 1) / 2) - 7))]) + (4 * Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) * 0.125f));
        Uy_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i2 + 1) - 15))] = (Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((-1 + _i1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + (_i2 + 1)))] - (((Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) - 1) - 7))] + (6 * Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) + Ux_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) + 1) - 7))]) * 0.125f));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1039); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((_i1 - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((4 * Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i1 - 1) / 2) - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))]) + (4 * Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i1 + 1) / 2) - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 7; (_i2 <= 521); _i2 = (_i2 + 1))
      {
        Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i1 + 1) - 15) * (((((C / 4) - 8) + 2) - 7) + 1))) + (_i2 - 7))] = (((Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((((_i1 + 1) / 2) - 1) - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))] + (6 * Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i1 + 1) / 2) - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))])) + Col_2[(((_i0 * ((((((R / 4) - 8) + 2) - 7) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((((_i1 + 1) / 2) + 1) - 7) * (((((C / 4) - 8) + 2) - 7) + 1))) + (-7 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1040); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        Res_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = ((Uy_1_img1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-15 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))] * Dy_1_mask[(((-1 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))]) + (Uy_1_img2[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-15 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))] * (1 - Dy_1_mask[(((-1 + _i1) * ((((C / 2) - 2) - 1) + 1)) + (-1 + _i2))])));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2077); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((4 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 - 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) + (4 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 + 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i1 + 1) - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + (((((_i1 + 1) / 2) - 1) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] + (6 * Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 + 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + Dy_1_img2[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + (((((_i1 + 1) / 2) + 1) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2077); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((4 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 - 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) + (4 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 + 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i1 + 1) - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + (((((_i1 + 1) / 2) - 1) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))] + (6 * Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + ((((_i1 + 1) / 2) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))])) + Dy_1_img1[(((_i0 * (((((R / 2) - 2) - 1) + 1) * ((((C / 2) - 2) - 1) + 1))) + (((((_i1 + 1) / 2) + 1) - 1) * ((((C / 2) - 2) - 1) + 1))) + (-1 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 15; (_i1 <= 1040); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1039); _i2 = (_i2 + 2))
      {
        Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (Res_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-15 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))] + (((4 * Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 - 1) / 2) - 7))]) + (4 * Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) * 0.125f));
        Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i2 + 1) - 15))] = (Res_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-15 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + (_i2 + 1)))] + (((Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) - 1) - 7))] + (6 * Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + (((_i2 + 1) / 2) - 7))])) + Col_1_x[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((-15 + _i1) * (((((C / 4) - 8) + 2) - 7) + 1))) + ((((_i2 + 1) / 2) + 1) - 7))]) * 0.125f));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2078); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 31; (_i2 <= 2077); _i2 = (_i2 + 2))
      {
        Uy_0_img2[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + (_i2 - 31))] = (img2[(((_i0 * (R * C)) + (_i1 * C)) + _i2)] - (((4 * Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 - 1) / 2) - 15))]) + (4 * Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) * 0.125f));
        Uy_0_img2[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + ((_i2 + 1) - 31))] = (img2[(((_i0 * (R * C)) + (_i1 * C)) + (_i2 + 1))] - (((Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) - 1) - 15))] + (6 * Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) + Ux_0_img2[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) + 1) - 15))]) * 0.125f));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2078); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 31; (_i2 <= 2077); _i2 = (_i2 + 2))
      {
        Uy_0_img1[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + (_i2 - 31))] = (img1[(((_i0 * (R * C)) + (_i1 * C)) + _i2)] - (((4 * Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 - 1) / 2) - 15))]) + (4 * Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) * 0.125f));
        Uy_0_img1[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + ((_i2 + 1) - 31))] = (img1[(((_i0 * (R * C)) + (_i1 * C)) + (_i2 + 1))] - (((Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) - 1) - 15))] + (6 * Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) + Ux_0_img1[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) + 1) - 15))]) * 0.125f));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2077); _i1 = (_i1 + 2))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((_i1 - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((4 * Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i1 - 1) / 2) - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))]) + (4 * Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i1 + 1) / 2) - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))])) * 0.125f);
      }
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 15; (_i2 <= 1040); _i2 = (_i2 + 1))
      {
        blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i1 + 1) - 31) * (((((C / 2) - 16) + 2) - 15) + 1))) + (_i2 - 15))] = (((Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((((_i1 + 1) / 2) - 1) - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))] + (6 * Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i1 + 1) / 2) - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))])) + Col_1[(((_i0 * ((((((R / 2) - 16) + 2) - 15) + 1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((((_i1 + 1) / 2) + 1) - 15) * (((((C / 2) - 16) + 2) - 15) + 1))) + (-15 + _i2))]) * 0.125f);
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2078); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 31; (_i2 <= 2078); _i2 = (_i2 + 1))
      {
        Res_0[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + (_i2 - 31))] = ((Uy_0_img1[(((_i0 * ((-60 + R) * (-60 + C))) + ((-31 + _i1) * (-60 + C))) + (-31 + _i2))] * mask[((_i1 * C) + _i2)]) + (Uy_0_img2[(((_i0 * ((-60 + R) * (-60 + C))) + ((-31 + _i1) * (-60 + C))) + (-31 + _i2))] * (1 - mask[((_i1 * C) + _i2)])));
      }
    }
  }
  for (long  _i0 = 0; (_i0 <= 2); _i0 = (_i0 + 1))
  {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long  _i1 = 31; (_i1 <= 2078); _i1 = (_i1 + 1))
    {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long  _i2 = 31; (_i2 <= 2077); _i2 = (_i2 + 2))
      {
        blend[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + (_i2 - 31))] = (Res_0[(((_i0 * ((-60 + R) * (-60 + C))) + ((-31 + _i1) * (-60 + C))) + (-31 + _i2))] + (((4 * blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 - 1) / 2) - 15))]) + (4 * blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) * 0.125f));
        blend[(((_i0 * ((-60 + R) * (-60 + C))) + ((_i1 - 31) * (-60 + C))) + ((_i2 + 1) - 31))] = (Res_0[(((_i0 * ((-60 + R) * (-60 + C))) + ((-31 + _i1) * (-60 + C))) + (-31 + (_i2 + 1)))] + (((blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) - 1) - 15))] + (6 * blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + (((_i2 + 1) / 2) - 15))])) + blend_x[(((_i0 * ((-60 + R) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((-31 + _i1) * (((((C / 2) - 16) + 2) - 15) + 1))) + ((((_i2 + 1) / 2) + 1) - 15))]) * 0.125f));
      }
    }
  }
  free(Dx_1_img1);
  free(Dx_1_img2);
  free(Dx_1_mask);
  free(Dy_1_img1);
  free(Dy_1_img2);
  free(Dy_1_mask);
  free(Dx_2_img1);
  free(Dx_2_img2);
  free(Dx_2_mask);
  free(Ux_0_img1);
  free(Ux_0_img2);
  free(Dy_2_img1);
  free(Dy_2_img2);
  free(Dy_2_mask);
  free(Uy_0_img1);
  free(Uy_0_img2);
  free(Dx_3_img1);
  free(Dx_3_img2);
  free(Dx_3_mask);
  free(Res_0);
  free(Ux_1_img1);
  free(Ux_1_img2);
  free(Dy_3_img1);
  free(Dy_3_img2);
  free(Dy_3_mask);
  free(Uy_1_img1);
  free(Uy_1_img2);
  free(Res_1);
  free(Res_3);
  free(Ux_2_img1);
  free(Ux_2_img2);
  free(Col_2_x);
  free(Uy_2_img1);
  free(Uy_2_img2);
  free(Res_2);
  free(Col_2);
  free(Col_1_x);
  free(Col_1);
  free(blend_x);
}
