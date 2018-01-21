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
extern "C" void  pipeline_harris(int  C, int  R, void * img_void_arg, void * harris_void_arg)
{
  float * img;
  img = (float *) (img_void_arg);
  float * harris;
  harris = (float *) (harris_void_arg);
  #pragma omp parallel for schedule(static)
  for (int  _T_i0 = -1; (_T_i0 <= (R / 32)); _T_i0 = (_T_i0 + 1))
  {
    float  Iy[34][258];
    float  Ix[34][258];
    for (int  _T_i1 = -1; (_T_i1 <= (C / 256)); _T_i1 = (_T_i1 + 1))
    {
      int  _ct0 = ((R < ((32 * _T_i0) + 33))? R: ((32 * _T_i0) + 33));
      int  _ct1 = ((1 > (32 * _T_i0))? 1: (32 * _T_i0));
      for (int  _i0 = _ct1; (_i0 <= _ct0); _i0 = (_i0 + 1))
      {
        int  _ct2 = ((C < ((256 * _T_i1) + 257))? C: ((256 * _T_i1) + 257));
        int  _ct3 = ((1 > (256 * _T_i1))? 1: (256 * _T_i1));
        #pragma ivdep
        for (int  _i1 = _ct3; (_i1 <= _ct2); _i1 = (_i1 + 1))
        {
          Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] = ((((((img[(((-1 + _i0) * (C + 2)) + (-1 + _i1))] * -0.0833333333333f) + (img[(((-1 + _i0) * (C + 2)) + (1 + _i1))] * 0.0833333333333f)) + (img[((_i0 * (C + 2)) + (-1 + _i1))] * -0.166666666667f)) + (img[((_i0 * (C + 2)) + (1 + _i1))] * 0.166666666667f)) + (img[(((1 + _i0) * (C + 2)) + (-1 + _i1))] * -0.0833333333333f)) + (img[(((1 + _i0) * (C + 2)) + (1 + _i1))] * 0.0833333333333f));
        }
      }
      int  _ct4 = ((R < ((32 * _T_i0) + 33))? R: ((32 * _T_i0) + 33));
      int  _ct5 = ((1 > (32 * _T_i0))? 1: (32 * _T_i0));
      for (int  _i0 = _ct5; (_i0 <= _ct4); _i0 = (_i0 + 1))
      {
        int  _ct6 = ((C < ((256 * _T_i1) + 257))? C: ((256 * _T_i1) + 257));
        int  _ct7 = ((1 > (256 * _T_i1))? 1: (256 * _T_i1));
        #pragma ivdep
        for (int  _i1 = _ct7; (_i1 <= _ct6); _i1 = (_i1 + 1))
        {
          Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] = ((((((img[(((-1 + _i0) * (C + 2)) + (-1 + _i1))] * -0.0833333333333f) + (img[(((1 + _i0) * (C + 2)) + (-1 + _i1))] * 0.0833333333333f)) + (img[(((-1 + _i0) * (C + 2)) + _i1)] * -0.166666666667f)) + (img[(((1 + _i0) * (C + 2)) + _i1)] * 0.166666666667f)) + (img[(((-1 + _i0) * (C + 2)) + (1 + _i1))] * -0.0833333333333f)) + (img[(((1 + _i0) * (C + 2)) + (1 + _i1))] * 0.0833333333333f));
        }
      }
      int  _ct8 = (((R - 1) < ((32 * _T_i0) + 32))? (R - 1): ((32 * _T_i0) + 32));
      int  _ct9 = ((2 > ((32 * _T_i0) + 1))? 2: ((32 * _T_i0) + 1));
      for (int  _i0 = _ct9; (_i0 <= _ct8); _i0 = (_i0 + 1))
      {
        int  _ct10 = (((C - 1) < ((256 * _T_i1) + 256))? (C - 1): ((256 * _T_i1) + 256));
        int  _ct11 = ((2 > ((256 * _T_i1) + 1))? 2: ((256 * _T_i1) + 1));
        #pragma ivdep
        for (int  _i1 = _ct11; (_i1 <= _ct10); _i1 = (_i1 + 1))
        {
          harris[((_i0 * (2 + C)) + _i1)] = ((((((((((((Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) * (((((((((Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))]))) - ((((((((((Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) * (((((((((Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])))) - ((0.04f * ((((((((((Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (((((((((Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])))) * ((((((((((Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Ix[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Ix[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Ix[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Ix[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (((((((((Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))]) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(-1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(-1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)] * Iy[((-32 * _T_i0) + _i0)][((-256 * _T_i1) + _i1)])) + (Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))] * Iy[((-32 * _T_i0) + _i0)][(1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(-1 + ((-256 * _T_i1) + _i1))])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)] * Iy[(1 + ((-32 * _T_i0) + _i0))][((-256 * _T_i1) + _i1)])) + (Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))] * Iy[(1 + ((-32 * _T_i0) + _i0))][(1 + ((-256 * _T_i1) + _i1))])))));
        }
      }
    }
  }
}
