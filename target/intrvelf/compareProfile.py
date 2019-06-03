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


def error(baseline, value, totalBaseline, totalValue):
    return value - baseline


def weightedError(baseline, value, totalBaseline, totalValue):
    if totalBaseline == 0:
        return 0
    else:
        return error(baseline, value, totalBaseline, totalValue) * (baseline / totalBaseline)


def absoluteWeightedError(baseline, value, totalBaseline, totalValue):
    return abs(weightedError(baseline, value, totalBaseline, totalValue))


def absoluteError(baseline, value, totalBaseline, totalValue):
    return abs(error(baseline, value, totalBaseline, totalValue))


def percentage(baseline, value, totalBaseline, totalValue):
    if baseline == 0:
        return 0
    else:
        return error(baseline, value, totalBaseline, totalValue) / baseline


def percentageOfTotal(baseline, value, totalBaseline, totalValue):
    if totalValue == 0:
        return 0
    else:
        return value / totalValue


def absolutePercentage(baseline, value, totalBaseline, totalValue):
    return abs(percentage(baseline, value, totalBaseline, totalValue))


def weightedPercentage(baseline, value, totalBaseline, totalValue):
    if totalBaseline == 0:
        return 0
    else:
        return percentage(baseline, value, totalBaseline, totalValue) * (baseline / totalBaseline)


def absoluteWeightedPercentage(baseline, value, totalBaseline, totalValue):
    return abs(weightedPercentage(baseline, value, totalBaseline, totalValue))


# values are already processed by errorFunction
def aggregateTotal(baselines, values, totalBaseline, totalValue):
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


def aggregateRootMeanSquaredError(baselines, values, totalBaseline, totalValue):
    return math.sqrt(sum([math.pow(error(baseline, value, totalBaseline, totalValue), 2) for baseline, value in zip(baselines, values)]) / len(values))


def aggregateWeightedRootMeanSquaredError(baselines, values, totalBaseline, totalValue):
    if totalBaseline == 0:
        return 0
    else:
        return math.sqrt(sum([math.pow(error(baseline, value, totalBaseline, totalValue), 2) * (baseline / totalBaseline) for baseline, value in zip(baselines, values)]))


_aggregatedProfileVersion = "a0.3"

# [ parameter, description, error function,  ]
errorFunctions = numpy.array([
    ['absolute_weighted_percentage', 'Absolute Weighted Percentage', absoluteWeightedPercentage],
    ['error', 'Error', error],
    ['absolute_error', 'Absolute Error', absoluteError],
    ['weighted_error', 'Weighted Error', weightedError],
    ['absolute_weighted_error', 'Absolute Weighted Error', absoluteWeightedError],
    ['percentage', 'Percentage', percentage],
    ['percentage_of_total', 'Percentage Of Total', percentageOfTotal],
    ['absolute_percentage', 'Absolute Percentage', absolutePercentage],
    ['weighted_percentage', 'Weighted Percentage', weightedPercentage],
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
parser.add_argument("-l", "--limit", help="time threshold to include", type=float, default=0)
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

compareOffset = 2
if args.use_time:
    header = "Time "
    compareOffset = 0
if args.use_power:
    header = "Power "
    compareOffset = 1
if args.use_energy:
    header = "Energy "
    compareOffset = 2


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


baselineCompensation = (1 / (baselineProfile['latencyTime'] / baselineProfile['samplingTime'])) if (args.compensation and baselineProfile['latencyTime'] != 0) else 1

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

chart = {'name': '', 'fullTotals': [0.0, 0.0, 0.0], 'totals': [0.0, 0.0, 0.0], 'keys': {}}
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

    profileCompensation = 1 - (profile['latencyTime'] / profile['samplingTime']) if args.compensation and profile['latencyTime'] != 0 else 1
    # profile['profile] = [[time, energy, label]]
    for key in profile['profile']:
        if key in baselineProfile['profile']:
            if args.limit == 0 or (baselineProfile['profile'][key][0] * baselineCompensation) >= args.limit:
                for chart in errorCharts:
                    if key not in chart['keys']:
                        chart['keys'][key] = [
                            baselineProfile['profile'][key][3],  # label
                            baselineProfile['profile'][key][0],  # time
                            baselineProfile['profile'][key][1],  # power
                            baselineProfile['profile'][key][2],  # energy
                            baselineProfile['profile'][key][0],  # time
                            baselineProfile['profile'][key][1],  # power
                            baselineProfile['profile'][key][2],  # energy
                            0.0,  # time error
                            0.0,  # power error
                            0.0   # energy error
                        ]
                errorCharts[index]['keys'][key][4] = profile['profile'][key][0]
                errorCharts[index]['keys'][key][5] = profile['profile'][key][1]
                errorCharts[index]['keys'][key][6] = profile['profile'][key][2]
                errorCharts[index]['totals'][0] += errorCharts[index]['keys'][key][4]
                errorCharts[index]['totals'][2] += errorCharts[index]['keys'][key][6]
            errorCharts[index]['fullTotals'][0] += profile['profile'][key][0]
            errorCharts[index]['fullTotals'][2] += profile['profile'][key][2]

    errorCharts[index]['totals'][1] += (errorCharts[index]['totals'][2] / errorCharts[index]['totals'][0]) if (errorCharts[index]['totals'][0] != 0) else 0
    errorCharts[index]['fullTotals'][1] += (errorCharts[index]['fullTotals'][2] / errorCharts[index]['fullTotals'][0]) if (errorCharts[index]['fullTotals'][0] != 0) else 0

    del profile


if len(errorCharts[0]['keys']) == 0:
    raise Exception("Nothing found to compare, limit too strict?")

if totals is False:
    values = numpy.array(list(errorCharts[0]['keys'].values()), dtype=object)
    totals = [numpy.sum(values[:, 1]), numpy.sum(values[:, 2]), numpy.sum(values[:, 3])]

if errorFunction is not False:
    for key in errorCharts[0]['keys']:
        for chart in errorCharts:
            chart['keys'][key][7] = errorFunction(chart['keys'][key][1], chart['keys'][key][4], totals[0], chart['totals'][0])
            chart['keys'][key][8] = errorFunction(chart['keys'][key][2], chart['keys'][key][5], totals[1], chart['totals'][1])
            chart['keys'][key][9] = errorFunction(chart['keys'][key][3], chart['keys'][key][6], totals[2], chart['totals'][2])


# names = [ key, name1, name2, name3, name4 ]
# values = [ key, error1, error2, error3, error4 ]a
#


if errorFunction:
        header += f"{chosenErrorFunction[1]}"
if aggregateFunction:
        header += f"{chosenAggregateFunction[1]} {header}"
header = header.strip()

if errorFunction is not False and aggregateFunction is False:
    headers = numpy.array([chart['name'] for chart in errorCharts])
    rows = numpy.array(list(errorCharts[0]['keys'].values()), dtype=object)[:, 0].reshape(-1, 1)
    for chart in errorCharts:
        values = numpy.array(list(chart['keys'].values()), dtype=object)
        rows = numpy.append(rows, values[:, (7 + compareOffset)].reshape(-1, 1), axis=1)

#    if args.limit != 0:
#        nrows = numpy.empty((0, rows.shape[1]), dtype=object)
#        for row in rows:
#            if max(map(abs, list(row[1:]))) >= args.limit:
#                nrows = numpy.append(nrows, [row], axis=0)
#        rows = nrows

if aggregateFunction is not False:
    rows = numpy.array([chart['name'] for chart in errorCharts], dtype=object).reshape(-1, 1)
    errors = numpy.empty(0)
    for chart in errorCharts:
        chartValues = numpy.array(list(chart['keys'].values()), dtype=object)
        errors = numpy.append(errors, aggregateFunction(
            chartValues[:, (1 + compareOffset)],
            chartValues[:, ((4 if errorFunction is False else 7) + compareOffset)],
            totals[0 + compareOffset],
            chart['totals'][0 + compareOffset]
        ))
    rows = numpy.append(rows, errors.reshape(-1, 1), axis=1)
    headers = numpy.array([header], dtype=object)
    header = "Profile"


rows = rows[rows[:, 1].argsort()]

if (args.plot):
    fig = {'data': []}
    maxLen = 0
    for index, name in enumerate(headers):
        pAggregationLabel = [textwrap.fill(x, 64).replace('\n', '<br />') for x in rows[:, 0]]
        fig["data"].append(
            go.Bar(
                y=pAggregationLabel,
                x=rows[:, (index + 1)],
                name=name,
                textposition='auto',
                orientation='h',
                hoverinfo='x+y'
            )
        )
        maxLen = max(maxLen, numpy.max([len(x) for x in rows[:, 0]]))

    fig['layout'] = go.Layout(
        title=go.layout.Title(
            text=f"{header}, {baselineProfile['target']}, Frequency {baselineProfile['samples'] / baselineProfile['samplingTime']:.2f} Hz, Time {baselineProfile['samplingTime']:.2f} s, Latency {baselineProfile['latencyTime'] * 1000000 / baselineProfile['samples']:.2f} us",
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
