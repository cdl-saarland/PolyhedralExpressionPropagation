#!/usr/bin/python
import sys
import os
import argparse
import numpy as np

stream_sizes = list(range(0,65,4))#[2, 4, 8, 16, 32, 40, 48, 64]
stream_sizes[0] = 2
apps = ["harris", "bilateral_grid", "unsharp_mask", "camera_pipe",
        "interpolate", "pyramid_blending", "local_laplacian"]

def initialize_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apps", help="Comma separated list of apps to evaluate.", default='all')
    parser.add_argument("--exclude_apps", help="Comma separated list of apps to exclude.", default='')
    parser.add_argument("nruns", help="Number of runs for each benchmark for this target.", type = int)
    return parser

parser = initialize_parser()
args = parser.parse_args()

ROOT_PATH = os.getcwd()
init_nruns = args.nruns
error_margin = 0.05

if args.apps != 'all':
    arg_apps = args.apps.split(',')
    apps = [app for app in apps if app in arg_apps]

apps = [app for app in apps if app not in args.exclude_apps]
# def required_sample_size(app, C):

# def compute_conf_int(times, app):

def compute_speedup(times):
# for now we just compute based on the median value from the scores 
# for every target. 
    result = {}
    for stream_size in stream_sizes:
        result[stream_size] = round(np.median(times[stream_size]), 3)
    return result

bar_graph_data = {}
for app in apps:
    print("Generating graph data file for app:  ", app) 
    times = {}
    for stream_size in stream_sizes:
        # print("Processing stream_size:  ", stream_size) 
        data_path = os.path.join(ROOT_PATH, "evaluation","stream", app)
        data_file = os.path.join(data_path,  "%d-%d.csv" % (stream_size, init_nruns))
        times[stream_size] = np.loadtxt(data_file)
    time_values = compute_speedup(times)
    bar_graph_data[app] = time_values

graph_data_file = os.path.join(ROOT_PATH, "splot-data.csv")
with open(graph_data_file, 'w') as of:
    apps_list = [app.replace('_',' ') for app in apps]
    of.write("stream_sizes,%s\n" % ",".join(apps_list))
    for stream_size in stream_sizes:
        of.write(str(stream_size))
        for app in apps:
            of.write(',' + str(round(bar_graph_data[app][stream_size], 3)))
        of.write('\n')
