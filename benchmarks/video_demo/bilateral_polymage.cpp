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
extern "C" void  pipeline_bilateral(int  C, int  R, void * input_void_arg, void * filtered_void_arg)
{
  unsigned char * input_orig;
  input_orig = (unsigned char *) (input_void_arg);
  float * filtered;
  filtered = (float *) (filtered_void_arg);

  unsigned char * input;
  input = (unsigned char *) (pool_allocate((sizeof(unsigned char) * (R * C * 3))));

  float * gray;
  gray = (float *) (pool_allocate((sizeof(float ) * (R * C))));
  float * grid;
  grid = (float *) (pool_allocate((sizeof(float ) * (((2 * (((((R - 1) / 8) - 1) - 1) + 1)) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15))));

  int off_left = 24;
  int total_pad = 56;

  int _R = R-total_pad;
  int _C = C-total_pad;
  for (int _i0 = off_left; _i0 < _R+off_left; _i0++)
  {
    for (int _i1 = off_left; _i1 < _C+off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input_orig[(_i0-off_left)*(_C)*3 + (_i1-off_left)*3 + _i2];
      }
    }
  }
  for (int _i0 = off_left; _i0 < _R+off_left; _i0++)
  {
    for (int _i1 = 0; _i1 < off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_i0) * C * 3 + off_left * 3 + _i2];
      }
    }
  }
  for (int _i0 = off_left; _i0 < _R+off_left; _i0++)
  {
    for (int _i1 = _C+off_left; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_i0) * C * 3 + (_C+(off_left-1)) * 3 + _i2];
      }
    }
  }

  for (int _i0 = 0; _i0 < off_left; _i0++)
  {
    for (int _i1 = 0; _i1 < off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[off_left * C * 3 + off_left * 3 + _i2];
      }
    }
  }
  for (int _i0 = 0; _i0 < off_left; _i0++)
  {
    for (int _i1 = off_left; _i1 < _C+off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[off_left * C * 3 + _i1 * 3 + _i2];
      }
    }
  }
  for (int _i0 = 0; _i0 < off_left; _i0++)
  {
    for (int _i1 = _C+off_left; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[off_left * C * 3 + (_C+(off_left-1)) * 3 + _i2];
      }
    }
  }

  for (int _i0 = _R+off_left; _i0 < R; _i0++)
  {
    for (int _i1 = 0; _i1 < off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+(off_left-1)) * C * 3 + off_left * 3 + _i2];
      }
    }
  }
  for (int _i0 = _R+off_left; _i0 < R; _i0++)
  {
    for (int _i1 = off_left; _i1 < _C+off_left; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+(off_left-1)) * C + _i1 * 3 + _i2];
      }
    }
  }
  for (int _i0 = _R+off_left; _i0 < R; _i0++)
  {
    for (int _i1 = _C+off_left; _i1 < C; _i1++)
    {
      for (int _i2 = 0; _i2 <= 2; _i2++)
      {
        input[_i0 * C * 3 + _i1 * 3 + _i2] = input[(_R+(off_left-1)) * C * 3 + (_C+(off_left-1)) * 3 + _i2];
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
    #pragma omp parallel for schedule(static)
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
  #pragma omp parallel for schedule(static)
  for (int  _T_i1 = -1; (_T_i1 <= ((R - 9) / 64)); _T_i1 = (_T_i1 + 1))
  {
    float  blurz[2][13][39][11];
    float  blurx[2][13][39][11];
    float  blury[2][13][39][11];
    float  interpolated[2][104][312];
    for (int  _T_i2 = -1; (_T_i2 <= ((C - 9) / 256)); _T_i2 = (_T_i2 + 1))
    {
      for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
      {
        int  _ct332 = (((R - 9) < ((64 * _T_i1) + 96))? (R - 9): ((64 * _T_i1) + 96));
        int  _ct333 = ((8 > (64 * _T_i1))? 8: (64 * _T_i1));
        for (int  _i1 = _ct333; (_i1 <= _ct332); _i1 = (_i1 + 8))
        {
          int  _ct334 = (((C - 9) < ((256 * _T_i2) + 304))? (C - 9): ((256 * _T_i2) + 304));
          int  _ct335 = ((8 > (256 * _T_i2))? 8: (256 * _T_i2));
          for (int  _i2 = _ct335; (_i2 <= _ct334); _i2 = (_i2 + 8))
          {
          #pragma ivdep
            for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
            {
              blurz[_i0][((_i1 / 8) - (8 * _T_i1))][((_i2 / 8) - (32 * _T_i2))][(_i3 - 2)] = ((((grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + (_i2 / 8)) * 15)) + (-2 + _i3))] + (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + (_i2 / 8)) * 15)) + (-1 + _i3))])) + (6 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + (_i2 / 8)) * 15)) + _i3)])) + (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + (_i2 / 8)) * 15)) + (1 + _i3))])) + grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) * (((((C - 1) / 8) - 1) - 1) + 1)) * 15)) + ((-1 + (_i1 / 8)) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) + ((-1 + (_i2 / 8)) * 15)) + (2 + _i3))]);
            }
          }
        }
      }
      for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
      {
        int  _ct336 = (((R - 25) < ((64 * _T_i1) + 96))? (R - 25): ((64 * _T_i1) + 96));
        int  _ct337 = ((24 > ((64 * _T_i1) + 8))? 24: ((64 * _T_i1) + 8));
        for (int  _i1 = _ct337; (_i1 <= _ct336); _i1 = (_i1 + 8))
        {
          int  _ct338 = (((C - 9) < ((256 * _T_i2) + 296))? (C - 9): ((256 * _T_i2) + 296));
          int  _ct339 = ((8 > ((256 * _T_i2) + 8))? 8: ((256 * _T_i2) + 8));
          for (int  _i2 = _ct339; (_i2 <= _ct338); _i2 = (_i2 + 8))
          {
          #pragma ivdep
            for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
            {
              blurx[_i0][((_i1 / 8) - (8 * _T_i1))][((_i2 / 8) - (32 * _T_i2))][(_i3 - 2)] = ((((blurz[_i0][(-2 + ((_i1 / 8) - (8 * _T_i1)))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)] + (4 * blurz[_i0][(-1 + ((_i1 / 8) - (8 * _T_i1)))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)])) + (6 * blurz[_i0][((_i1 / 8) - (8 * _T_i1))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)])) + (4 * blurz[_i0][(1 + ((_i1 / 8) - (8 * _T_i1)))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)])) + blurz[_i0][(2 + ((_i1 / 8) - (8 * _T_i1)))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)]);
            }
          }
        }
      }
      for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
      {
        int  _ct340 = (((R - 25) < ((64 * _T_i1) + 88))? (R - 25): ((64 * _T_i1) + 88));
        int  _ct341 = ((24 > ((64 * _T_i1) + 8))? 24: ((64 * _T_i1) + 8));
        for (int  _i1 = _ct341; (_i1 <= _ct340); _i1 = (_i1 + 8))
        {
          int  _ct342 = (((C - 25) < ((256 * _T_i2) + 288))? (C - 25): ((256 * _T_i2) + 288));
          int  _ct343 = ((24 > ((256 * _T_i2) + 16))? 24: ((256 * _T_i2) + 16));
          for (int  _i2 = _ct343; (_i2 <= _ct342); _i2 = (_i2 + 8))
          {
          #pragma ivdep
            for (int  _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1))
            {
              blury[_i0][((_i1 / 8) - (8 * _T_i1))][((_i2 / 8) - (32 * _T_i2))][(_i3 - 2)] = ((((blurx[_i0][((_i1 / 8) - (8 * _T_i1))][(-2 + ((_i2 / 8) - (32 * _T_i2)))][(-2 + _i3)] + (4 * blurx[_i0][((_i1 / 8) - (8 * _T_i1))][(-1 + ((_i2 / 8) - (32 * _T_i2)))][(-2 + _i3)])) + (6 * blurx[_i0][((_i1 / 8) - (8 * _T_i1))][((_i2 / 8) - (32 * _T_i2))][(-2 + _i3)])) + (4 * blurx[_i0][((_i1 / 8) - (8 * _T_i1))][(1 + ((_i2 / 8) - (32 * _T_i2)))][(-2 + _i3)])) + blurx[_i0][((_i1 / 8) - (8 * _T_i1))][(2 + ((_i2 / 8) - (32 * _T_i2)))][(-2 + _i3)]);
            }
          }
        }
      }
      for (int  _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1))
      {
        int  _ct344 = (((R - 33) < ((64 * _T_i1) + 85))? (R - 33): ((64 * _T_i1) + 85));
        int  _ct345 = ((24 > ((64 * _T_i1) + 12))? 24: ((64 * _T_i1) + 12));
        for (int  _i1 = _ct345; (_i1 <= _ct344); _i1 = (_i1 + 1))
        {
          int  _ct346 = (((C - 33) < ((256 * _T_i2) + 287))? (C - 33): ((256 * _T_i2) + 287));
          int  _ct347 = ((24 > ((256 * _T_i2) + 18))? 24: ((256 * _T_i2) + 18));
          #pragma ivdep
          for (int  _i2 = _ct347; (_i2 <= _ct346); _i2 = (_i2 + 1))
          {
            interpolated[_i0][((-64 * _T_i1) + _i1)][((-256 * _T_i2) + _i2)] = (((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)]))) - (blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)])))))) + (((gray[((_i1 * C) + _i2)] * 10) - (int ) ((gray[((_i1 * C) + _i2)] * 10))) * (((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][((((-256 * _T_i2) + _i2) / 8) + 1)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)]))) - (blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][(((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) + 1) - 2)])))))) - ((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)]))) + (((float ) ((_i2 % 8)) / 8) * ((blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][((((-256 * _T_i2) + _i2) / 8) + 1)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)]))) - (blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] + (((float ) ((_i1 % 8)) / 8) * (blury[_i0][((((-64 * _T_i1) + _i1) / 8) + 1)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)] - blury[_i0][(((-64 * _T_i1) + _i1) / 8)][(((-256 * _T_i2) + _i2) / 8)][((2 + (int ) ((gray[((_i1 * C) + _i2)] * 10))) - 2)])))))))));
          }
        }
      }
      if ((_T_i2 >= 0))
      {
        int  _ct348 = (((R - 33) < ((64 * _T_i1) + 79))? (R - 33): ((64 * _T_i1) + 79));
        int  _ct349 = ((24 > ((64 * _T_i1) + 16))? 24: ((64 * _T_i1) + 16));
        for (int  _i1 = _ct349; (_i1 <= _ct348); _i1 = (_i1 + 1))
        {
          int  _ct350 = (((C - 33) < ((256 * _T_i2) + 279))? (C - 33): ((256 * _T_i2) + 279));
          #pragma ivdep
          for (int  _i2 = ((256 * _T_i2) + 24); (_i2 <= _ct350); _i2 = (_i2 + 1))
          {
            float  _ct351 = (interpolated[0][((-64 * _T_i1) + _i1)][((-256 * _T_i2) + _i2)] / interpolated[1][((-64 * _T_i1) + _i1)][((-256 * _T_i2) + _i2)]);
            float  _ct352 = gray[((_i1 * C) + _i2)];
            float  _ct353 = ((interpolated[1][((-64 * _T_i1) + _i1)][((-256 * _T_i2) + _i2)] > 0)? _ct351: _ct352);
            filtered[((0 + ((_i1 - 24) * (-56 + C))) + (_i2 - 24))] = _ct353;
          }
        }
      }
    }
  }
  pool_deallocate(gray);
  pool_deallocate(grid);
  pool_deallocate(input);
}


