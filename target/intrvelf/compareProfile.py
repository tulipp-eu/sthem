#!/usr/bin/python3

import sys
import argparse
import bz2
import pickle
import plotly
import plotly.graph_objs as go
import numpy
import textwrap
# import tabulate


def difference(x, y, z):
    return x - y


def absoluteDifference(x, y, z):
    return abs(difference(x, y, z))


def error(x, y, z):
    return difference(x, y, z) / x


def absoluteError(x, y, z):
    return abs(error(x, y, z))


def weightedError(x, y, z):
    return error(x, y, z) * (x / z)


def weightedAbsoluteError(x, y, z):
    return abs(weightedError(x, y, z))


_aggregatedProfileVersion = "a0.1"

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="baseline aggregated profile")
parser.add_argument("profiles", help="aggregated profiles to compare", nargs="+")
parser.add_argument("-l", "--limit", help="error threshold to include", type=float, default=0)
parser.add_argument("-t", "--table", help="output csv table")
parser.add_argument("-p", "--plot", help="plotly html file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open plot file", default=False)


args = parser.parse_args()

if (args.limit is not 0 and (args.limit < 0)):
    print("ERROR: limit is out of range")
    parser.print_help()
    sys.exit(0)

if (args.quiet and not args.plot and not args.table):
    parser.print_help()
    sys.exit(0)

if (not args.profiles) or (len(args.profiles) <= 0):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)


if args.profile.endswith(".bz2"):
    baselineProfile = pickle.load(bz2.BZ2File(args.profile, mode="rb"))
else:
    baselineProfile = pickle.load(open(args.profile, mode="rb"))

if 'version' not in baselineProfile or baselineProfile['version'] != _aggregatedProfileVersion:
    raise Exception(f"Incompatible profile version (required: {_aggregatedProfileVersion})")

totalTime = 0
totalMetric = 0
for key in baselineProfile['profile']:
    totalTime += baselineProfile['profile'][key][0]
    totalMetric += baselineProfile['profile'][key][1]

errorCharts = {}
errorFunction = weightedAbsoluteError

i = 1
for fileProfile in args.profiles:
    print(f"Aggreagte profile {i}/{len(args.profiles)}...\r", end="")
    i += 1

    profile = {}
    if fileProfile.endswith(".bz2"):
        profile = pickle.load(bz2.BZ2File(fileProfile, mode="rb"))
    else:
        profile = pickle.load(open(fileProfile, mode="rb"))

    if 'version' not in profile or profile['version'] != _aggregatedProfileVersion:
        raise Exception(f"Incompatible profile version (required: {_aggregatedProfileVersion})")

    errorCharts[fileProfile] = {}

    # profile['profile] = [[time, metric, label]]
    for key in baselineProfile['profile']:
        if key in profile['profile']:
            errorVal = errorFunction(baselineProfile['profile'][key][1], profile['profile'][key][1], totalMetric)
            if (args.limit == 0 or abs(errorVal) >= args.limit):
                errorCharts[fileProfile][key] = [
                    errorVal,
                    baselineProfile['profile'][key][2]
                ]

    del profile

if (args.plot):
    fig = {'data': []}
    maxLen = 0
    for chart in errorCharts:
        values = numpy.array(list(errorCharts[chart].values()), dtype=object)
        if (len(values) == 0):
            continue
        values = values[values[:, 0].argsort()]
        metrics = numpy.array(values[:, 0], dtype=float)
        aggregationLabel = values[:, 1]
        pAggregationLabel = [textwrap.fill(x, 64).replace('\n', '<br />') for x in aggregationLabel]
        fig["data"].append(
            go.Bar(
                y=pAggregationLabel,
                x=metrics,
                name=chart,
                textposition='auto',
                orientation='h',
                hoverinfo='x+y'
            )
        )
        maxLen = max(maxLen, numpy.max([len(x) for x in aggregationLabel]))

    fig['layout'] = go.Layout(
        title=go.layout.Title(
            text=f"compare plot",
            xref='paper',
            x=0
        ),
        xaxis=go.layout.XAxis(
            title=go.layout.xaxis.Title(
                text="Functions",
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
        margin=go.layout.Margin(l=6.8 * min(64, maxLen))
    )
    print(maxLen)

    plotly.offline.plot(fig, filename=args.plot, auto_open=not args.quiet)
    print(f"Plot saved to {args.plot}")
    del pAggregationLabel
    del fig
'''
if (args.table or not args.quiet):
    aggregationLabel = numpy.insert(aggregationLabel[::-1], 0, "_total")
    times = numpy.insert(times[::-1], 0, totalTime)
    metrics = numpy.insert(metrics[::-1], 0, totalMetric)
    something = numpy.insert(something[::-1], 0, totalSomething)


if (args.table):
    if args.table.endswith("bz2"):
        table = bz2.BZ2File.open(args.table, "w")
    else:
        table = open(args.table, "w")
    table.write(f"function;time;{'watt' if useVolts else 'current'};{'energy' if useVolts else 'charge'}\n")
    for f, t, s, m in zip(aggregationLabel, times, something, metrics):
        table.write(f"{f};{t};{s};{m}\n")
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
    print(tabulate.tabulate(zip(aggregationLabel, times, something, metrics), headers=['Function', 'Time [s]', 'Watt [W]' if useVolts else 'Current [A]', 'Energy [J]' if useVolts else 'Charge [C]']))
'''
