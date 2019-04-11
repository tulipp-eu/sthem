#!/usr/bin/python3

import sys
import os
import argparse
import struct
import bz2
import pickle
import plotly
import plotly.graph_objs as go

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="postprocessed profile from intrvelf")

args = parser.parse_args();

if (not args.profile) or (not os.path.isfile(args.profile)):
    print("ERROR: profile not found")
    parser.print_help()
    sys.exit(1)

if args.profile.endswith(".bz2"):
    profile = pickle.load(bz2.BZ2File(args.profile, mode="rb"))
else:
    profile = pickle.load(open(args.profile, mode="rb"))

freq = (1000000 * profile['samples']) / profile['samplingTimeUs']
latency = int(profile['latencyTimeUs'] / profile['samples'])

#profile['aggregatedProfile'][addr] = [samples, current, function, file, line]

title = f"{profile['elf']}, {freq:.2f} Hz, {profile['samples']} samples, {latency} us latency"

functions = [ 'total' ]
currents = [ 0.0 ]

for sample in profile['aggregatedProfile']:
    sample = profile['aggregatedProfile'][sample];
    if (sample[0] > 0):
        avgCurrent = sample[1] / sample[0]
        function = profile['functions'][sample[2]]
        currents[0] += avgCurrent
        if function in functions:
            currents[functions.index(function)] += avgCurrent
        else:
            functions.append(profile['functions'][sample[2]])
            currents.append(avgCurrent)

sorty = [x for _,x in sorted(zip(currents,functions))]
sortx = sorted(currents)

aggregated = go.Bar(
    y=sorty,
    x=sortx,
    orientation='h'
)

plotly.offline.plot({
    "data": [aggregated]
    #"layout": go.Layout(barmode='basic-bar', title=title)
}, auto_open=True)
