#!/usr/bin/python
import sys
import os
import subprocess
import numpy as np
import argparse
import re

commands = {}
times = {}
apps = ["harris", "bilateral_grid", "unsharp_mask", "camera_pipe",
        "interpolate", "pyramid_blending", "local_laplacian"]

commands["bilateral_grid"] = "python2 main.py ../../images/rgb.png 2"
commands["harris"] = "python2 main.py ../../images/grand_canyon2.jpg 2"
commands["camera_pipe"] = "python2 main.py 3700 50 2.0 2"
commands["interpolate"] = "python2 main.py ../../images/rgb.png ../../images/alpha.png 2"
commands["local_laplacian"] = "python2 main.py ../../images/rgb.png 1 1 2"
commands["pyramid_blending"] = "python2 main.py ../../images/grand_canyon1.jpg ../../images/grand_canyon2.jpg 2 2048 2048"
commands["unsharp_mask"] = "python2 main.py ../../images/grand_canyon2.jpg 0.001 3 2 2048 2048"

def initialize_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("target", help="Target that is currently built using make for which you need to collect the data.")
    parser.add_argument("nruns", help="Number of runs for each benchmark for this target.", type = int)
    parser.add_argument("--apps", help="Comma separated list of apps to evaluate.", default='all')
    parser.add_argument("--stream_size", help="Store data in the streaming directory. Needed for streaming tests.", type=int, default = -1)
    return parser

parser = initialize_parser()
args = parser.parse_args()

ROOT_PATH = os.getcwd()
init_nruns = args.nruns
error_margin = 0.05


def compute_timescores(app, nruns):
    for run in range(0, nruns):
        result = subprocess.run(commands[app].split(), stdout=subprocess.PIPE).stdout.decode('utf-8') 
        # print(result)
        time_str = re.search("time taken: (.*) ms\n", result).group(1)
        print(time_str)
        times[app].append(float(time_str))   
        if((run + 1) % 50 == 0):
            print("\t\tCompleted processing ", run + 1, "runs.")

if args.apps != 'all':
    arg_apps = args.apps.split(',')
    apps = [app for app in apps if app in arg_apps]

for app in apps:
    times[app] = []
    print("Generating data for app:  ",app, "\n") 
    dir_path = os.path.join(ROOT_PATH, app)
    os.chdir(dir_path)
    compute_timescores(app, init_nruns)
    if (args.stream_size < 0):
        data_path = os.path.join(ROOT_PATH, "evaluation", args.target + "-" + str(init_nruns))
        if not os.path.exists(data_path):
            os.makedirs(data_path)
        os.chdir(data_path)
        np.savetxt(app + ".csv", np.asarray(times[app]), delimiter=",")
    else:
        data_path = os.path.join(ROOT_PATH, "evaluation","stream", app) 
        if not os.path.exists(data_path):
            os.makedirs(data_path)
        os.chdir(data_path)
        np.savetxt("%d-%d.csv" %(args.stream_size, init_nruns), np.asarray(times[app]), delimiter=",")
