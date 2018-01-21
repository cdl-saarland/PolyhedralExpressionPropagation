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
  EVENT SHALL Ravi Teja Mullapudi, Vinay Vasista, CSA Indian Institute of
  Science BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <cmath>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define isl_min(x, y) ((x) < (y) ? (x) : (y))
#define isl_max(x, y) ((x) > (y) ? (x) : (y))
#define isl_floord(n, d) (((n) < 0) ? -((-(n) + (d)-1) / (d)) : (n) / (d))
extern "C" void pipeline_filtered(long C, long R, void *input_void_arg,
                                  void *filtered_void_arg) {
  float *input;
  input = (float *)(input_void_arg);
  float *filtered;
  filtered = (float *)(filtered_void_arg);
  float *grid;
  grid = (float *)(malloc(
      (sizeof(float) * (((2 * (((((R - 1) / 8) - 1) - 1) + 1)) *
                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                        15))));
  float *blurz;
  blurz = (float *)(malloc(
      (sizeof(float) * (((2 * (((((R - 1) / 8) - 1) - 1) + 1)) *
                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                        11))));
  float *blurx;
  blurx = (float *)(malloc(
      (sizeof(float) * (((2 * (((((R - 1) / 8) - 3) - 3) + 1)) *
                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                        11))));
  float *blury;
  blury = (float *)(malloc(
      (sizeof(float) * (((2 * (((((R - 1) / 8) - 3) - 3) + 1)) *
                         (((((C - 1) / 8) - 3) - 3) + 1)) *
                        11))));
  float *interpolated;
  interpolated =
      (float *)(malloc((sizeof(float) * ((2 * (-56 + R)) * (-56 + C)))));
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1)) {
    for (long _i1 = 1; (_i1 <= 325); _i1 = (_i1 + 1)) {
      for (long _i2 = 1; (_i2 <= 197); _i2 = (_i2 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long _i3 = 0; (_i3 <= 14); _i3 = (_i3 + 1)) {
          grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                           (((((C - 1) / 8) - 1) - 1) + 1)) *
                          15)) +
                  ((_i1 - 1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                 ((_i2 - 1) * 15)) +
                _i3)] = 0;
        }
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long _i0 = 1; (_i0 <= 325); _i0 = (_i0 + 1)) {
    for (long _i1 = 1; (_i1 <= 197); _i1 = (_i1 + 1)) {
      for (long _i2 = 0; (_i2 <= 8); _i2 = (_i2 + 1)) {
        for (long _i3 = 0; (_i3 <= 8); _i3 = (_i3 + 1)) {
          grid[((((0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                        15)) +
                  ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                 ((-1 + _i1) * 15)) +
                (2 + (long)(((input[((((-4 + (8 * _i0)) + _i2) * C) +
                                     ((-4 + (8 * _i1)) + _i3))] *
                              10) +
                             0.5f))))] =
              (grid[((((0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                              (((((C - 1) / 8) - 1) - 1) + 1)) *
                             15)) +
                       ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                      ((-1 + _i1) * 15)) +
                     (2 + (long)(((input[((((-4 + (8 * _i0)) + _i2) * C) +
                                          ((-4 + (8 * _i1)) + _i3))] *
                                   10) +
                                  0.5f))))] +
               input[((((-4 + (8 * _i0)) + _i2) * C) +
                      ((-4 + (8 * _i1)) + _i3))]);
          grid[((((1 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                        15)) +
                  ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                 ((-1 + _i1) * 15)) +
                (2 + (long)(((input[((((-4 + (8 * _i0)) + _i2) * C) +
                                     ((-4 + (8 * _i1)) + _i3))] *
                              10) +
                             0.5f))))] =
              (grid[((((1 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                              (((((C - 1) / 8) - 1) - 1) + 1)) *
                             15)) +
                       ((-1 + _i0) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                      ((-1 + _i1) * 15)) +
                     (2 + (long)(((input[((((-4 + (8 * _i0)) + _i2) * C) +
                                          ((-4 + (8 * _i1)) + _i3))] *
                                   10) +
                                  0.5f))))] +
               1);
        }
      }
    }
  }
  for (long _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long _i1 = 1; (_i1 <= 325); _i1 = (_i1 + 1)) {
      for (long _i2 = 1; (_i2 <= 197); _i2 = (_i2 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
        #endif
        for (long _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1)) {
          blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                            (((((C - 1) / 8) - 1) - 1) + 1)) *
                           11)) +
                   ((_i1 - 1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                  ((_i2 - 1) * 11)) +
                 (_i3 - 2))] =
              ((((grid[(
                      (((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                 (((((C - 1) / 8) - 1) - 1) + 1)) *
                                15)) +
                        ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                       ((-1 + _i2) * 15)) +
                      (-2 + _i3))] +
                  (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                        (((((C - 1) / 8) - 1) - 1) + 1)) *
                                       15)) +
                               ((-1 + _i1) *
                                ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                              ((-1 + _i2) * 15)) +
                             (-1 + _i3))])) +
                 (6 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                       (((((C - 1) / 8) - 1) - 1) + 1)) *
                                      15)) +
                              ((-1 + _i1) *
                               ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                             ((-1 + _i2) * 15)) +
                            _i3)])) +
                (4 * grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                      (((((C - 1) / 8) - 1) - 1) + 1)) *
                                     15)) +
                             ((-1 + _i1) *
                              ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                            ((-1 + _i2) * 15)) +
                           (1 + _i3))])) +
               grid[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                (((((C - 1) / 8) - 1) - 1) + 1)) *
                               15)) +
                       ((-1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 15))) +
                      ((-1 + _i2) * 15)) +
                     (2 + _i3))]);
        }
      }
    }
  }
  for (long _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long _i1 = 3; (_i1 <= 323); _i1 = (_i1 + 1)) {
      for (long _i2 = 1; (_i2 <= 197); _i2 = (_i2 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1)) {
          blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                            (((((C - 1) / 8) - 1) - 1) + 1)) *
                           11)) +
                   ((_i1 - 3) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                  ((_i2 - 1) * 11)) +
                 (_i3 - 2))] =
              ((((blurz[(
                      (((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                 (((((C - 1) / 8) - 1) - 1) + 1)) *
                                11)) +
                        ((-3 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                       ((-1 + _i2) * 11)) +
                      (-2 + _i3))] +
                  (4 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                                        11)) +
                                ((-2 + _i1) *
                                 ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                               ((-1 + _i2) * 11)) +
                              (-2 + _i3))])) +
                 (6 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                        (((((C - 1) / 8) - 1) - 1) + 1)) *
                                       11)) +
                               ((-1 + _i1) *
                                ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                              ((-1 + _i2) * 11)) +
                             (-2 + _i3))])) +
                (4 * blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                       (((((C - 1) / 8) - 1) - 1) + 1)) *
                                      11)) +
                              (_i1 * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                             ((-1 + _i2) * 11)) +
                            (-2 + _i3))])) +
               blurz[((((_i0 * (((((((R - 1) / 8) - 1) - 1) + 1) *
                                 (((((C - 1) / 8) - 1) - 1) + 1)) *
                                11)) +
                        ((1 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                       ((-1 + _i2) * 11)) +
                      (-2 + _i3))]);
        }
      }
    }
  }
  for (long _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long _i1 = 3; (_i1 <= 323); _i1 = (_i1 + 1)) {
      for (long _i2 = 3; (_i2 <= 195); _i2 = (_i2 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
        #pragma ivdep
#endif
        for (long _i3 = 2; (_i3 <= 12); _i3 = (_i3 + 1)) {
          blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                            (((((C - 1) / 8) - 3) - 3) + 1)) *
                           11)) +
                   ((_i1 - 3) * ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                  ((_i2 - 3) * 11)) +
                 (_i3 - 2))] =
              ((((blurx[(
                      (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                 (((((C - 1) / 8) - 1) - 1) + 1)) *
                                11)) +
                        ((-3 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                       ((-3 + _i2) * 11)) +
                      (-2 + _i3))] +
                  (4 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                         (((((C - 1) / 8) - 1) - 1) + 1)) *
                                        11)) +
                                ((-3 + _i1) *
                                 ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                               ((-2 + _i2) * 11)) +
                              (-2 + _i3))])) +
                 (6 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                        (((((C - 1) / 8) - 1) - 1) + 1)) *
                                       11)) +
                               ((-3 + _i1) *
                                ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                              ((-1 + _i2) * 11)) +
                             (-2 + _i3))])) +
                (4 * blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                       (((((C - 1) / 8) - 1) - 1) + 1)) *
                                      11)) +
                              ((-3 + _i1) *
                               ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                             (_i2 * 11)) +
                            (-2 + _i3))])) +
               blurx[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                 (((((C - 1) / 8) - 1) - 1) + 1)) *
                                11)) +
                        ((-3 + _i1) * ((((((C - 1) / 8) - 1) - 1) + 1) * 11))) +
                       ((1 + _i2) * 11)) +
                      (-2 + _i3))]);
        }
      }
    }
  }
  for (long _i0 = 0; (_i0 <= 1); _i0 = (_i0 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma omp parallel for schedule(static)
#endif
    for (long _i1 = 24; (_i1 <= 2583); _i1 = (_i1 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
      #pragma ivdep
#endif
      for (long _i2 = 24; (_i2 <= 1559); _i2 = (_i2 + 1)) {
        interpolated[(
            ((_i0 * ((-56 + R) * (-56 + C))) + ((_i1 - 24) * (-56 + C))) +
            (_i2 - 24))] =
            (((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                 (((((C - 1) / 8) - 3) - 3) + 1)) *
                                11)) +
                        (((_i1 / 8) - 3) *
                         ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                       (((_i2 / 8) - 3) * 11)) +
                      ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
               (((float)((_i1 % 8)) / 8) *
                (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                   (((((C - 1) / 8) - 3) - 3) + 1)) *
                                  11)) +
                          ((((_i1 / 8) + 1) - 3) *
                           ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                         (((_i2 / 8) - 3) * 11)) +
                        ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                 blury[(
                     (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                (((((C - 1) / 8) - 3) - 3) + 1)) *
                               11)) +
                       (((_i1 / 8) - 3) *
                        ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                      (((_i2 / 8) - 3) * 11)) +
                     ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))]))) +
              (((float)((_i2 % 8)) / 8) *
               ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                   (((((C - 1) / 8) - 3) - 3) + 1)) *
                                  11)) +
                          (((_i1 / 8) - 3) *
                           ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                         ((((_i2 / 8) + 1) - 3) * 11)) +
                        ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
                 (((float)((_i1 % 8)) / 8) *
                  (blury[(
                       (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                  (((((C - 1) / 8) - 3) - 3) + 1)) *
                                 11)) +
                         ((((_i1 / 8) + 1) - 3) *
                          ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                        ((((_i2 / 8) + 1) - 3) * 11)) +
                       ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                   blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           ((((_i2 / 8) + 1) - 3) * 11)) +
                          ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) -
                           2))]))) -
                (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                   (((((C - 1) / 8) - 3) - 3) + 1)) *
                                  11)) +
                          (((_i1 / 8) - 3) *
                           ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                         (((_i2 / 8) - 3) * 11)) +
                        ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
                 (((float)((_i1 % 8)) / 8) *
                  (blury[(
                       (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                  (((((C - 1) / 8) - 3) - 3) + 1)) *
                                 11)) +
                         ((((_i1 / 8) + 1) - 3) *
                          ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                        (((_i2 / 8) - 3) * 11)) +
                       ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                   blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           (((_i2 / 8) - 3) * 11)) +
                          ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) -
                           2))])))))) +
             (((input[((_i1 * C) + _i2)] * 10) -
               (long)((input[((_i1 * C) + _i2)] * 10))) *
              (((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                   (((((C - 1) / 8) - 3) - 3) + 1)) *
                                  11)) +
                          (((_i1 / 8) - 3) *
                           ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                         (((_i2 / 8) - 3) * 11)) +
                        (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                         2))] +
                 (((float)((_i1 % 8)) / 8) *
                  (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            ((((_i1 / 8) + 1) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           (((_i2 / 8) - 3) * 11)) +
                          (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                           2))] -
                   blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           (((_i2 / 8) - 3) * 11)) +
                          (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                           2))]))) +
                (((float)((_i2 % 8)) / 8) *
                 ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           ((((_i2 / 8) + 1) - 3) * 11)) +
                          (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                           2))] +
                   (((float)((_i1 % 8)) / 8) *
                    (blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           ((((_i1 / 8) + 1) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          ((((_i2 / 8) + 1) - 3) * 11)) +
                         (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                          2))] -
                     blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           (((_i1 / 8) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          ((((_i2 / 8) + 1) - 3) * 11)) +
                         (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                          2))]))) -
                  (blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           (((_i2 / 8) - 3) * 11)) +
                          (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                           2))] +
                   (((float)((_i1 % 8)) / 8) *
                    (blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           ((((_i1 / 8) + 1) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          (((_i2 / 8) - 3) * 11)) +
                         (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                          2))] -
                     blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           (((_i1 / 8) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          (((_i2 / 8) - 3) * 11)) +
                         (((2 + (long)((input[((_i1 * C) + _i2)] * 10))) + 1) -
                          2))])))))) -
               ((blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                   (((((C - 1) / 8) - 3) - 3) + 1)) *
                                  11)) +
                          (((_i1 / 8) - 3) *
                           ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                         (((_i2 / 8) - 3) * 11)) +
                        ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
                 (((float)((_i1 % 8)) / 8) *
                  (blury[(
                       (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                  (((((C - 1) / 8) - 3) - 3) + 1)) *
                                 11)) +
                         ((((_i1 / 8) + 1) - 3) *
                          ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                        (((_i2 / 8) - 3) * 11)) +
                       ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                   blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                     (((((C - 1) / 8) - 3) - 3) + 1)) *
                                    11)) +
                            (((_i1 / 8) - 3) *
                             ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                           (((_i2 / 8) - 3) * 11)) +
                          ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) -
                           2))]))) +
                (((float)((_i2 % 8)) / 8) *
                 ((blury[(
                       (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                  (((((C - 1) / 8) - 3) - 3) + 1)) *
                                 11)) +
                         (((_i1 / 8) - 3) *
                          ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                        ((((_i2 / 8) + 1) - 3) * 11)) +
                       ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
                   (((float)((_i1 % 8)) / 8) *
                    (blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           ((((_i1 / 8) + 1) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          ((((_i2 / 8) + 1) - 3) * 11)) +
                         ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                     blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                       (((((C - 1) / 8) - 3) - 3) + 1)) *
                                      11)) +
                              (((_i1 / 8) - 3) *
                               ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                             ((((_i2 / 8) + 1) - 3) * 11)) +
                            ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) -
                             2))]))) -
                  (blury[(
                       (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                  (((((C - 1) / 8) - 3) - 3) + 1)) *
                                 11)) +
                         (((_i1 / 8) - 3) *
                          ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                        (((_i2 / 8) - 3) * 11)) +
                       ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] +
                   (((float)((_i1 % 8)) / 8) *
                    (blury[(
                         (((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                    (((((C - 1) / 8) - 3) - 3) + 1)) *
                                   11)) +
                           ((((_i1 / 8) + 1) - 3) *
                            ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                          (((_i2 / 8) - 3) * 11)) +
                         ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) - 2))] -
                     blury[((((_i0 * (((((((R - 1) / 8) - 3) - 3) + 1) *
                                       (((((C - 1) / 8) - 3) - 3) + 1)) *
                                      11)) +
                              (((_i1 / 8) - 3) *
                               ((((((C - 1) / 8) - 3) - 3) + 1) * 11))) +
                             (((_i2 / 8) - 3) * 11)) +
                            ((2 + (long)((input[((_i1 * C) + _i2)] * 10))) -
                             2))])))))))));
      }
    }
  }
#ifdef ENABLE_OMP_PRAGMAS
  #pragma omp parallel for schedule(static)
#endif
  for (long _i1 = 24; (_i1 <= 2583); _i1 = (_i1 + 1)) {
#ifdef ENABLE_OMP_PRAGMAS
    #pragma ivdep
#endif
    for (long _i2 = 24; (_i2 <= 1559); _i2 = (_i2 + 1)) {
      float _ct0 = (interpolated[(((0 * ((-56 + R) * (-56 + C))) +
                                   ((-24 + _i1) * (-56 + C))) +
                                  (-24 + _i2))] /
                    interpolated[(((1 * ((-56 + R) * (-56 + C))) +
                                   ((-24 + _i1) * (-56 + C))) +
                                  (-24 + _i2))]);
      float _ct1 = input[((_i1 * C) + _i2)];
      float _ct2 = ((interpolated[(((1 * ((-56 + R) * (-56 + C))) +
                                    ((-24 + _i1) * (-56 + C))) +
                                   (-24 + _i2))] > 0)
                        ? _ct0
                        : _ct1);
      filtered[((0 + ((_i1 - 24) * (-56 + C))) + (_i2 - 24))] = _ct2;
    }
  }
  free(grid);
  free(blurz);
  free(blurx);
  free(blury);
  free(interpolated);
}
