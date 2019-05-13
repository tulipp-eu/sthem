#!/usr/bin/python3

import argparse
import os
import sys
import subprocess
import pickle
import bz2
import re
import csv

cross_compile = "" if 'CROSS_COMPILE' not in os.environ else os.environ['CROSS_COMPILE']

if cross_compile != "":
    print(f"Using cross compilation prefix '{cross_compile}'")

label_unknown = '_unknown'
label_foreign = '_foreign'

maxPowerSensors = 7

profile = {
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'volts': 1,
    'cpus': 1,
    'target': label_unknown,
    'binaries': [label_unknown, label_foreign],
    'functions_mangled': [label_unknown, label_foreign],
    'functions': [label_unknown, label_foreign],
    'files': [label_unknown, label_foreign],
    'profile': [],
}

_fetched_pc_data = {}

binaryMap = []
kallsyms = []


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


def isPcFromBinary(pc):
    for binary in binaryMap:
        if (pc >= binary['start'] and pc <= binary['end']):
            return binary
    return False


def fetchPCInfo(pc, binary):
    if pc in _fetched_pc_data:
        return _fetched_pc_data[pc]

    srcbinary = binary['binary']
    srcfunction = label_unknown
    srcdemangled = label_unknown
    srcfile = label_unknown
    srcline = 0
    lookupPc = pc if binary['static'] else pc - binary['start']

    if binary['kernel']:
        for f in kallsyms:
            if f[0] <= lookupPc:
                srcfunction = f[1]
                srcdemangled = f[1]
                break
    else:
        addr2line = subprocess.run(f"{cross_compile}addr2line -f -s -e {binary['path']} -a {lookupPc:x}", shell=True, stdout=subprocess.PIPE)
        addr2line.check_returncode()
        result = addr2line.stdout.decode('utf-8').split("\n")
        fileAndLine = result[2].split(':')
        fileAndLine[1] = fileAndLine[1].split(' ')[0]
        if not result[1] == '??':
            srcfunction = result[1]
        if not fileAndLine[0] == '??':
            srcfile = fileAndLine[0]
        if not fileAndLine[1] == '?':
            srcline = int(fileAndLine[1])

        if (srcfunction != label_unknown):
            cppfilt = subprocess.run(f"{cross_compile}c++filt -i {srcfunction}", shell=True, stdout=subprocess.PIPE)
            cppfilt.check_returncode()
            srcdemangled = cppfilt.stdout.decode('utf-8').split("\n")[0]

    if srcbinary not in profile['binaries']:
        profile['binaries'].append(srcbinary)
    if srcfunction not in profile['functions_mangled']:
        profile['functions_mangled'].append(srcfunction)
        profile['functions'].append(srcdemangled)
    if srcfile not in profile['files']:
        profile['files'].append(srcfile)

    result = [
        profile['binaries'].index(srcbinary),
        profile['functions_mangled'].index(srcfunction),
        profile['files'].index(srcfile),
        srcline
    ]

    _fetched_pc_data[pc] = result
    return result


parser = argparse.ArgumentParser(description="Parse sthem csv exports.")
parser.add_argument("csv", help="csv export from sthem")
parser.add_argument("-p", "--power-sensor", help="power sensor to use", type=int)
parser.add_argument("-s", "--search-path", help="add search path", action="append")
parser.add_argument("-o", "--output", help="output profile")
parser.add_argument("-v", "--vmmap", help="vmmap from profiling run")
parser.add_argument("-ks", "--kallsyms", help="parse with kernel symbol file")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile")

args = parser.parse_args()

if (not args.power_sensor) or (args.power_sensor < 1) or (args.power_sensor > maxPowerSensors):
    print("ERROR: invalid power sensor defined!")
    parser.print_help()
    sys.exit(1)

if (not args.output):
    print("ERROR: not output file defined!")
    parser.print_help()
    sys.exit(1)

if (not args.csv) or (not os.path.isfile(args.csv)):
    print("ERROR: profile not found!")
    parser.print_help()
    sys.exit(1)

if (not args.vmmap) or (not os.path.isfile(args.vmmap)):
    print("ERROR: vmmap not found!")
    parser.print_help()
    sys.exit(1)

if (not args.kallsyms) or (not os.path.isfile(args.kallsyms)):
    print("ERROR: kallsyms not found!")
    parser.print_help()
    sys.exit(1)

if (not args.search_path):
    args.search_path = []

useCpus = list(set(convertStringRange(args.cpus)))
profile['cpus'] = len(useCpus)

args.search_path.append(os.getcwd())

print("Reading csv... ", end="")
sys.stdout.flush()

csvProfile = []
if args.csv.endswith(".bz2"):
    with bz2.open(args.csv, "rt") as csvFile:
        csvProfile = list(csv.reader(csvFile, delimiter=";"))
else:
    with open(args.csv, "r") as csvFile:
        csvProfile = list(csv.reader(csvFile, delimiter=";"))


print("finished!")

print("Reading vm maps... ", end="")
sys.stdout.flush()

if (args.bzip2):
    if not args.output.endswith(".bz2"):
        args.output += ".bz2"
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")

aggregatedSamples = {}

vmmaps = []
vmmapFile = open(args.vmmap, 'r').read()
for line in vmmapFile.split("\n"):
    if (len(line) > 2):
        (addr, size, label,) = line.split(" ", 2)
        vmmaps.append([int(addr, 16), int(size, 16), label])


print("finished!")

binaryMap = []

if (args.kallsyms):
    print("Parsing kallsyms... ", end="")
    sys.stdout.flush()
    symbolTable = open(args.kallsyms, "r").read().split("\n")
    for symbol in symbolTable:
        s = symbol.split(" ")
        if len(s) >= 3:
            kallsyms.append([int(s[0], 16), f"_kernel:{s[2]}"])
    binaryMap.append({
        'binary': '_kernel',
        'path': '_kernel',
        'kernel': True,
        'static': True,
        'start': kallsyms[0][0],
        'size': kallsyms[-1][0] - kallsyms[0][0],
        'end': kallsyms[-1][0]
    })
    kallsyms.reverse()
    print("finished")

profile['target'] = vmmaps[0][2]

for map in vmmaps:
    found = False
    path = ""
    static = False
    for searchPath in args.search_path:
        path = f"{searchPath}/{map[2]}"
        if (os.path.isfile(path)):
            readelf = subprocess.run(f"readelf -h {path}", shell=True, stdout=subprocess.PIPE)
            try:
                readelf.check_returncode()
                static = True if re.search("Type:[ ]+EXEC", readelf.stdout.decode('utf-8'), re.M) else False
                found = True
                break
            except Exception as e:
                found = False
    if found:
        binaryMap.append({
            'binary': map[2],
            'path': path,
            'kernel': False,
            'static': static,
            'start': map[0],
            'size': map[1],
            'end': map[0] + map[1]
        })
    else:
        print(f"Could not find {map[2]}!")
        sys.exit(1)

i = 0


csvProfile.pop(0)
profile['samples'] = len(csvProfile)
profile['samplingTime'] = float(csvProfile[-1][0])
avgSampleTime = float(csvProfile[-1][0]) / profile['samples']


for sample in csvProfile:
    if (i % 100 == 0):
        progress = int((i + 1) * 100 / len(csvProfile))
        print(f"Post processing... {progress}%\r", end="")
    i += 1

    processedSample = []
    power = float(sample[args.power_sensor])

    for cpu in useCpus:
        pc = int(sample[maxPowerSensors + cpu + 1])

        cpuSample = [cpu, avgSampleTime / len(useCpus)]
        # binary, function , file, line
        pcInfo = [
            profile['binaries'].index(label_foreign),
            profile['functions_mangled'].index(label_foreign),
            profile['files'].index(label_foreign),
            0
        ]
        binary = isPcFromBinary(pc)
        if binary:
            pcInfo = fetchPCInfo(pc, binary)
        cpuSample.extend(pcInfo)
        processedSample.append(cpuSample)

    profile['profile'].append([power, avgSampleTime, processedSample])


print("Post processing... finished!")

print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")
