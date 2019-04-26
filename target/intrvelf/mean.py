#!/usr/bin/python3

import sys
import os
import argparse
import struct
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy

parser = argparse.ArgumentParser(description="Average multiple profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-o", "--output", help="output mean aggregate only profile")
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile");


args = parser.parse_args()

if (not args.profiles) or (len(args.profiles) <=1):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)

if (not args.output):
    print("ERROR: not output file defined!")
    parser.print_help()
    sys.exit(1)

if (args.bzip2):
    args.output += ".bz2"
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")


profiles = []

for profile in args.profiles:
    if profile.endswith(".bz2"):
        new = pickle.load(bz2.BZ2File(profile, mode="rb"))
    else:
        new = pickle.load(open(profile, mode="rb"))
    del new['fullProfile']
    profiles.append(new)

label_unknown = profiles[0]['binaries'][0]

meanprofile={
    'samples' : 0,
    'samplingTimeUs' : 0,
    'latencyTimeUs' : 0,
    'volts' : profiles[0]['volts'],
    'target' : profiles[0]['target'],
    'binaries' : [ label_unknown ],
    'functions' : [ label_unknown ],
    'files' : [ label_unknown ],
    'fullProfile' : [],
    'aggregatedProfile' : { label_unknown : [0,0,0,0,0,0] }
}

meanfac = 1/len(profiles)

print(f"Calculating mean of {len(profiles)} profiles...")

i=0
for profile in profiles:
    progress = int((i+1) * 100 / len(profiles))
    print(f"Processing profiles... {progress}%\r", end="")
    sys.stdout.flush()
    i += 1
        
    if (profile['volts'] != meanprofile['volts']):
        print("ERROR: profile voltages do not match!")
        sys.exit(1)
        
    meanprofile['samples'] += meanfac * profile['samples']
    meanprofile['samplingTimeUs'] += meanfac * profile['samplingTimeUs']
    meanprofile['latencyTimeUs'] += meanfac * profile['latencyTimeUs']
    for pc in profile['aggregatedProfile']:
        sample = profile['aggregatedProfile'][pc]
        samples = sample[0] * meanfac
        current = sample[1] * meanfac
        srcbinary = profile['binaries'][sample[2]]
        srcfunction = profile['functions'][sample[3]]
        srcfile = profile['files'][sample[4]]
        srcline = sample[5]
       
        if not pc in meanprofile['aggregatedProfile']:
            if not srcbinary in meanprofile['binaries']:
                meanprofile['binaries'].append(srcbinary)
            if not srcfunction in meanprofile['functions']:
                meanprofile['functions'].append(srcfunction)
            if not srcfile in meanprofile['files']:
                meanprofile['files'].append(srcfile)

            meanprofile['aggregatedProfile'][pc] = [
                samples,
                current,
                meanprofile['binaries'].index(srcbinary),
                meanprofile['functions'].index(srcfunction),
                meanprofile['files'].index(srcfile),
                srcline
            ]
        else:
            meanprofile['aggregatedProfile'][pc][0] += samples
            meanprofile['aggregatedProfile'][pc][1] += current
    
print(f"Processing profiles... finished")
print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(meanprofile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")

sys.exit(0)
