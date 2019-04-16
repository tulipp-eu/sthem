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
parser.add_argument("-v", "--volts", help="set pmu voltage")


args = parser.parse_args()

if (not args.profile) or (not os.path.isfile(args.profile)):
    print("ERROR: profile not found")
    parser.print_help()
    sys.exit(1)

volts = 1 if not args.volts else float(args.volts)


if args.profile.endswith(".bz2"):
    profile = pickle.load(bz2.BZ2File(args.profile, mode="rb"))
else:
    profile = pickle.load(open(args.profile, mode="rb"))

freq = (1000000 * profile['samples']) / profile['samplingTimeUs']
latency = int(profile['latencyTimeUs'] / profile['samples'])
sampleTime = (profile['samplingTimeUs'] / profile['samples'])

#profile['aggregatedProfile'][addr] = [samples, current, function, file, line]

title = f"{profile['elf']}, {freq:.2f} Hz, {profile['samples']} samples, {latency} us latency"

title_xaxis = "Charge in C" if not args.volts else "Energy in J"

functions = [] #[ 'total' ]
currents = [] #[ 0.0 ]

for sample in profile['aggregatedProfile']:
    sample = profile['aggregatedProfile'][sample];
    if (sample[0] > 0):
        avgVal = (sample[1] / sample[0]) * volts
        function = profile['functions'][sample[2]]
        #currents[0] += avgVal
        if function in functions:
            currents[functions.index(function)] += avgVal
        else:
            functions.append(profile['functions'][sample[2]])
            currents.append(avgVal)

sorty = [x for _,x in sorted(zip(currents,functions))]
sortx = sorted(currents)

aggregated = go.Bar(
    y=sorty,
    x=sortx,
    orientation='h'
)

layout = go.Layout(
    title=go.layout.Title(
        text=title,
        xref='paper',
        x=0
    ),
    xaxis=go.layout.XAxis(
        title=go.layout.xaxis.Title(
            text=title_xaxis,
            font=dict(
                family='Courier New, monospace',
                size=18,
                color='#7f7f7f'
            )
        )
    ),
    margin=dict(
        l=200
    )
)

plotly.offline.plot({
    "data": [aggregated],
    "layout" : layout
    #"layout": go.Layout(barmode='basic-bar', title=title)
}, auto_open=True)
