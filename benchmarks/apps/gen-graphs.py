#!/usr/bin/python
import sys
import os
import argparse
import numpy as np

targets = ["naive-parallel", "polly-prop-parallel", "polymage", "polymage-gcc", "polymage-icc", "polly-all-but-prop", "polly-parallel", "halide"]
apps = ["unsharp_mask", "harris", "bilateral_grid", "camera_pipe",
        "pyramid_blending", "interpolate", "local_laplacian"]

def initialize_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--apps", help="Comma separated list of apps to evaluate.", default='all')
    parser.add_argument("--targets", help="Comma separated list of targets to evaluate.", default='all')
    parser.add_argument("--exclude_apps", help="Comma separated list of apps to exclude.", default='')
    parser.add_argument("--exclude_targets", help="Comma separated list of targets to exclude.", default='')
    parser.add_argument("nruns", help="Number of runs for each benchmark for this target.", type = int)
    return parser

parser = initialize_parser()
args = parser.parse_args()

ROOT_PATH = os.getcwd()
init_nruns = args.nruns
error_margin = 0.05
graph_data_file = os.path.join(ROOT_PATH, "median_scores-" + str(init_nruns) + ".csv")

if args.apps != 'all':
    arg_apps = args.apps.split(',')
    apps = [app for app in apps if app in arg_apps]

if args.targets != 'all':
    arg_targets = args.targets.split(',')
    targets = [target for target in targets if target in arg_targets]

targets = [target for target in targets if target not in args.exclude_targets]
apps = [app for app in apps if app not in args.exclude_apps]
excluded_apps = args.exclude_apps.split(',')

# def required_sample_size(app, C):

# def compute_conf_int(times, app):

def compute_speedup(times):
# for now we just compute based on the median value from the scores
# for every target.
    result = {}
    for target in targets:
        if target == 'naive-parallel':
            continue
        if not times[target].any():
            result[target] = 0
            continue
        result[target] = round(np.median(times['naive-parallel'] / np.median(times[target])), 3)
    return result

def compute_median_scores(times):
    result = {}
    for target in targets:
        result[target] = round(np.median(times[target]), 3)
    return result

bar_graph_data = {}
for app in apps:
    print("Generating graph data file for app:  ", app)
    times = {}
    for target in targets:
        print("Processing target:  ", target)
        data_path = os.path.join(ROOT_PATH, "evaluation", target + "-" + str(init_nruns))
        data_file = os.path.join(data_path, app + '.csv')
        if not os.path.isfile(data_file):
            print("Skipping target %s since file not found in data directory" % target)
            times[target] = np.zeros(init_nruns) 
            continue
        times[target] = np.loadtxt(data_file)
    time_values = compute_median_scores(times)
    bar_graph_data[app] = time_values

with open(graph_data_file, 'w') as of:
    of.write("apps")
    for target in targets:
        # if target == 'naive-parallel':
            # continue
        of.write(',' + target)
    of.write('\n')

    for index,app in enumerate(apps):
        of.write(str(index + 1))
        time_values = bar_graph_data[app]
        for target in targets:
            # if target == 'naive-parallel':
                # continue
            of.write(',' + str(time_values[target]))
        of.write('\n')
    for app in excluded_apps:
        if app == '':
            continue
        of.write(app.replace('_',' '))
        for target in targets:
            if target == 'naive-parallel':
                continue
            of.write(',0.0') # initializing to default 0.0 for excluded apps
        of.write('\n')
