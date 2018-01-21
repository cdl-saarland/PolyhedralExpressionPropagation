#!/usr/bin/python
import sys
import os
import numpy as np
import argparse
import shutil

make_commands = {}
targets = ["naive-parallel", "polly-prop-parallel", "polymage", "polymage-gcc", "polymage-icc", "polly-all-but-prop", "polly-parallel"]
apps = ["harris", "bilateral_grid", "unsharp_mask", "camera_pipe",
        "interpolate", "pyramid_blending", "local_laplacian"]
stream_sizes = list(range(0,65,4))#[2, 4, 8, 16, 32, 40, 48, 64]
stream_sizes[0] = 2

make_commands["naive-parallel"] = "make naive-parallel"
make_commands["polly-prop-parallel"] = "make polly-prop-parallel"
make_commands["polymage"] = "make polymage"
make_commands["polly-all-but-prop"] = "make polly-prop-parallel EXTRA_FLAGS=\"-mllvm -polly-prop-all-but-prop\""

def initialize_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("nruns", help="Number of runs for each benchmark.", type = int)
    parser.add_argument("--targets", help="Comma separated list of targets to evaluate.", default='all')
    parser.add_argument("--exclude_targets", help="Comma separated list of targets to exclude.", default='')
    parser.add_argument("--apps", help="Comma separated list of apps to evaluate.", default='all')
    parser.add_argument("--exclude_apps", help="Comma separated list of apps to exclude.", default='')
    parser.add_argument("--build_root", help="Build directory root path(default:$pwd/build)",
            default='')
    parser.add_argument("--stream", help="Perform streaming tests.", action='store_true', default=False)
    return parser

parser = initialize_parser()
args = parser.parse_args()

ROOT_PATH = os.getcwd()
BUILD_ROOT= os.path.join(ROOT_PATH,"build") if args.build_root == '' else args.build_root
init_nruns = args.nruns
error_margin = 0.05

if args.apps != 'all':
    arg_apps = args.apps.split(',')
    apps = [app for app in apps if app in arg_apps]

if args.targets != 'all':
    arg_targets = args.targets.split(',')
    targets = [target for target in targets if target in arg_targets]

targets = [target for target in targets if target not in args.exclude_targets]
apps = [app for app in apps if app not in args.exclude_apps]

print("Apps in processing list:  " , ", ".join(apps))
print("Targets in processing list:  " , ", ".join(targets))

for app in apps:
    dir_path = os.path.join(ROOT_PATH, app)
    build_path = os.path.join(BUILD_ROOT,app)
    if not os.path.exists(build_path):
        os.makedirs(build_path)
    if (not args.stream):
        print("Performing speedup tests for app: ", app)
        for target in targets:
            print("Processing target: ", target)
            src_file = os.path.join(build_path, "%s-%d.so" % (target, 32))
            if not os.path.isfile(src_file):
                print("Skipping target %s since file not found in build directory" % target)
                continue
            lib_file = app + ".so"
            shutil.copy2(src_file, os.path.join(dir_path,lib_file))
            os.chdir(ROOT_PATH)
            os.system("python gen-data.py %s %d --apps %s" %(target, args.nruns, app))

    else:
        print("Performing stream tests for app: ", app)
        target = "polly-prop-parallel"
        for stream_size in stream_sizes:
            print("Processing stream_size: %d " % stream_size)
            src_file = os.path.join(build_path, "%s-%d.so" % (target, stream_size))
            if not os.path.isfile(src_file):
                print("Skipping stream size %d since file not found in build directory" % stream_size)
                continue
            lib_file = app + ".so"
            shutil.copy2(src_file, os.path.join(dir_path,lib_file))
            os.chdir(ROOT_PATH)
            os.system("python gen-data.py %s %d --apps %s --stream_size %d"
                    % (target, args.nruns, app, stream_size))

