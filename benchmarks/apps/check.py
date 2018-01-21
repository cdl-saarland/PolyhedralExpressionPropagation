import numpy as np
import cv2
import sys

sys.path.insert(0, '../')
# initialize the app. parameters
def initVars(args):
    if len(args) < 4 :
        print "Usage:\n" + sys.argv[0] + \
              " base_image result_image tolerance"
        sys.exit(1)
    else :
        base = cv2.imread(args[1], cv2.CV_LOAD_IMAGE_COLOR)
        result = cv2.imread(args[2], cv2.CV_LOAD_IMAGE_COLOR)
        tol = float(args[3])

    return base, result, tol

#----------------------------------------------------------------------

base = ''
result = ''
tol = 0
# initialize
base, result, tol = initVars(sys.argv)
# normalizing images to speed up later computations
base = cv2.normalize(base)
result = cv2.normalize(result)

if(np.allclose(base,result, atol=tol)):
    print "SUCCESS"
else:
    print "FAILED"
