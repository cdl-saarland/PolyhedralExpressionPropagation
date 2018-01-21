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
extern "C" void  pipeline_harris(int  C, int  R, void * img_void_arg, void * harris_void_arg)
{
  unsigned char * img;
  img = (unsigned char *) (img_void_arg);
  float * harris;
  harris = (float *) (harris_void_arg);
  #pragma omp parallel for schedule(static)
  for (int  _T_i0 = -1; (_T_i0 <= ((R + 1) / 16)); _T_i0 = (_T_i0 + 1))
  {
    float  gray[26][138];
    float  Ix[26][138];
    float  Iy[26][138];
    float  Ixx[26][138];
    float  Iyy[26][138];
    float  Ixy[26][138];
    float  Syy[26][138];
    float  Sxy[26][138];
    float  Sxx[26][138];
    float  det[26][138];
    float  trace[26][138];
    for (int  _T_i1 = -1; (_T_i1 <= ((C + 1) / 128)); _T_i1 = (_T_i1 + 1))
    {
      int  _ct144 = (((R + 1) < ((16 * _T_i0) + 25))? (R + 1): ((16 * _T_i0) + 25));
      int  _ct145 = ((0 > (16 * _T_i0))? 0: (16 * _T_i0));
      for (int  _i0 = _ct145; (_i0 <= _ct144); _i0 = (_i0 + 1))
      {
        int  _ct146 = (((C + 1) < ((128 * _T_i1) + 137))? (C + 1): ((128 * _T_i1) + 137));
        int  _ct147 = ((0 > (128 * _T_i1))? 0: (128 * _T_i1));
        #pragma ivdep
        for (int  _i1 = _ct147; (_i1 <= _ct146); _i1 = (_i1 + 1))
        {
          gray[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((img[((_i0 * ((C + 2) * 3)) + (_i1 * 3))] * 0.114f) + (img[(((_i0 * ((C + 2) * 3)) + (_i1 * 3)) + 1)] * 0.587f)) + (img[(((_i0 * ((C + 2) * 3)) + (_i1 * 3)) + 2)] * 0.299f)) / 255.0f);
        }
      }
      int  _ct148 = ((R < ((16 * _T_i0) + 24))? R: ((16 * _T_i0) + 24));
      int  _ct149 = ((1 > ((16 * _T_i0) + 1))? 1: ((16 * _T_i0) + 1));
      for (int  _i0 = _ct149; (_i0 <= _ct148); _i0 = (_i0 + 1))
      {
        int  _ct150 = ((C < ((128 * _T_i1) + 136))? C: ((128 * _T_i1) + 136));
        int  _ct151 = ((1 > ((128 * _T_i1) + 1))? 1: ((128 * _T_i1) + 1));
        #pragma ivdep
        for (int  _i1 = _ct151; (_i1 <= _ct150); _i1 = (_i1 + 1))
        {
          Ix[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((((gray[(-1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] * -0.0833333333333f) + (gray[(1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] * 0.0833333333333f)) + (gray[(-1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)] * -0.166666666667f)) + (gray[(1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)] * 0.166666666667f)) + (gray[(-1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))] * -0.0833333333333f)) + (gray[(1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))] * 0.0833333333333f));
        }
      }
      int  _ct152 = ((R < ((16 * _T_i0) + 24))? R: ((16 * _T_i0) + 24));
      int  _ct153 = ((1 > ((16 * _T_i0) + 1))? 1: ((16 * _T_i0) + 1));
      for (int  _i0 = _ct153; (_i0 <= _ct152); _i0 = (_i0 + 1))
      {
        int  _ct154 = ((C < ((128 * _T_i1) + 136))? C: ((128 * _T_i1) + 136));
        int  _ct155 = ((1 > ((128 * _T_i1) + 1))? 1: ((128 * _T_i1) + 1));
        #pragma ivdep
        for (int  _i1 = _ct155; (_i1 <= _ct154); _i1 = (_i1 + 1))
        {
          Iy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((((gray[(-1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] * -0.0833333333333f) + (gray[(-1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))] * 0.0833333333333f)) + (gray[((-16 * _T_i0) + _i0)][(-1 + ((-128 * _T_i1) + _i1))] * -0.166666666667f)) + (gray[((-16 * _T_i0) + _i0)][(1 + ((-128 * _T_i1) + _i1))] * 0.166666666667f)) + (gray[(1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] * -0.0833333333333f)) + (gray[(1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))] * 0.0833333333333f));
        }
      }
      int  _ct156 = ((R < ((16 * _T_i0) + 23))? R: ((16 * _T_i0) + 23));
      int  _ct157 = ((1 > ((16 * _T_i0) + 2))? 1: ((16 * _T_i0) + 2));
      for (int  _i0 = _ct157; (_i0 <= _ct156); _i0 = (_i0 + 1))
      {
        int  _ct158 = ((C < ((128 * _T_i1) + 135))? C: ((128 * _T_i1) + 135));
        int  _ct159 = ((1 > ((128 * _T_i1) + 2))? 1: ((128 * _T_i1) + 2));
        #pragma ivdep
        for (int  _i1 = _ct159; (_i1 <= _ct158); _i1 = (_i1 + 1))
        {
          Ixx[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = (Ix[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] * Ix[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]);
        }
      }
      int  _ct160 = ((R < ((16 * _T_i0) + 23))? R: ((16 * _T_i0) + 23));
      int  _ct161 = ((1 > ((16 * _T_i0) + 2))? 1: ((16 * _T_i0) + 2));
      for (int  _i0 = _ct161; (_i0 <= _ct160); _i0 = (_i0 + 1))
      {
        int  _ct162 = ((C < ((128 * _T_i1) + 135))? C: ((128 * _T_i1) + 135));
        int  _ct163 = ((1 > ((128 * _T_i1) + 2))? 1: ((128 * _T_i1) + 2));
        #pragma ivdep
        for (int  _i1 = _ct163; (_i1 <= _ct162); _i1 = (_i1 + 1))
        {
          Iyy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = (Iy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] * Iy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]);
        }
      }
      int  _ct164 = ((R < ((16 * _T_i0) + 23))? R: ((16 * _T_i0) + 23));
      int  _ct165 = ((1 > ((16 * _T_i0) + 2))? 1: ((16 * _T_i0) + 2));
      for (int  _i0 = _ct165; (_i0 <= _ct164); _i0 = (_i0 + 1))
      {
        int  _ct166 = ((C < ((128 * _T_i1) + 135))? C: ((128 * _T_i1) + 135));
        int  _ct167 = ((1 > ((128 * _T_i1) + 2))? 1: ((128 * _T_i1) + 2));
        #pragma ivdep
        for (int  _i1 = _ct167; (_i1 <= _ct166); _i1 = (_i1 + 1))
        {
          Ixy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = (Ix[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] * Iy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]);
        }
      }
      int  _ct168 = (((R - 1) < ((16 * _T_i0) + 22))? (R - 1): ((16 * _T_i0) + 22));
      int  _ct169 = ((2 > ((16 * _T_i0) + 3))? 2: ((16 * _T_i0) + 3));
      for (int  _i0 = _ct169; (_i0 <= _ct168); _i0 = (_i0 + 1))
      {
        int  _ct170 = (((C - 1) < ((128 * _T_i1) + 134))? (C - 1): ((128 * _T_i1) + 134));
        int  _ct171 = ((2 > ((128 * _T_i1) + 3))? 2: ((128 * _T_i1) + 3));
        #pragma ivdep
        for (int  _i1 = _ct171; (_i1 <= _ct170); _i1 = (_i1 + 1))
        {
          Syy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((((((Iyy[(-1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] + Iyy[(-1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Iyy[(-1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]) + Iyy[((-16 * _T_i0) + _i0)][(-1 + ((-128 * _T_i1) + _i1))]) + Iyy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]) + Iyy[((-16 * _T_i0) + _i0)][(1 + ((-128 * _T_i1) + _i1))]) + Iyy[(1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))]) + Iyy[(1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Iyy[(1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]);
        }
      }
      int  _ct172 = (((R - 1) < ((16 * _T_i0) + 22))? (R - 1): ((16 * _T_i0) + 22));
      int  _ct173 = ((2 > ((16 * _T_i0) + 3))? 2: ((16 * _T_i0) + 3));
      for (int  _i0 = _ct173; (_i0 <= _ct172); _i0 = (_i0 + 1))
      {
        int  _ct174 = (((C - 1) < ((128 * _T_i1) + 134))? (C - 1): ((128 * _T_i1) + 134));
        int  _ct175 = ((2 > ((128 * _T_i1) + 3))? 2: ((128 * _T_i1) + 3));
        #pragma ivdep
        for (int  _i1 = _ct175; (_i1 <= _ct174); _i1 = (_i1 + 1))
        {
          Sxy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((((((Ixy[(-1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] + Ixy[(-1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Ixy[(-1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]) + Ixy[((-16 * _T_i0) + _i0)][(-1 + ((-128 * _T_i1) + _i1))]) + Ixy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]) + Ixy[((-16 * _T_i0) + _i0)][(1 + ((-128 * _T_i1) + _i1))]) + Ixy[(1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))]) + Ixy[(1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Ixy[(1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]);
        }
      }
      int  _ct176 = (((R - 1) < ((16 * _T_i0) + 22))? (R - 1): ((16 * _T_i0) + 22));
      int  _ct177 = ((2 > ((16 * _T_i0) + 3))? 2: ((16 * _T_i0) + 3));
      for (int  _i0 = _ct177; (_i0 <= _ct176); _i0 = (_i0 + 1))
      {
        int  _ct178 = (((C - 1) < ((128 * _T_i1) + 134))? (C - 1): ((128 * _T_i1) + 134));
        int  _ct179 = ((2 > ((128 * _T_i1) + 3))? 2: ((128 * _T_i1) + 3));
        #pragma ivdep
        for (int  _i1 = _ct179; (_i1 <= _ct178); _i1 = (_i1 + 1))
        {
          Sxx[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((((((((Ixx[(-1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))] + Ixx[(-1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Ixx[(-1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]) + Ixx[((-16 * _T_i0) + _i0)][(-1 + ((-128 * _T_i1) + _i1))]) + Ixx[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]) + Ixx[((-16 * _T_i0) + _i0)][(1 + ((-128 * _T_i1) + _i1))]) + Ixx[(1 + ((-16 * _T_i0) + _i0))][(-1 + ((-128 * _T_i1) + _i1))]) + Ixx[(1 + ((-16 * _T_i0) + _i0))][((-128 * _T_i1) + _i1)]) + Ixx[(1 + ((-16 * _T_i0) + _i0))][(1 + ((-128 * _T_i1) + _i1))]);
        }
      }
      int  _ct180 = (((R - 1) < ((16 * _T_i0) + 21))? (R - 1): ((16 * _T_i0) + 21));
      int  _ct181 = ((2 > ((16 * _T_i0) + 4))? 2: ((16 * _T_i0) + 4));
      for (int  _i0 = _ct181; (_i0 <= _ct180); _i0 = (_i0 + 1))
      {
        int  _ct182 = (((C - 1) < ((128 * _T_i1) + 133))? (C - 1): ((128 * _T_i1) + 133));
        int  _ct183 = ((2 > ((128 * _T_i1) + 4))? 2: ((128 * _T_i1) + 4));
        #pragma ivdep
        for (int  _i1 = _ct183; (_i1 <= _ct182); _i1 = (_i1 + 1))
        {
          det[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = ((Sxx[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] * Syy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]) - (Sxy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] * Sxy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]));
        }
      }
      int  _ct184 = (((R - 1) < ((16 * _T_i0) + 21))? (R - 1): ((16 * _T_i0) + 21));
      int  _ct185 = ((2 > ((16 * _T_i0) + 4))? 2: ((16 * _T_i0) + 4));
      for (int  _i0 = _ct185; (_i0 <= _ct184); _i0 = (_i0 + 1))
      {
        int  _ct186 = (((C - 1) < ((128 * _T_i1) + 133))? (C - 1): ((128 * _T_i1) + 133));
        int  _ct187 = ((2 > ((128 * _T_i1) + 4))? 2: ((128 * _T_i1) + 4));
        #pragma ivdep
        for (int  _i1 = _ct187; (_i1 <= _ct186); _i1 = (_i1 + 1))
        {
          trace[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] = (Sxx[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] + Syy[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]);
        }
      }
      int  _ct188 = (((R - 1) < ((16 * _T_i0) + 20))? (R - 1): ((16 * _T_i0) + 20));
      int  _ct189 = ((2 > ((16 * _T_i0) + 5))? 2: ((16 * _T_i0) + 5));
      for (int  _i0 = _ct189; (_i0 <= _ct188); _i0 = (_i0 + 1))
      {
        int  _ct190 = (((C - 1) < ((128 * _T_i1) + 132))? (C - 1): ((128 * _T_i1) + 132));
        int  _ct191 = ((2 > ((128 * _T_i1) + 5))? 2: ((128 * _T_i1) + 5));
        #pragma ivdep
        for (int  _i1 = _ct191; (_i1 <= _ct190); _i1 = (_i1 + 1))
        {
          harris[((_i0 * (2 + C)) + _i1)] = (((det[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)] - ((0.04f * trace[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)]) * trace[((-16 * _T_i0) + _i0)][((-128 * _T_i1) + _i1)])) * 65536.0f) * 256.0f);
        }
      }
    }
  }
}

