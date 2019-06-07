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

_profileVersion = "0.3"
_aggregatedProfileVersion = "a0.4"

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-a", "--aggregate-keys", help="list of aggregation and display keys", default="1,3")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-l", "--limit", help="limit output to % of energy/current", type=float, default=0)
parser.add_argument("-t", "--table", help="output csv table")
parser.add_argument("-p", "--plot", help="plotly html file")
parser.add_argument("-o", "--output", help="output aggregated profile")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open output file", default=False)


args = parser.parse_args()

if (args.limit is not 0 and (args.limit < 0 or args.limit > 1)):
    print("ERROR: limit is out of range")
    parser.print_help()
    sys.exit(0)

if (args.quiet and not args.plot and not args.table and not args.output):
    parser.print_help()
    sys.exit(0)

if (not args.profiles) or (len(args.profiles) <= 0):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)

useCpus = list(set(profileLib.parseRange(args.cpus)))
aggregateKeys = profileLib.parseRange(args.aggregate_keys)

if (max(aggregateKeys) > 5 or min(aggregateKeys) < 0):
    print("ERROR: aggregate keys are out of bounds (0-5)")
    sys.exit(1)

aggregatedProfile = {
    'version': _aggregatedProfileVersion,
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'profile': {},
    'volts': 0,
    'target': False,
    'mean': len(args.profiles),
    'aggreagted': True
}

meanFac = 1 / aggregatedProfile['mean']
avgLatencyUs = 0
avgSampleTime = 0

i = 1
for fileProfile in args.profiles:
    print(f"Aggregate profile {i}/{len(args.profiles)}...\r", end="")
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

    if (profile['volts'] != aggregatedProfile['volts']):
        print("ERROR: profile voltages don't match!")

    sampleFormatter = profileLib.sampleFormatter(profile['binaries'], profile['functions'], profile['files'])

    aggregatedProfile['latencyTime'] += profile['latencyTime'] * meanFac
    aggregatedProfile['samplingTime'] += profile['samplingTime'] * meanFac
    aggregatedProfile['samples'] += profile['samples'] * meanFac
    avgSampleTime = profile['samplingTime'] / profile['samples']

    subAggregate = {}
    threadLocations = {}
    for sample in profile['profile']:
        # activeCores = min(len(sample[2]), len(useCpus))

        for thread in sample[2]:
            threadId = thread[0]
            threadCpuTime = thread[1]
            sampleData = sampleFormatter.getSample(thread[2])

            aggregateIndex = sampleFormatter.formatSample(sampleData, displayKeys=aggregateKeys)
            if threadId not in threadLocations:
                threadLocations[threadId] = None

            if aggregateIndex not in subAggregate:
                subAggregate[aggregateIndex] = [
                    threadCpuTime,  # total execution time
                    1,
                    sample[0] * threadCpuTime,  # energy (later power)
                    sampleFormatter.sanitizeOutput(aggregateIndex, lStringStrip=aggregatedProfile['target'])  # label
                ]
            else:
                subAggregate[aggregateIndex][0] += threadCpuTime
                if threadLocations[threadId] != aggregateIndex:
                    subAggregate[aggregateIndex][1] += 1
                subAggregate[aggregateIndex][2] += sample[0] * threadCpuTime

            threadLocations[threadId] = aggregateIndex

    del sampleFormatter
    del profile

    for key in subAggregate:
        if key in aggregatedProfile['profile']:
            aggregatedProfile['profile'][key][0] += subAggregate[key][0] * meanFac
            aggregatedProfile['profile'][key][1] += subAggregate[key][1] * meanFac
            aggregatedProfile['profile'][key][3] += subAggregate[key][2] * meanFac
        else:
            aggregatedProfile['profile'][key] = [
                subAggregate[key][0] * meanFac,
                subAggregate[key][1] * meanFac,
                0,
                subAggregate[key][2] * meanFac,
                subAggregate[key][3]
            ]

    del subAggregate

for key in aggregatedProfile['profile']:
    time = aggregatedProfile['profile'][key][0]
    energy = aggregatedProfile['profile'][key][3]
    aggregatedProfile['profile'][key][2] = energy / time if time != 0 else 0


avgLatencyTime = aggregatedProfile['latencyTime'] / aggregatedProfile['samples']
avgSampleTime = aggregatedProfile['samplingTime'] / aggregatedProfile['samples']
frequency = 1 / avgSampleTime

values = numpy.array(list(aggregatedProfile['profile'].values()), dtype=object)
values = values[values[:, 3].argsort()]

times = numpy.array(values[:, 0], dtype=float)
execs = numpy.array(values[:, 1], dtype=float)
powers = numpy.array(values[:, 2], dtype=float)
energies = numpy.array(values[:, 3], dtype=float)
aggregationLabel = values[:, 4]

totalTime = numpy.sum(times)
totalEnergy = numpy.sum(energies)
totalPower = totalEnergy / totalTime if totalTime > 0 else 0
totalExec = numpy.sum(execs)

if args.limit is not 0:
    accumulate = 0.0
    accumulateLimit = totalEnergy * args.limit
    for index, value in enumerate(energies[::-1]):
        accumulate += value
        if (accumulate >= accumulateLimit):
            cutOff = len(energies) - (index + 1)
            print(f"Limit output to {index+1}/{len(energies)} values...")
            times = times[cutOff:]
            execs = execs[cutOff:]
            energies = energies[cutOff:]
            powers = powers[cutOff:]
            aggregationLabel = aggregationLabel[cutOff:]
            break

labels = [f"{x:.4f} s, {x * 1000/a:.3f} ms, {s:.2f} W" + f", {y * 100 / totalEnergy if totalEnergy > 0 else 0:.2f}%" for x, a, s, y in zip(times, execs, powers, energies)]


if (args.plot):
    pAggregationLabel = [textwrap.fill(x, 64).replace('\n', '<br />') for x in aggregationLabel]
    fig = {
        "data": [go.Bar(
            x=energies,
            y=pAggregationLabel,
            text=labels,
            textposition='auto',
            orientation='h',
            hoverinfo="x",
        )],
        "layout": go.Layout(
            title=go.layout.Title(
                text=f"{aggregatedProfile['target']}, {frequency:.2f} Hz, {aggregatedProfile['samples']:.2f} samples, {(avgLatencyTime * 1000000):.2f} us latency, {totalEnergy:.2f} J" + (f", mean of {aggregatedProfile['mean']} runs" if aggregatedProfile['mean'] > 1 else ""),
                xref='paper',
                x=0
            ),
            xaxis=go.layout.XAxis(
                title=go.layout.xaxis.Title(
                    text="Energy in J",
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
            margin=go.layout.Margin(l=6.2 * min(64, numpy.max([len(x) for x in aggregationLabel])))
        )
    }

    plotly.offline.plot(fig, filename=args.plot, auto_open=not args.quiet)
    print(f"Plot saved to {args.plot}")
    del pAggregationLabel
    del fig

if (args.table or not args.quiet):
    aggregationLabel = numpy.insert(aggregationLabel[::-1], 0, "_total")
    times = numpy.insert(times[::-1], 0, totalTime)
    execs = numpy.insert(execs[::-1], 0, totalExec)
    energies = numpy.insert(energies[::-1], 0, totalEnergy)
    powers = numpy.insert(powers[::-1], 0, totalPower)


if (args.table):
    if args.table.endswith("bz2"):
        table = bz2.BZ2File.open(args.table, "w")
    else:
        table = open(args.table, "w")
    table.write("function;time;power;energy\n")
    for f, t, e, s, m in zip(aggregationLabel, times, execs, powers, energies):
        table.write(f"{f};{t};{e},{s};{m}\n")
    table.close()
    print(f"CSV saved to {args.table}")

if (args.output):
    if args.output.endswith("bz2"):
        output = bz2.BZ2File(args.output, "wb")
    else:
        output = open(args.output, "wb")
    pickle.dump(aggregatedProfile, output, pickle.HIGHEST_PROTOCOL)
    print(f"Aggregated profile saved to {args.output}")


if (not args.quiet):
    print(tabulate.tabulate(zip(aggregationLabel, times, execs, powers, energies), headers=['Function', 'Time [s]', 'Executions', 'Power [W]', 'Energy [J]']))
