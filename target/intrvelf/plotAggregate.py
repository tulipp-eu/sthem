#!/usr/bin/python3

import sys
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy


def convertStringRange(x):
    result = []
    for part in x.split(','):
        if '-' in part:
            a, b = part.split('-')
            a, b = int(a), int(b)
            result.extend(range(a, b + 1))
        else:
            a = int(part)
            result.append(a)
    return result


parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-o", "--output", help="html output file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open output file")


args = parser.parse_args()

if (not args.profiles) or (len(args.profiles) <= 0):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)

useCpus = list(set(convertStringRange(args.cpus)))

# [binary, function_mangled, function, file, line]
aggregateKeys = [2]

samples = 0  # profiles[0]['samples']
samplingTime = 0  # profiles[0]['samplingTimeUs']
latencyTime = 0  # profiles[0]['latencyTimeUs']
binaries = []
functions = []
functions_mangled = []
files = []
volts = 0
target = False
mean = len(args.profiles)

meanFac = 1 / mean


avgLatencyUs = 0  # int(profiles['latencyTimeUs'] / profile['samples'])
avgSampleTime = 0  # profile['samplingTimeUs'] / (profile['samples'] * 1000000)

aggregatedProfile = {}

i = 1
for fileProfile in args.profiles:
    print(f"Aggreagte profile {i}/{len(args.profiles)}...\r", end="")
    i += 1

    profile = {}
    if fileProfile.endswith(".bz2"):
        profile = pickle.load(bz2.BZ2File(fileProfile, mode="rb"))
    else:
        profile = pickle.load(open(fileProfile, mode="rb"))

    if not target:
        target = profile['target']
        volts = profile['volts']
        useVolts = False if profile['volts'] == 0 else True

    if (profile['volts'] != volts):
        print("ERROR: profile voltages don't match!")

    latencyTime += profile['latencyTime'] * meanFac
    samplingTime += profile['samplingTime'] * meanFac
    samples += profile['samples'] * meanFac
    avgSampleTime = profile['samplingTime'] / profile['samples']

    for sample in profile['profile']:
        metric = sample[0] * (volts if useVolts else 1) * avgSampleTime
        sampleCpuTime = sample[1]
        activeCores = min(len(sample[2]), len(useCpus))

        for thread in sample[2]:
            threadId = thread[0]
            threadSampleCpuTime = thread[1]
            srcbinary = profile['binaries'][thread[2]]
            srcfunction = profile['functions_mangled'][thread[3]]
            srcdemangled = profile['functions'][thread[3]]
            srcfile = profile['files'][thread[4]]
            srcline = thread[5]

            # Needs improvement
            cpuShare = min(threadSampleCpuTime, avgSampleTime) / (avgSampleTime * activeCores)

            if srcbinary not in binaries:
                binaries.append(srcbinary)
            if srcfunction not in functions_mangled:
                functions_mangled.append(srcfunction)
                functions.append(srcdemangled)
            if srcfile not in files:
                files.append(srcfile)

            sampleInfo = [
                binaries.index(srcbinary),
                functions_mangled.index(srcfunction),
                functions.index(srcdemangled),
                files.index(srcfile),
                srcline
            ]
            sampleInfoText = [
                srcbinary,
                srcfunction,
                srcdemangled,
                srcfile,
                srcline
            ]

            aggregateIndex = ':'.join([str(sampleInfo[i]) for i in aggregateKeys])
            aggregateString = ':'.join([str(sampleInfoText[i]) for i in aggregateKeys])

            sampleData = [
                min(threadSampleCpuTime, avgSampleTime) * meanFac,
                metric * cpuShare * meanFac,
                aggregateString
            ]

            if aggregateIndex in aggregatedProfile:
                aggregatedProfile[aggregateIndex][0] += sampleData[0]
                aggregatedProfile[aggregateIndex][1] += sampleData[1]
            else:
                aggregatedProfile[aggregateIndex] = sampleData

    del profile

print(f"Successfully aggregated {len(args.profiles)} profiles!")

avgLatencyTime = latencyTime / samples
avgSampleTime = samplingTime / samples
frequency = 1 / avgSampleTime

# profile['aggregatedProfile'][addr] = [samples, current, binary, function, file, line]

# aggmap[function] = [ current, time, function ]

values = numpy.array(list(aggregatedProfile.values()), dtype=object)
values = values[values[:, 1].argsort()]

times = numpy.array(values[:, 0], dtype=float)
metrics = numpy.array(values[:, 1], dtype=float)
aggregation = values[:, 2]

labelUnit = "J" if useVolts else "C"
labels = [f"{x:.4f} s, {y:.2f} {labelUnit}" if not useVolts else f"{x:.4f} s, {y/x if x > 0 else 0:.2f} W, {y:.2f} {labelUnit}" for x, y in zip(times, metrics)]

aggregationLength = numpy.max([len(x) for x in aggregation])

fig = {
    "data": [go.Bar(
        x=metrics,
        y=aggregation,
        text=labels,
        textposition='auto',
        orientation='h',
        hoverinfo="x",
    )],
    "layout": go.Layout(
        title=go.layout.Title(
            text=f"{target}, {frequency:.2f} Hz, {samples:.2f} samples, {(avgLatencyTime * 1000000):.2f} us latency" + (f", mean of {mean} runs" if mean > 1 else ""),
            xref='paper',
            x=0
        ),
        xaxis=go.layout.XAxis(
            title=go.layout.xaxis.Title(
                text="Energy in J" if useVolts else "Charge in C",
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
                size=11,
                color='black'
            )
        ),
        margin=go.layout.Margin(l=min(500, 6.2 * numpy.max([len(x) for x in functions])))
    )
}

file = "temp-plot.html" if not args.output else args.output

plotly.offline.plot(fig, filename=file, auto_open=not args.quiet)
print(f"Plot saved to {file}")
