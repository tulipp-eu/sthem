#!/usr/bin/python3

import sys
import os
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="postprocessed profile from intrvelf")
parser.add_argument("-o", "--output", help="html output file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open output file")


args = parser.parse_args()

if (not args.profile) or (not os.path.isfile(args.profile)):
    print("ERROR: profile not found")
    parser.print_help()
    sys.exit(1)

if args.profile.endswith(".bz2"):
    profile = pickle.load(bz2.BZ2File(args.profile, mode="rb"))
else:
    profile = pickle.load(open(args.profile, mode="rb"))

latencyUs = int(profile['latencyTimeUs'] / profile['samples'])
sampleTime = profile['samplingTimeUs'] / (profile['samples'] * 1000000)
freq = 1 / sampleTime

volts = 1 if (profile['volts'] == 0) else profile['volts']

# profile['aggregatedProfile'][addr] = [samples, current, function, file, line]

# aggmap[function] = [ current, time, function ]

# Fix for introduced mean
if 'mean' not in profile:
    profile['mean'] = 1

aggmap = {}
for sample in profile['aggregatedProfile']:
    sample = profile['aggregatedProfile'][sample]
    if (sample[0] > 0):
        if sample[3] in aggmap:
            aggmap[sample[3]][0] += sample[1] * sampleTime
            aggmap[sample[3]][1] += sample[0] * sampleTime
        else:
            aggmap[sample[3]] = [sample[1] * sampleTime, sample[0] * sampleTime, sample[3]]


values = numpy.array(sorted(aggmap.values()), dtype=object)
metrics = numpy.array(values[:, 0:1], dtype=float).flatten() * volts
times = numpy.array(values[:, 1:2], dtype=float).flatten()
functions = [profile['functions'][x] for x in numpy.array(values[:, 2:3].flatten(), dtype=int)]
labelUnit = "C" if profile['volts'] == 0 else "J"
labels = [f"{x:.4f} s, {y:.2f} {labelUnit}" if profile['volts'] == 0 else f"{x:.4f} s, {y/x:.2f} W, {y:.2f} {labelUnit}" for x, y in zip(times, metrics)]

functionLength = numpy.max([len(x) for x in functions])

fig = {
    "data": [go.Bar(
        x=metrics,
        y=functions,
        text=labels,
        textposition='auto',
        orientation='h',
        hoverinfo="x"
    )],
    "layout": go.Layout(
        title=go.layout.Title(
            text=f"{profile['target']}, {freq:.2f} Hz, {profile['samples']} samples, {latencyUs} us latency" + (f", mean of {profile['mean']} runs" if profile['mean'] > 1 else ""),
            xref='paper',
            x=0
        ),
        xaxis=go.layout.XAxis(
            title=go.layout.xaxis.Title(
                text="Charge in C" if profile['volts'] == 0 else "Energy in J",
                font=dict(
                    family='Courier New, monospace',
                    size=18,
                    color='#7f7f7f'
                )
            )
        ),
        yaxis=go.layout.YAxis(
            tickfont=dict(
                family='monospace',
                size=12,
                color='black'
            )
        ),
        margin=go.layout.Margin(l=7.5 * numpy.max([len(x) for x in functions]))
    )
}

file = "temp-plot.html" if not args.output else args.output

plotly.offline.plot(fig, filename=file, auto_open=not args.quiet)
print(f"Plot saved to {file}")
