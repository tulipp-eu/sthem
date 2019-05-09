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

aggregateKeys = [1]

maxPowerSensors = 7

profile = {
    'samples': 0,
    'samplingTimeUs': 0,
    'latencyTimeUs': 0,
    'volts': 1,
    'target': label_unknown,
    'binaries': [label_unknown, label_foreign],
    'functions_mangled': [label_unknown, label_foreign],
    'functions': [label_unknown, label_foreign],
    'files': [label_unknown, label_foreign],
    'fullProfile': [],
    'aggregatedProfile': {},
    'mean': 1
}

_fetched_pc_data = {}

binaryMap = []


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

    lookupPc = pc

    elf = binary['binary']
    lookupPc = pc if binary['static'] else pc - binary['start']

    addr2line = subprocess.run(f"{cross_compile}addr2line -f -s -e {elf} -a {lookupPc:x}", shell=True, stdout=subprocess.PIPE)
    addr2line.check_returncode()
    result = addr2line.stdout.decode('utf-8').split("\n")
    srcfunction = result[1].replace('??', label_unknown)
    srcfile = result[2].split(':')[0].replace('??', label_unknown)
    srcline = int(result[2].split(':')[1].split(' ')[0].replace('?', '0'))
    srcdemangled = label_unknown

    if (srcfunction != label_unknown):
        cppfilt = subprocess.run(f"{cross_compile}c++filt -i {srcfunction}", shell=True, stdout=subprocess.PIPE)
        cppfilt.check_returncode()
        srcdemangled = cppfilt.stdout.decode('utf-8').split("\n")[0]

    if elf not in profile['binaries']:
        profile['binaries'].append(elf)
    if srcfunction not in profile['functions_mangled']:
        profile['functions_mangled'].append(srcfunction)
        profile['functions'].append(srcdemangled)
    if srcfile not in profile['files']:
        profile['files'].append(srcfile)

    result = [
        profile['binaries'].index(elf),
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
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile")
parser.add_argument("-u", "--unknown", action="store_true", help="dump unknown addresses after postprocessing")

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

if (not args.search_path):
    args.search_path = []

useCpus = list(set(convertStringRange(args.cpus)))

args.search_path.append(os.getcwd())


csvProfile = []
with open(args.csv, "r") as csvFile:
    csvProfile = list(csv.reader(csvFile, delimiter=";"))

print("Reading csv... finished!")

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


print("Reading vm maps... finished!")


profile['target'] = vmmaps[0][2]

binaryMap = []

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
            'binary': path,
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
profile['samplingTimeUs'] = float(csvProfile[-1][0]) * 1000000

for sample in csvProfile:
    if (i % 100 == 0):
        progress = int((i + 1) * 100 / len(csvProfile))
        print(f"Post processing... {progress}%\r", end="")
    i += 1

    processedSample = []
    power = float(sample[args.power_sensor])

    for cpu in useCpus:
        cpuShare = (1 / len(useCpus))
        pc = int(sample[maxPowerSensors + cpu + 1])

        cpuSample = [cpu, cpuShare]
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

        aggregateIndex = ':'.join([str(pcInfo[i]) for i in aggregateKeys])

        if aggregateIndex in profile['aggregatedProfile']:
            profile['aggregatedProfile'][aggregateIndex][0] += 1
            profile['aggregatedProfile'][aggregateIndex][1] += power * cpuShare
        else:
            asample = [1, power * cpuShare]
            asample.extend(pcInfo)
            profile['aggregatedProfile'][aggregateIndex] = asample

    profile['fullProfile'].append([power, processedSample])


print("Post processing... finished!")

print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")
