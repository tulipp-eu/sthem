#!/usr/bin/python3

import sys
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy
import textwrap
import tabulate

import profileLib

_profileVersion = "0.1"

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-a", "--aggregate-keys", help="list of aggregation and display keys", default="1,3")
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

useCpus = list(set(profileLib.parseRange(args.cpus)))
aggregateKeys = profileLib.parseRange(args.aggregate_keys)


# [binary, function_mangled, function, file, line]

aggregatedProfile = {
    'version': _profileVersion,
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'profile': {},
    'volts': 0,
    'target': False,
    'mean': len(args.profiles)
}

useVolts = False
meanFac = 1 / aggregatedProfile['mean']
avgLatencyUs = 0
avgSampleTime = 0
# debugEnergy = 0

i = 1
for fileProfile in args.profiles:
    print(f"Aggreagte profile {i}/{len(args.profiles)}...\r", end="")
    i += 1

    profile = {}
    if fileProfile.endswith(".bz2"):
        profile = pickle.load(bz2.BZ2File(fileProfile, mode="rb"))
    else:
        profile = pickle.load(open(fileProfile, mode="rb"))

    if 'version' not in profile or profile['version'] != _profileVersion:
        raise Exception(f"Incompatible profile version (required: {_profileVersion})")

    if not aggregatedProfile['target']:
        aggregatedProfile['target'] = profile['target']
        aggregatedProfile['volts'] = profile['volts']
        useVolts = False if profile['volts'] == 0 else True

    if (profile['volts'] != aggregatedProfile['volts']):
        print("ERROR: profile voltages don't match!")

    sampleFormatter = profileLib.sampleFormatter(profile['binaries'], profile['functions'], profile['files'])

    aggregatedProfile['latencyTime'] += profile['latencyTime'] * meanFac
    aggregatedProfile['samplingTime'] += profile['samplingTime'] * meanFac
    aggregatedProfile['samples'] += profile['samples'] * meanFac
    avgSampleTime = profile['samplingTime'] / profile['samples']

    for sample in profile['profile']:
        # debugEnergy += sample[0] * (aggregatedProfile['volts'] if useVolts else 1) * avgSampleTime * meanFac

        metric = sample[0] * (aggregatedProfile['volts'] if useVolts else 1) * avgSampleTime

        activeCores = min(len(sample[2]), len(useCpus))

        for thread in sample[2]:
            threadId = thread[0]
            threadCpuTime = thread[1]
            sample = sampleFormatter.getSample(thread[2])

            # Needs improvement
            cpuShare = min(threadCpuTime, avgSampleTime) / (avgSampleTime * activeCores)

            aggregateIndex = sampleFormatter.formatSample(
                sample,
                displayKeys=aggregateKeys,
                delimiter=':',
                doubleSanitizer=False,
                lStringStrip=False,
                rStringStrip=False
            )

            aggregateData = [
                min(threadCpuTime, avgSampleTime) * meanFac,
                metric * cpuShare * meanFac
            ]

            if aggregateIndex in aggregatedProfile['profile']:
                aggregatedProfile['profile'][aggregateIndex][0] += aggregateData[0]
                aggregatedProfile['profile'][aggregateIndex][1] += aggregateData[1]
            else:
                aggregatedProfile['profile'][aggregateIndex] = [
                    aggregateData[0],
                    aggregateData[1],
                    sampleFormatter.formatSample(
                        sample,
                        displayKeys=aggregateKeys,
                        lStringStrip=aggregatedProfile['target']
                    )
                ]

    del sampleFormatter
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

totalEnergy = numpy.sum(metrics)

labelUnit = "J" if useVolts else "C"
labels = [f"{x:.4f} s, {y:.2f} {labelUnit}" if not useVolts else f"{x:.4f} s, {y/x if x > 0 else 0:.2f} W, {y*100/totalEnergy if totalEnergy > 0 else 0:.2f}%" for x, y in zip(times, metrics)]


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
                text=f"{aggregatedProfile['target']}, {frequency:.2f} Hz, {aggregatedProfile['samples']:.2f} samples, {(avgLatencyTime * 1000000):.2f} us latency, {totalEnergy:.2f} {labelUnit}" + (f", mean of {aggregatedProfile['mean']} runs" if aggregatedProfile['mean'] > 1 else ""),
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
    # print(f"DEBUG: {debugEnergy:.2f} J")
