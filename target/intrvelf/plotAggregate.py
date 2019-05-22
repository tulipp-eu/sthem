#!/usr/bin/python3

import sys
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy
import textwrap
import re
import tabulate

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


# [srcbinary, srcfunction, srcdemangled, srcfile, srcline]
def formatOutput(useProfile, indexes, displayKeys=[0, 2], delimiter=':', sanitizer=['_unknown', '_foreign', '_kernel'], includeTarget=False):
    indexMap = ['binaries', 'functions_mangled', 'functions', 'srcfile']
    outputMap = [(useProfile[indexMap[x]][indexes[x]]) if not x == 5 else str(indexes[x]) for x in displayKeys]
    display = delimiter.join(outputMap)
    for san in sanitizer:
        display = display.replace(f"{san}{delimiter}{san}", f"{san}").strip(delimiter)
    if not includeTarget:
        display = re.sub(r"^" + re.escape(useProfile['target']), "", display).strip(delimiter)
    return display


parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-t", "--table", help="output csv table")
parser.add_argument("-o", "--output", help="html output file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open output file", default=False)


args = parser.parse_args()

if (args.quiet and not args.output and not args.table):
    parser.print_help()
    sys.exit(1)

if (not args.profiles) or (len(args.profiles) <= 0):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)

useCpus = list(set(convertStringRange(args.cpus)))

# [binary, function_mangled, function, file, line]
aggregateKeys = [2]

aggregatedProfile = {
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'binaries': [],
    'functions': [],
    'functions_mangled': [],
    'files': [],
    'profile': {},
    'volts': 0,
    'target': False,
    'mean': len(args.profiles)
}

useVolts = False

meanFac = 1 / aggregatedProfile['mean']


avgLatencyUs = 0
avgSampleTime = 0

i = 1
for fileProfile in args.profiles:
    print(f"Aggreagte profile {i}/{len(args.profiles)}...\r", end="")
    i += 1

    profile = {}
    if fileProfile.endswith(".bz2"):
        profile = pickle.load(bz2.BZ2File(fileProfile, mode="rb"))
    else:
        profile = pickle.load(open(fileProfile, mode="rb"))

    if not aggregatedProfile['target']:
        aggregatedProfile['target'] = profile['target']
        aggregatedProfile['volts'] = profile['volts']
        useVolts = False if profile['volts'] == 0 else True

    if (profile['volts'] != aggregatedProfile['volts']):
        print("ERROR: profile voltages don't match!")

    aggregatedProfile['latencyTime'] += profile['latencyTime'] * meanFac
    aggregatedProfile['samplingTime'] += profile['samplingTime'] * meanFac
    aggregatedProfile['samples'] += profile['samples'] * meanFac
    avgSampleTime = profile['samplingTime'] / profile['samples']

    for sample in profile['profile']:
        metric = sample[0] * (aggregatedProfile['volts'] if useVolts else 1) * avgSampleTime
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

            if srcbinary not in aggregatedProfile['binaries']:
                aggregatedProfile['binaries'].append(srcbinary)
            if srcfunction not in aggregatedProfile['functions_mangled']:
                aggregatedProfile['functions_mangled'].append(srcfunction)
                aggregatedProfile['functions'].append(srcdemangled)
            if srcfile not in aggregatedProfile['files']:
                aggregatedProfile['files'].append(srcfile)

            sampleInfo = [
                aggregatedProfile['binaries'].index(srcbinary),
                aggregatedProfile['functions_mangled'].index(srcfunction),
                aggregatedProfile['functions'].index(srcdemangled),
                aggregatedProfile['files'].index(srcfile),
                srcline
            ]

            aggregateIndex = ':'.join([str(sampleInfo[i]) for i in aggregateKeys])

            sampleData = [
                min(threadSampleCpuTime, avgSampleTime) * meanFac,
                metric * cpuShare * meanFac
            ]

            if aggregateIndex in aggregatedProfile['profile']:
                aggregatedProfile['profile'][aggregateIndex][0] += sampleData[0]
                aggregatedProfile['profile'][aggregateIndex][1] += sampleData[1]
            else:
                aggregatedProfile['profile'][aggregateIndex] = [sampleData[0], sampleData[1], formatOutput(aggregatedProfile, sampleInfo)]

    del profile


avgLatencyTime = aggregatedProfile['latencyTime'] / aggregatedProfile['samples']
avgSampleTime = aggregatedProfile['samplingTime'] / aggregatedProfile['samples']
frequency = 1 / avgSampleTime

# profile['aggregatedProfile'][addr] = [samples, current, binary, function, file, line]

# aggmap[function] = [ current, time, function ]

values = numpy.array(list(aggregatedProfile['profile'].values()), dtype=object)
values = values[values[:, 1].argsort()]

times = numpy.array(values[:, 0], dtype=float)
metrics = numpy.array(values[:, 1], dtype=float)
aggregation = values[:, 2]

labelUnit = "J" if useVolts else "C"
labels = [f"{x:.4f} s, {y:.2f} {labelUnit}" if not useVolts else f"{x:.4f} s, {y/x if x > 0 else 0:.2f} W, {y:.2f} {labelUnit}" for x, y in zip(times, metrics)]

if (args.output):
    paggregation = [textwrap.fill(x, 64).replace('\n', '<br />') for x in aggregation]
    fig = {
        "data": [go.Bar(
            x=metrics,
            y=paggregation,
            text=labels,
            textposition='auto',
            orientation='h',
            hoverinfo="x",
        )],
        "layout": go.Layout(
            title=go.layout.Title(
                text=f"{aggregatedProfile['target']}, {frequency:.2f} Hz, {aggregatedProfile['samples']:.2f} samples, {(avgLatencyTime * 1000000):.2f} us latency, {numpy.sum(metrics):.2f} {labelUnit}" + (f", mean of {aggregatedProfile['mean']} runs" if aggregatedProfile['mean'] > 1 else ""),
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
            margin=go.layout.Margin(l=6.2 * min(64, numpy.max([len(x) for x in aggregation])))
        )
    }

    plotly.offline.plot(fig, filename=args.output, auto_open=not args.quiet)
    print(f"Plotly saved to {args.output}")
    del paggregation

if (args.table or not args.quiet):
    something = numpy.array([(x / y) if y > 0 else 0 for x, y in zip(metrics, times)])
    aggregation = numpy.insert(aggregation[::-1], 0, "_total")
    times = numpy.insert(times[::-1], 0, numpy.sum(times))
    metrics = numpy.insert(metrics[::-1], 0, numpy.sum(metrics))
    something = numpy.insert(something[::-1], 0, metrics[0] / times[0] if times[0] > 0 else 0)


if (args.table):
    table = open(args.table, "w")
    table.write(f"function;time;{'watt' if useVolts else 'current'};metric\n")
    for f, t, s, m in zip(aggregation, times, something, metrics):
        table.write(f"{f};{t};{s};{m}\n")
    table.close()
    print(f"CSV save to {args.table}")

if (not args.quiet):
    print(tabulate.tabulate(zip(aggregation, times, something, metrics), headers=['Function', 'Time [s]', 'Watt [W]' if useVolts else 'Current [A]', 'Energy [J]' if useVolts else 'Charge [C]']))
