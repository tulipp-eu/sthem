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


def error(baseline, value, totalBaseline, totalValue, weight):
    return value - baseline


def weightedError(baseline, value, totalBaseline, totalValue, weight):
    if totalBaseline == 0:
        return 0
    else:
        return error(baseline, value, totalBaseline, totalValue, weight) * weight


def absoluteWeightedError(baseline, value, totalBaseline, totalValue, weight):
    return abs(weightedError(baseline, value, totalBaseline, totalValue, weight))


def absoluteError(baseline, value, totalBaseline, totalValue, weight):
    return abs(error(baseline, value, totalBaseline, totalValue, weight))


def relativeError(baseline, value, totalBaseline, totalValue, weight):
    return error(baseline, value, totalBaseline, totalValue, weight) / baseline if (baseline != 0) else 0


def absoluteRelativeError(baseline, value, totalBaseline, totalValue, weight):
    return abs(relativeError(baseline, value, totalBaseline, totalValue, weight))


def weightedRelativeError(baseline, value, totalBaseline, totalValue, weight):
    return relativeError(baseline, value, totalBaseline, totalValue, weight) * weight


def absoluteWeightedRelativeError(baseline, value, totalBaseline, totalValue, weight):
    return abs(weightedRelativeError(baseline, value, totalBaseline, totalValue, weight))


# values are already processed by errorFunction
def aggregateTotal(baselines, values, weightValues, weightTotal):
    return sum(values)


# values are already processed by errorFunction
def aggregateMin(baselines, values, totalBaseline, totalValue):
    return min(values)


# values are already processed by errorFunction
def aggregateMax(baselines, values, totalBaseline, totalValue):
    return max(values)


# values are already processed by errorFunction
def aggregateMean(baselines, values, totalBaseline, totalValue):
    return sum(values) / len(values)


def aggregateRootMeanSquaredError(baselines, values, totalBaseline, totalValue, weights):
    return math.sqrt(sum([math.pow(error(baseline, value, totalBaseline, totalValue, weight), 2) for baseline, value, weight in zip(baselines, values, weights)]) / len(values))


def aggregateWeightedRootMeanSquaredError(baselines, values, totalBaseline, totalValue, weights):
    return math.sqrt(sum([math.pow(error(baseline, value, totalBaseline, totalValue, weight), 2) * weight for baseline, value, weight in zip(baselines, values, weights)]))


_aggregatedProfileVersion = "a0.4"

# [ parameter, description, error function,  ]
errorFunctions = numpy.array([
    ['relative_error', 'Relative Error', relativeError],
    ['error', 'Error', error],
    ['absolute_error', 'Absolute Error', absoluteError],
    ['weighted_error', 'Weighted Error', weightedError],
    ['absolute_weighted_error', 'Absolute Weighted Error', absoluteWeightedError],
    ['absolute_relative_error', 'Absolute Relative Error', absoluteRelativeError],
    ['weighted_relative_error', 'Weighted Relative Error', weightedRelativeError],
    ['absolute_weighted_relative_error', 'Absolute Weighted Relative Error', absoluteWeightedRelativeError],
], dtype=object)

aggregateFunctions = numpy.array([
    ['total', 'Total', aggregateTotal, True],
    ['min', 'Minimum', aggregateMin, True],
    ['max', 'Maximum', aggregateMax, True],
    ['mean', 'Mean', aggregateMean, True],
    ['rmse', 'Root Mean Squared Error', aggregateRootMeanSquaredError, False],
    ['wrmse', 'Weighted Root Mean Squared Error', aggregateWeightedRootMeanSquaredError, False]
])

parser = argparse.ArgumentParser(description="Visualize profiles from intrvelf sampler.")
parser.add_argument("profile", help="baseline aggregated profile")
parser.add_argument("profiles", help="aggregated profiles to compare", nargs="+")
parser.add_argument("--use-time", help="compare time values", action="store_true", default=False)
parser.add_argument("--use-energy", help="compare energy values (default)", action="store_true", default=False)
parser.add_argument("--use-power", help="compare power values", action="store_true", default=False)
parser.add_argument("-e", "--error", help=f"error function (default: {errorFunctions[0][0]})", default=False, choices=errorFunctions[:, 0], type=str.lower)
parser.add_argument("-a", "--aggregate", help="aggregate erros", default=False, choices=aggregateFunctions[:, 0], type=str.lower)
parser.add_argument("-c", "--compensation", help="switch on latency compensation (experimental)", action="store_true", default=False)
parser.add_argument("--time-threshold", help="time threshold to include (in percent, e.g. 0.0 - 1.0)", type=float, default=0)
parser.add_argument("--energy-threshold", help="power threshold to include (in percent, e.g. 0.0 - 1.0)", type=float, default=0)
parser.add_argument('--names', help='names of the provided profiles (comma sepperated)', type=str, default=False)
parser.add_argument('-n', '--name', action='append', help='name the provided profiles', default=[])
parser.add_argument("-t", "--table", help="output csv table")
parser.add_argument("-p", "--plot", help="plotly html file")
parser.add_argument("-q", "--quiet", action="store_true", help="do not automatically open plot file", default=False)


args = parser.parse_args()

if (not args.use_time and not args.use_energy and not args.use_power):
    args.use_energy = True

if (len(args.name) == 0 and args.names is not False):
    args.name = args.names.split(',')

header = ""

compareOffset = 3
if args.use_time:
    header = "Time "
    compareOffset = 0
if args.use_power:
    header = "Power "
    compareOffset = 2
if args.use_energy:
    header = "Energy "
    compareOffset = 3


if (args.time_threshold is not 0 and (args.time_threshold < 0 or args.time_threshold > 1.0)):
    print("ERROR: time threshold out of range")
    parser.print_help()
    sys.exit(0)

if (args.energy_threshold is not 0 and (args.energy_threshold < 0 or args.energy_threshold > 1.0)):
    print("ERROR: energy threshold out of range")
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


totals = False  # [0, 0]
# for key in baselineProfile['profile']:
#     totals[0] += baselineProfile['profile'][key][0]
#    totals[1] += baselineProfile['profile'][key][1]

errorFunction = False
aggregateFunction = False

if args.aggregate is not False:
    chosenAggregateFunction = aggregateFunctions[numpy.where(aggregateFunctions == args.aggregate)[0][0]]
    aggregateFunction = chosenAggregateFunction[2]

if args.aggregate is not False and not chosenAggregateFunction[3] and args.error is not False:
    print(f"NOTICE: error function does not have an influence on '{chosenAggregateFunction[1]}'")
    args.error = False

if args.error is False and args.aggregate is False:  # default value
    args.error = errorFunctions[0][0]

if args.error is not False:
    chosenErrorFunction = errorFunctions[numpy.where(errorFunctions == args.error)[0][0]]
    errorFunction = chosenErrorFunction[2]


fullTotals = [0.0, 0.0, 0.0, 0.0]
for key in baselineProfile['profile']:
    fullTotals[0] += baselineProfile['profile'][key][0]
    fullTotals[1] += baselineProfile['profile'][key][1]
    fullTotals[3] += baselineProfile['profile'][key][3]
fullTotals[1] = (fullTotals[0] / fullTotals[1])
fullTotals[2] = (fullTotals[3] / fullTotals[0]) if fullTotals[0] != 0 else 0


chart = {'name': '', 'fullTotals': [0.0, 0.0, 0.0, 0.0], 'totals': [0.0, 0.0, 0.0, 0.0], 'keys': {}}
errorCharts = [copy.deepcopy(chart) for x in args.profiles]


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
            if (((args.time_threshold == 0) or ((baselineProfile['profile'][key][0] / fullTotals[0]) >= args.time_threshold)) and
               ((args.energy_threshold == 0) or ((baselineProfile['profile'][key][3] / fullTotals[3]) >= args.energy_threshold))):
                for chart in errorCharts:
                    if key not in chart['keys']:
                        chart['keys'][key] = [
                            baselineProfile['profile'][key][4],  # label
                            baselineProfile['profile'][key][0],  # time
                            baselineProfile['profile'][key][0] / baselineProfile['profile'][key][1],  # avg execution time
                            baselineProfile['profile'][key][2],  # power
                            baselineProfile['profile'][key][3],  # energy
                            0.0,  # time
                            0.0,  # avg execution time
                            0.0,  # power
                            0.0,  # energy
                            0.0,  # time error
                            0.0,  # avg execution time error
                            0.0,  # power error
                            0.0,  # energy error
                            1.0   # weight
                        ]
                errorCharts[index]['keys'][key][5] = profile['profile'][key][0]
                errorCharts[index]['keys'][key][6] = profile['profile'][key][0] / profile['profile'][key][1]
                errorCharts[index]['keys'][key][7] = profile['profile'][key][2]
                errorCharts[index]['keys'][key][8] = profile['profile'][key][3]
                errorCharts[index]['totals'][0] += profile['profile'][key][0]
                errorCharts[index]['totals'][1] += profile['profile'][key][1]
                errorCharts[index]['totals'][3] += profile['profile'][key][3]
            errorCharts[index]['fullTotals'][0] += profile['profile'][key][0]
            errorCharts[index]['fullTotals'][1] += profile['profile'][key][1]
            errorCharts[index]['fullTotals'][3] += profile['profile'][key][3]

    errorCharts[index]['totals'][1] += (errorCharts[index]['totals'][0] / errorCharts[index]['totals'][1]) if errorCharts[index]['totals'][1] != 0 else 0
    errorCharts[index]['totals'][2] += (errorCharts[index]['totals'][3] / errorCharts[index]['totals'][0]) if (errorCharts[index]['totals'][0] != 0) else 0
    errorCharts[index]['fullTotals'][1] += errorCharts[index]['fullTotals'][0] / errorCharts[index]['fullTotals'][1]
    errorCharts[index]['fullTotals'][2] += (errorCharts[index]['fullTotals'][3] / errorCharts[index]['fullTotals'][0]) if (errorCharts[index]['fullTotals'][0] != 0) else 0

    del profile


if len(errorCharts[0]['keys']) == 0:
    raise Exception("Nothing found to compare, limit too strict?")

# calculate baseline total
if totals is False:
    values = numpy.array(list(errorCharts[0]['keys'].values()), dtype=object)
    totals = [numpy.sum(values[:, 1]), numpy.sum(values[:, 2]) / len(values[:, 2]), numpy.sum(values[:, 3]), numpy.sum(values[:, 4])]

# fill in the weights, based on baseline energy
for key in errorCharts[0]['keys']:
    for chart in errorCharts:
        chart['keys'][key][10] = chart['keys'][key][3] / totals[2]

# fill in the errors
if errorFunction is not False:
    for key in errorCharts[0]['keys']:
        for chart in errorCharts:
            chart['keys'][key][9] = errorFunction(chart['keys'][key][1], chart['keys'][key][5], totals[0], chart['totals'][0], chart['keys'][key][13])
            chart['keys'][key][10] = errorFunction(chart['keys'][key][2], chart['keys'][key][6], totals[1], chart['totals'][1], chart['keys'][key][13])
            chart['keys'][key][11] = errorFunction(chart['keys'][key][3], chart['keys'][key][7], totals[2], chart['totals'][2], chart['keys'][key][13])
            chart['keys'][key][12] = errorFunction(chart['keys'][key][4], chart['keys'][key][8], totals[3], chart['totals'][3], chart['keys'][key][13])


# names = [ key, name1, name2, name3, name4 ]
# values = [ key, error1, error2, error3, error4 ]a
#


if aggregateFunction:
        header += f"{chosenAggregateFunction[1]} "
if errorFunction:
        header += f"{chosenErrorFunction[1]}"
header = header.strip()

if errorFunction is not False and aggregateFunction is False:
    headers = numpy.array([chart['name'] for chart in errorCharts])
    tmp = numpy.array(list(errorCharts[0]['keys'].values()), dtype=object)
    rows = numpy.array([x + f', {t * 1000:.3f} ms' for x, t in zip(tmp[:, 0].flatten(), tmp[:, 2].flatten())], dtype=object).reshape(-1, 1)
    execTimes = numpy.empty((rows.shape[0], 0))
    del tmp
    for chart in errorCharts:
        values = numpy.array(list(chart['keys'].values()), dtype=object)
        rows = numpy.append(rows, values[:, (9 + compareOffset)].reshape(-1, 1), axis=1)
        execTimes = numpy.append(execTimes, values[:, 6].reshape(-1, 1), axis=1)

    asort = rows[:, 1].argsort()
    rows = rows[asort]
    execTimes = execTimes[asort]

#    if args.limit != 0:
#        nrows = numpy.empty((0, rows.shape[1]), dtype=object)
#        for row in rows:
#            if max(map(abs, list(row[1:]))) >= args.limit:
#                nrows = numpy.append(nrows, [row], axis=0)
#        rows = nrows

if aggregateFunction is not False:
    rows = numpy.array([chart['name'] for chart in errorCharts], dtype=object).reshape(-1, 1)
    execTimes = numpy.array([''] * len(rows)).reshape(1, -1)
    errors = numpy.empty(0)
    for chart in errorCharts:
        chartValues = numpy.array(list(chart['keys'].values()), dtype=object)
        errors = numpy.append(errors, aggregateFunction(
            chartValues[:, (1 + compareOffset)],
            chartValues[:, ((5 if errorFunction is False else 9) + compareOffset)],
            totals[0 + compareOffset],
            chart['totals'][0 + compareOffset],
            chartValues[:, 13]
        ))
    rows = numpy.append(rows, errors.reshape(-1, 1), axis=1)
    headers = numpy.array([header], dtype=object)

if (args.plot):
    fig = {'data': []}
    maxLen = 0
    for index, name in enumerate(headers):
        pAggregationLabel = [textwrap.fill(x, 64).replace('\n', '<br />') for x in rows[:, 0]]
        pExecTimes = [(f'{t * 1000:.3f} ms' if isinstance(t, float) else t) for t in execTimes[:, index]]
        fig["data"].append(
            go.Bar(
                y=pAggregationLabel,
                x=rows[:, (index + 1)],
                name=name,
                text=pExecTimes,
                textposition='auto',
                orientation='h',
                hoverinfo='name+x' if aggregateFunction is False else 'y+x'
            )
        )
        maxLen = max(maxLen, numpy.max([len(x) for x in rows[:, 0]]))

    fig['layout'] = go.Layout(
        title=go.layout.Title(
            text=f"{header}, {baselineProfile['target']}, Frequency {baselineProfile['samples'] / baselineProfile['samplingTime']:.2f} Hz, Time {baselineProfile['samplingTime']:.2f} s, Energy {fullTotals[3]:.2f} J, Latency {baselineProfile['latencyTime'] * 1000000 / baselineProfile['samples']:.2f} us",
            xref='paper',
            x=0
        ),
        xaxis=go.layout.XAxis(
            title=go.layout.xaxis.Title(
                text=header,
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
        legend=go.layout.Legend(traceorder="reversed"),
        margin=go.layout.Margin(l=6.8 * min(64, maxLen))
    )

    plotly.offline.plot(fig, filename=args.plot, auto_open=not args.quiet)
    print(f"Plot saved to {args.plot}")
    del pAggregationLabel
    del fig


if (args.table or not args.quiet):
    if aggregateFunction is False:
        coverage = ['_coverage']
        coverage.extend([chart['totals'][0 + compareOffset] / chart['fullTotals'][0 + compareOffset] if chart['fullTotals'][0 + compareOffset] != 0 else 0 for chart in errorCharts])
        total = ['_total']
        for i in range(1, len(rows[0])):
            total.append(numpy.sum(rows[:, (i)]))
        rows = numpy.concatenate(([total], rows[::-1]), axis=0)
        rows = numpy.concatenate(([coverage], rows), axis=0)
    else:
        header = "Profile"
        rows = rows[::-1]

if (args.table):
    if args.table.endswith("bz2"):
        table = bz2.BZ2File.open(args.table, "w")
    else:
        table = open(args.table, "w")
    table.write(header + ";" + ';'.join(headers) + "\n")
    for x in rows:
        table.write(';'.join([f"{y:.16f}" if not isinstance(y, str) else y for y in x]) + "\n")
    table.close()
    print(f"CSV saved to {args.table}")

if (not args.quiet):
    headers = numpy.append([header], headers)

    rows[:, 0] = [textwrap.fill(x, 64) for x in rows[:, 0]]
    print(tabulate.tabulate(rows, headers=headers))
