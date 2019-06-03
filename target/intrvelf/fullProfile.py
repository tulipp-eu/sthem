#!/usr/bin/python3

import sys
import os
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy
import profileLib

_profileVersion = "0.3"

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="postprocessed profile from intrvelf")
parser.add_argument("-s", "--start", type=float, help="plot start time (seconds)")
parser.add_argument("-e", "--end", type=float, help="plot end time (seconds)")
parser.add_argument("-n", "--no-threads", action="store_true", help="interpolate samples")
parser.add_argument("-i", "--interpolate", type=int, help="interpolate samples")
parser.add_argument("-p", "--plot", help="plot output html file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open plot")

args = parser.parse_args()

if (not args.plot):
    parser.print_help()
    sys.exit(0)

if (not args.profile) or (not os.path.isfile(args.profile)):
    print("ERROR: profile not found")
    parser.print_help()
    sys.exit(1)

print("Reading profile... ", end="")
sys.stdout.flush()

if args.profile.endswith(".bz2"):
    profile = pickle.load(bz2.BZ2File(args.profile, mode="rb"))
else:
    profile = pickle.load(open(args.profile, mode="rb"))

print("finished")


if 'version' not in profile or profile['version'] != _profileVersion:
    raise Exception(f"Incompatible profile version (required: {_profileVersion})")


avgSampleLatency = profile['latencyTime'] / profile['samples']
avgSampleTime = profile['samplingTime'] / profile['samples']
freq = 1 / avgSampleTime
volts = profile['volts']
cpus = profile['cpus']

samples = numpy.array(profile['profile'], dtype=object)


if (args.start):
    samples = samples[int(args.start / avgSampleTime) - 1:]
    if (args.end):
        args.end -= args.start
else:
    args.start = 0.0

if (args.end):
    samples = samples[:int(args.end / avgSampleTime)]

if (args.interpolate):
    print("Interpolating... ", end="")
    sys.stdout.flush()
    if (len(samples) % args.interpolate != 0):
        samples = numpy.delete(samples, numpy.s_[-(len(samples) % args.interpolate):], axis=0)
    samples = samples.reshape(-1, args.interpolate, 3)
    samples = numpy.array([[x[:, :1].mean(), 0, x[0][2]] for x in samples], dtype=object)
    samples = samples.reshape(-1, 3)
    print("finished")
else:
    args.interpolate = 1

times = numpy.arange(len(samples)) * avgSampleTime * args.interpolate + args.start
powers = samples[:, :1].flatten()

threads = []
threadDisplay = []

sampleFormatter = profileLib.sampleFormatter(profile['binaries'], profile['functions'], profile['files'])

if not args.no_threads:
    print("Parsing threads... ", end="")
    sys.stdout.flush()
    threadNone = [None] * len(samples)
    threadMap = {}
    for i in range(0, len(samples)):
        # Determine possible active cores
        activeCores = min(len(samples[i][2]), cpus)

        for threadSample in samples[i][2]:
            threadSampleCpuTime = threadSample[1]

            if threadSample[0] in threadMap:
                threadIndex = threadMap[threadSample[0]]
            else:
                threadIndex = len(threads)
                threadMap[threadSample[0]] = threadIndex
                threads.append(list.copy(threadNone))
                threadDisplay.append(list.copy(threadNone))

            cpuShare = min(threadSampleCpuTime, avgSampleTime) / (avgSampleTime * activeCores)

            threads[threadIndex][i] = threadIndex + 1
            threadDisplay[threadIndex][i] = sampleFormatter.sanitizeOutput(sampleFormatter.formatData(threadSample[2]), lStringStrip=profile['target']) + f", {cpuShare:.2f}"
    print("finished")


title = f"{profile['target']}, {freq:.2f} Hz, {profile['samples']} samples, {int(avgSampleLatency * 1000000)} us latency"
if args.interpolate > 1:
    title += f", {args.interpolate} samples interpolated"
threadAxisHeight = 0 if args.no_threads else 0.1 + (0.233 * min(1, len(threads) / 32))

del profile

fig = plotly.tools.make_subplots(
    rows=1 if args.no_threads else 2,
    cols=1,
    specs=[[{}]] if args.no_threads else [[{}], [{}]],
    shared_xaxes=True,
    shared_yaxes=False,
    vertical_spacing=0.001,
    print_grid=False
)

fig['layout'].update(
    title=go.layout.Title(
        text=title,
        xref='paper',
        x=0
    ),
    showlegend=False,
)

fig['layout']['xaxis'].update(
    title=go.layout.xaxis.Title(
        text='Time in Seconds',
        font=dict(
            family='Courier New, monospace',
            size=18,
            color='#7f7f7f'
        )
    ),
)

fig['layout']['yaxis1'].update(
    title=go.layout.yaxis.Title(
        text="Power in W",
        font=dict(
            family='Courier New, monospace',
            size=18,
            color='#7f7f7f'
        )
    ),
    domain=[threadAxisHeight, 1]
)

print(f"Going to plot {len(samples)} samples from {times[0]}s to {times[-1]}s")

fig.append_trace(
    go.Scatter(
        name="W",
        x=times,
        y=powers,
        line={'width': 1},
    ), 1, 1
)

del powers

if not args.no_threads:
    ticknumbers = numpy.arange(len(threads) + 2)
    ticklabels = numpy.array(list(map(str, ticknumbers)))
    ticklabels[0] = ''
    ticklabels[-1] = ''

    fig['layout']['yaxis2'].update(
        title=go.layout.yaxis.Title(
            text="Threads",
            font=dict(
                family='Courier New, monospace',
                size=18,
                color='#7f7f7f'
            )
        ),
        tickvals=ticknumbers,
        ticktext=ticklabels,
        tick0=0,
        dtick=1,
        range=[0, len(threads) + 1],
        domain=[0, threadAxisHeight]
    )

    for i in range(0, len(threads)):
        print(f"Including thread {i}... ", end="")
        sys.stdout.flush()
        fig.append_trace(
            go.Scatter(
                name=f"Thread {i+1}",
                x=times,
                y=threads[i],
                text=threadDisplay[i],
                hoverinfo='text+x'
            ), 2, 1
        )
        print("finished")


del threads
del threadDisplay

sys.stdout.flush()
plotly.offline.plot(fig, filename=args.plot, auto_open=not args.quiet)
print(f"Plot saved to {args.plot}")
