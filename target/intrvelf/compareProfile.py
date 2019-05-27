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
import copy
import math


# x - baseline, y - compare, z - total of baseline
def difference(x, y, z):
    return y - x


def absoluteDifference(x, y, z):
    return abs(difference(x, y, z))


def error(x, y, z):
    return difference(x, y, z) / x if x != 0 else 0


def absoluteError(x, y, z):
    return abs(error(x, y, z))


def weightedError(x, y, z):
    return error(x, y, z) * (x / z)


def absoluteWeightedError(x, y, z):
    return abs(weightedError(x, y, z))


# x - error value array
def aggregateTotalError(x):
    return sum(x)


def aggregateMeanError(x):
    return sum(x) / len(x)


def aggregateRootMeanSquaredError(x):
    return math.sqrt(sum([math.pow(t, 2) for t in x]) / len(x))


_aggregatedProfileVersion = "a0.2"

# [ parameter, description, error function,  ]
errorFunctions = numpy.array([
    ['absolute_weighted_error', 'Absolute Weighted Error', absoluteWeightedError],
    ['difference', 'Difference', difference],
    ['absolute_difference', 'Absolute Difference', absoluteDifference],
    ['error', 'Error', error],
    ['absolute_error', 'Absolute Error', absoluteError],
    ['weighted_error', 'Weighted Error', weightedError],
], dtype=object)

aggregateErrorFunctions = numpy.array([
    ['none', 'None', False],
    ['total', 'Total Error', aggregateTotalError],
    ['mean', 'Mean Error', aggregateMeanError],
    ['rmse', 'Root Mean Squared Error', aggregateRootMeanSquaredError]
])

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="baseline aggregated profile")
parser.add_argument("profiles", help="aggregated profiles to compare", nargs="+")
parser.add_argument("-e", "--error", help="error function (default: %(default)s)", default=errorFunctions[0][0], choices=errorFunctions[:, 0])
parser.add_argument("-a", "--aggregate", help="aggregate erros (default: %(default)s)", default=aggregateErrorFunctions[0][0], choices=aggregateErrorFunctions[:, 0])
parser.add_argument("-l", "--limit", help="error threshold to include", type=float, default=0)
parser.add_argument('-n', '--name', action='append', help='name the provided profiles', default=[])


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
totalEnergy = 0
for key in baselineProfile['profile']:
    totalTime += baselineProfile['profile'][key][0]
    totalEnergy += baselineProfile['profile'][key][1]

chart = {'name': '', 'keys': {}}
errorCharts = [copy.deepcopy(chart) for x in args.profiles]

errorFunctionIndex = numpy.where(errorFunctions == args.error)[0][0]
errorFunction = errorFunctions[errorFunctionIndex][2]
aggregateErrorFunctionIndex = numpy.where(aggregateErrorFunctions == args.aggregate)[0][0]
aggregateErrorFunction = aggregateErrorFunctions[aggregateErrorFunctionIndex][2]

i = 1
for index, path in enumerate(args.profiles):
    print(f"Compare profile {i}/{len(args.profiles)}...\r", end="")
    i += 1

    if path.endswith(".bz2"):
        profile = pickle.load(bz2.BZ2File(path, mode="rb"))
    else:
        profile = pickle.load(open(path, mode="rb"))

    if 'version' not in profile or profile['version'] != _aggregatedProfileVersion:
        raise Exception(f"Incompatible profile version (required: {_aggregatedProfileVersion})")

    if len(args.name) > index:
        errorCharts[index]['name'] = args.name[index]
    else:
        errorCharts[index]['name'] = f"{profile['samples'] / profile['samplingTime']:.2f} Hz, {profile['samplingTime']:.2f} s, {profile['latencyTime'] * 1000000 / profile['samples']:.2f} us"

    # profile['profile] = [[time, energy, label]]
    for key in profile['profile']:
        if key in baselineProfile['profile']:
            for chart in errorCharts:
                if key not in chart['keys']:
                    chart['keys'][key] = [
                        baselineProfile['profile'][key][2],  # label
                        baselineProfile['profile'][key][0],  # time
                        baselineProfile['profile'][key][1],  # energy
                        0.0,  # time error
                        0.0,  # energy error
                    ]
            errorCharts[index]['keys'][key][3] = errorFunction(baselineProfile['profile'][key][0], profile['profile'][key][0], totalTime)
            errorCharts[index]['keys'][key][4] = errorFunction(baselineProfile['profile'][key][1], profile['profile'][key][1], totalEnergy)

    del profile

# names = [ key, name1, name2, name3, name4 ]
# values = [ key, error1, error2, error3, error4 ]a
#

values = []
names = []

if aggregateErrorFunction is not False:
    for chart in errorCharts:
        temp = numpy.array(list(chart['keys'].values()), dtype=object)
        values.append([chart['name'], aggregateErrorFunction(temp[:, 4])])
    names = [aggregateErrorFunctions[aggregateErrorFunctionIndex][1]]
else:
    names = [chart['name'] for chart in errorCharts]
    for key in errorCharts[0]['keys']:
        errors = [charts['keys'][key][4] for charts in errorCharts]

        if args.limit == 0 or max(map(abs, errors)) >= args.limit:
            entry = [errorCharts[0]['keys'][key][0]]
            entry.extend(errors)
            values.append(entry)


values = numpy.array(values, dtype=object)
values = values[values[:, 1].argsort()]

if (args.plot):
    fig = {'data': []}
    maxLen = 0
    for index, name in enumerate(names):
        energies = numpy.array(values[:, (index + 1)], dtype=float)
        aggregationLabel = values[:, 0]
        pAggregationLabel = [textwrap.fill(x, 64).replace('\n', '<br />') for x in aggregationLabel]
        fig["data"].append(
            go.Bar(
                y=pAggregationLabel,
                x=energies,
                name=name,
                textposition='auto',
                orientation='h',
                hoverinfo='x+y'
            )
        )
        maxLen = max(maxLen, numpy.max([len(x) for x in aggregationLabel]))

    fig['layout'] = go.Layout(
        title=go.layout.Title(
            text=f"{errorFunctions[errorFunctionIndex][1]}, {baselineProfile['target']}, Frequency {baselineProfile['samples'] / baselineProfile['samplingTime']:.2f} Hz, Time {baselineProfile['samplingTime']:.2f} s, Latency {baselineProfile['latencyTime'] * 1000000 / baselineProfile['samples']:.2f} us",
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

    plotly.offline.plot(fig, filename=args.plot, auto_open=not args.quiet)
    print(f"Plot saved to {args.plot}")
    del pAggregationLabel
    del fig


if (args.table or not args.quiet):
    if aggregateErrorFunction is False:
        total = ['_total']
        for i in range(1, len(values[0])):
            total.append(numpy.sum(values[:, (i)]))
        values = numpy.concatenate(([total], values[::-1]), axis=0)
    else:
        values = values[::-1]
    # metrics = numpy.insert(metrics[::-1], 0, totalMetric)
    # something = numpy.insert(something[::-1], 0, totalSomething)

if (args.table):
    if args.table.endswith("bz2"):
        table = bz2.BZ2File.open(args.table, "w")
    else:
        table = open(args.table, "w")
    table.write("key;" + ';'.join(names) + "\n")
    for x in values:
        table.write(';'.join([f"{y:.16f}" if not isinstance(y, str) else y for y in x]) + "\n")
    table.close()
    print(f"CSV saved to {args.table}")

if (not args.quiet):
    headers = ['Key']
    headers.extend(names)

    values[:, 0] = [textwrap.fill(x, 64) for x in values[:, 0]]
    print(tabulate.tabulate(values, headers=headers, floatfmt=".16f"))
