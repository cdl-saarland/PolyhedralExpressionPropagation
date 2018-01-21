'''
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
'''

import ctypes
import numpy as np
import time
import cv2
import sys
sys.path.insert(0, '../')
from utils import clock, initOptionals, \
                   printLine, image_clamp

# shared library, and function
libbilateral = ctypes.cdll.LoadLibrary("./bilateral_grid.so")
bilateral_grid = libbilateral.pipeline_filtered

# initialize the app. parameters
def initVars(image, nruns, rows, cols, args):
    if len(args) < 2 :
        print "Usage:\n" + args[0] + \
              " image [[nruns] [rows [cols]]]"
        sys.exit(1)
    else :
        image = cv2.imread(args[1], cv2.CV_LOAD_IMAGE_COLOR)
        optionalArgStart = 2

        # Optional Parameters
        # default
        nruns = 6
        rows, cols = 2560, 1536
        nruns, rows, cols = initOptionals(optionalArgStart, \
                                          nruns, rows, cols, args)

    return image, nruns, rows, cols

#-------------------------------------------------------------------

image = ''
nruns, rows, cols = 0, 0, 0

# initialize
image, nruns, rows, cols = initVars(\
    image, nruns, rows, cols, \
    sys.argv)

# Values set to
printLine()
print 'image: \"', sys.argv[1], '\"'
print 'nruns =', nruns,', rows =', rows,', cols =', cols

if rows != 2560 or cols != 1536:
    print "Please use 1536x2560 image size"
    sys.exit(0)

R = image.shape[0]
C = image.shape[1]

off_left = 24
total_pad = 56

# get image roi
row_base = (R - rows)/2
col_base = (C - cols)/2
image_region = image[row_base:row_base+rows, \
                     col_base:col_base+cols]

# create ghost zone and copy image roi
image_ghost = np.empty((rows+total_pad, cols+total_pad, 3), image_region.dtype)
image_ghost[off_left:rows+off_left, off_left:cols+off_left, 0:3] = \
    np.array(image_region[0:rows, 0:cols, 0:3], image_region.dtype)
# clamp the boundary portions
image_clamp(image_region, image_ghost, \
            rows, cols, 3, \
            image_region.dtype, 1, \
            off_left, total_pad)

# convert input image to grayscale floating point
gray = cv2.cvtColor(image_ghost, cv2.COLOR_BGR2GRAY)
gray = np.float32(gray) / 255.0

# result array
res = np.empty((rows, cols), np.float32)

# start timer
print "-------------------------------"
startTime = time.clock()
avgTime = 0
for run in range(1, nruns+1):
    frameStart = clock()
    # Compute
    bilateral_grid(ctypes.c_int(cols+total_pad), \
                   ctypes.c_int(rows+total_pad), \
                   ctypes.c_void_p(gray.ctypes.data), \
                   ctypes.c_void_p(res.ctypes.data))
    frameEnd = clock()
    frameTime = float(frameEnd) - float(frameStart)

    if run != 1:
        print "time taken: ", frameEnd*1000 - frameStart*1000, "ms"
        avgTime += frameTime
print "-------------------------------"
print "avg time: ", (avgTime/(nruns-1))*1000, "ms"
print "-------------------------------"

# input image roi
imgShow = image[row_base:row_base+rows, col_base:col_base+cols]

# convert to grayscale floating point
gray = cv2.cvtColor(imgShow, cv2.COLOR_BGR2GRAY)
gray = np.float32(gray) / 255.0

cv2.imwrite("result.jpg", res*255.0);
# Display
# cv2.namedWindow("input", cv2.WINDOW_NORMAL)
# cv2.namedWindow("output", cv2.WINDOW_NORMAL)
# ch = 0xFF
# while [ ch != ord('q') ]:
    # ch = 0xFF & cv2.waitKey(1)
    # cv2.imshow("input", gray)
    # cv2.imshow("output", res)
    # if ch == ord('q'):
        # break

# cv2.destroyAllWindows()
