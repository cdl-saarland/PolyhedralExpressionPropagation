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
  WARRANTIES, INCLUDING,
  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Ravi Teja Mullapudi, Vinay
  Vasista, CSA Indian Institute of Science BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <cmath>
#include <string.h>
#include "simple_pool_allocator.h"
#define isl_min(x,y) ((x) < (y) ? (x) : (y))
#define isl_max(x,y) ((x) > (y) ? (x) : (y))
#define isl_floord(n,d) (((n)<0) ? -((-(n)+(d)-1)/(d)) : (n)/(d))
extern "C" void  pipeline_bilateral_naive(int  C, int  R, void * input_void_arg, void * filtered_void_arg)
{
  unsigned char * input_orig;
  input_orig = (unsigned char *) (input_void_arg);
  float * filtered;
  filtered = (float *) (filtered_void_arg);

  float * gray;
  gray = (float *) (pool_allocate((sizeof(float ) * (R * C))));
  float * grid;
  grid = (float *) (pool_allocate((sizeof(float ) * (((2 * (((((R - 1) / 8) - 1) - 1) + 1)) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15))));
  float * blurz;
  blurz = (float *) (pool_allocate((sizeof(float ) * (((2 * (((((R - 1) / 8) - 1) - 1) + 1)) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11))));
  float * blurx;
  blurx = (float *) (pool_allocate((sizeof(float ) * (((2 * (((((R - 1) / 8) - 3) - 3) + 1)) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11))));
  float * blury;
  blury = (float *) (pool_allocate((sizeof(float ) * (((2 * (((((R - 1) / 8) - 3) - 3) + 1)) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11))));
  float * interpolated;
  interpolated = (float *) (pool_allocate((sizeof(float ) * ((2 * (-56 + R)) * (-56 + C)))));

  unsigned char * input;
  input = (unsigned char *) (pool_allocate((sizeof(unsigned char) * (R * C * 3))));

  int _R = R-56;
  int _C = C-56;
  for (int _i0 = 24; _i0 < _R+24; _i0++)
  {
    for (int _i1 = 24; _i1 < _C+24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input_orig[(_i0-24)*(_C)*3 + (_i1-24)*3 + _i2];
      }
    }
  }
  for (int _i0 = 24; _i0 < _R+24; _i0++)
  {
    for (int _i1 = 0; _i1 < 24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_i0) * C * 3 + 24 * 3 + _i2];
      }
    }
  }
  for (int _i0 = 24; _i0 < _R+24; _i0++)
  {
    for (int _i1 = _C+24; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_i0) * C * 3 + (_C+15) * 3 + _i2];
      }
    }
  }

  for (int _i0 = 0; _i0 < 24; _i0++)
  {
    for (int _i1 = 0; _i1 < 24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[24 * C * 3 + 24 * 3 + _i2];
      }
    }
  }
  for (int _i0 = 0; _i0 < 24; _i0++)
  {
    for (int _i1 = 24; _i1 < _C+24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[24 * C * 3 + _i1 * 3 + _i2];
      }
    }
  }
  for (int _i0 = 0; _i0 < 24; _i0++)
  {
    for (int _i1 = _C+24; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[24 * C * 3 + (_C+15) * 3 + _i2];
      }
    }
  }

  for (int _i0 = _R+24; _i0 < R; _i0++)
  {
    for (int _i1 = 0; _i1 < 24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+15) * C * 3 + 24 * 3 + _i2];
      }
    }
  }
  for (int _i0 = _R+24; _i0 < R; _i0++)
  {
    for (int _i1 = 24; _i1 < _C+24; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+15) * C + _i1 * 3 + _i2];
      }
    }
  }
  for (int _i0 = _R+24; _i0 < R; _i0++)
  {
    for (int _i1 = _C+24; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+15) * C * 3 + (_C+15) * 3 + _i2];
      }
    }
  }

  #pragma omp parallel for schedule(static)
  for (int  _i1 = 0; (_i1 < R); _i1 = (_i1 + 1))
  {
    #pragma ivdep
    for (int  _i2 = 0; (_i2 < C); _i2 = (_i2 + 1))
    {
      gray[((_i1 * C) + _i2)] = ((((input[((_i1 * (C * 3)) + (_i2 * 3))] * 0.114f) + (input[(((_i1 * (C * 3)) + (_i2 * 3)) + 1)] * 0.587f)) + (input[(((_i1 * (C * 3)) + (_i2 * 3)) + 2)] * 0.299f)) / 256.0f);
    }
  }
  for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
  {
    for (int  _i1 = 1; (_i1 < ((R - 1) / 8)); _i1 = (_i1 + 1))
    {
      for (int  _i2 = 1; (_i2 < ((C - 1) / 8)); _i2 = (_i2 + 1))
      {
        for (int  _i3 = 0; (_i3 <= 14); _i3 = (_i3 + 1))
        {
          grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((_i1 - 1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((_i2 - 1) * 15)) + _i3)] = 0;
        }
      }
    }
  }
  #pragma omp parallel for schedule(static)
  for (int  _i0 = 1; (_i0 < ((R - 1) / 8)); _i0 = (_i0 + 1))
  {
    for (int  _i1 = 1; (_i1 < ((C - 1) / 8)); _i1 = (_i1 + 1))
    {
      for (int  _i2 = 0; (_i2 <= 8); _i2 = (_i2 + 1))
      {
        for (int  _i3 = 0; (_i3 <= 8); _i3 = (_i3 + 1))
        {
          grid[((((0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i1) * 15)) + (2 + (int ) (((gray[((((-4 + _i2) + (8 * _i0)) * C) + ((-4 + (8 * _i1)) + _i3))] * 10) + 0.5f))))] = (grid[((((0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i1) * 15)) + (2 + (int ) (((gray[((((-4 + _i2) + (8 * _i0)) * C) + ((-4 + (8 * _i1)) + _i3))] * 10) + 0.5f))))] + gray[((((-4 + _i2) + (8 * _i0)) * C) + ((-4 + (8 * _i1)) + _i3))]);
          grid[((((1 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i1) * 15)) + (2 + (int ) (((gray[((((-4 + _i2) + (8 * _i0)) * C) + ((-4 + (8 * _i1)) + _i3))] * 10) + 0.5f))))] = (grid[((((1 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i1) * 15)) + (2 + (int ) (((gray[((((-4 + _i2) + (8 * _i0)) * C) + ((-4 + (8 * _i1)) + _i3))] * 10) + 0.5f))))] + 1);
        }
      }
    }
  }
  for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
  {
  #pragma omp parallel for schedule(static)
    for (int  _i1 = 1; (_i1 < ((R - 1) / 8)); _i1 = (_i1 + 1))
    {
      for (int  _i2 = 1; (_i2 < ((C - 1) / 8)); _i2 = (_i2 + 1))
      {
    #pragma ivdep
        for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
        {
          blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((_i1 - 1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((_i2 - 1) * 11)) + (_i3 - 2))] = ((((grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i2) * 15)) + (-2 + _i3))] + (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i2) * 15)) + (-1 + _i3))])) + (6 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i2) * 15)) + _i3)])) + (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i2) * 15)) + (1 + _i3))])) + grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + _i2) * 15)) + (2 + _i3))]);
        }
      }
    }
  }
  for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
  {
  #pragma omp parallel for schedule(static)
    for (int  _i1 = 3; (_i1 < (((R - 1) / 8) - 2)); _i1 = (_i1 + 1))
    {
      for (int  _i2 = 1; (_i2 < ((C - 1) / 8)); _i2 = (_i2 + 1))
      {
    #pragma ivdep
        for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
        {
          blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((_i1 - 3) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((_i2 - 1) * 11)) + (_i3 - 2))] = ((((blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + _i2) * 11)) + (-2 + _i3))] + (4 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-2 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + _i2) * 11)) + (-2 + _i3))])) + (6 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + _i2) * 11)) + (-2 + _i3))])) + (4 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + (_i1 * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + _i2) * 11)) + (-2 + _i3))])) + blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + _i2) * 11)) + (-2 + _i3))]);
        }
      }
    }
  }
  for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
  {
  #pragma omp parallel for schedule(static)
    for (int  _i1 = 24; (_i1 < (R - 24)); _i1 = (_i1 + 8))
    {
      for (int  _i2 = 24; (_i2 < (C - 24)); _i2 = (_i2 + 8))
      {
    #pragma ivdep
        for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
        {
          blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (_i3 - 2))] = ((((blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-3 + (_i2 / 8)) * 11)) + (-2 + _i3))] + (4 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-2 + (_i2 / 8)) * 11)) + (-2 + _i3))])) + (6 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((-1 + (_i2 / 8)) * 11)) + (-2 + _i3))])) + (4 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((_i2 / 8) * 11)) + (-2 + _i3))])) + blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 11)) + ((-3 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) + ((1 + (_i2 / 8)) * 11)) + (-2 + _i3))]);
        }
      }
    }
  }
  for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
  {
  #pragma omp parallel for schedule(static)
    for (int  _i1 = 24; (_i1 < (R - 32)); _i1 = (_i1 + 1))
    {
      for (int  _i2 = 24; (_i2 < (C - 32)); _i2 = (_i2 + 1))
      {
        interpolated[(((_i0 * ((-56 + R) * (-56 + C))) + ((_i1 - 24) * (-56 + C))) + (_i2 - 24))] = (((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))]))) - (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))])))))) + (((gray[((_i1 * C) + _i2)] * 10) - (int ) ((gray[((_i1 * C) + _i2)] * 10))) * (((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))]))) - (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + (((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2))])))))) - ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + ((((_i2 / 8) + 1) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))]))) - (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] + (((float ) ((_i1 % 8)) / 8) * (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + ((((_i1 / 8) + 1) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))] - blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) * (((((C - 1) / 8) - 3) - 3) + 1)) * 11)) + (((_i1 / 8) - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) + (((_i2 / 8) - 3) * 11)) + ((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2))])))))))));
      }
    }
  }
  #pragma omp parallel for schedule(static)
  for (int  _i1 = 24; (_i1 < (R - 32)); _i1 = (_i1 + 1))
  {
    #pragma ivdep
    for (int  _i2 = 24; (_i2 < (C - 32)); _i2 = (_i2 + 1))
    {
      float  _ct0 = (interpolated[(((0 * ((-56 + R) * (-56 + C))) + ((-24 + _i1) * (-56 + C))) + (-24 + _i2))] / interpolated[(((1 * ((-56 + R) * (-56 + C))) + ((-24 + _i1) * (-56 + C))) + (-24 + _i2))]);
      float  _ct1 = gray[((_i1 * C) + _i2)];
      float  _ct2 = ((interpolated[(((1 * ((-56 + R) * (-56 + C))) + ((-24 + _i1) * (-56 + C))) + (-24 + _i2))] > 0)? _ct0: _ct1);
      filtered[((0 + ((_i1 - 24) * (-56 + C))) + (_i2 - 24))] = _ct2;
    }
  }
  pool_deallocate(input);
  pool_deallocate(gray);
  pool_deallocate(grid);
  pool_deallocate(blurz);
  pool_deallocate(blurx);
  pool_deallocate(blury);
  pool_deallocate(interpolated);
}
