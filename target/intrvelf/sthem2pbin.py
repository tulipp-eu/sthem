#!/usr/bin/python3

import argparse
import os
import sys
import pickle
import bz2
import csv
import profileLib

_profileVersion = "0.3"

maxPowerSensors = 7

profile = {
    'version': _profileVersion,
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'volts': 1,
    'cpus': 1,
    'target': "",
    'binaries': [],
    'functions': [],
    'files': [],
    'profile': [],
}

parser = argparse.ArgumentParser(description="Parse sthem csv exports.")
parser.add_argument("csv", help="csv export from sthem")
parser.add_argument("-p", "--power-sensor", help="power sensor to use", type=int)
parser.add_argument("-s", "--search-path", help="add search path", action="append")
parser.add_argument("-o", "--output", help="output profile")
parser.add_argument("-v", "--vmmap", help="vmmap from profiling run")
parser.add_argument("-ks", "--kallsyms", help="parse with kernel symbol file")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")

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

if (args.kallsyms) and (not os.path.isfile(args.kallsyms)):
    print("ERROR: kallsyms not found!")
    parser.print_help()
    sys.exit(1)

if (not args.search_path):
    args.search_path = []
args.search_path.append(os.getcwd())

useCpus = list(set(profileLib.parseRange(args.cpus)))
profile['cpus'] = len(useCpus)

sampleParser = profileLib.sampleParser()

sampleParser.addSearchPath(args.search_path)

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
sampleParser.loadVMMap(args.vmmap)
print("finished")

profile['target'] = sampleParser.binaries[0]['binary']

if (args.kallsyms):
    sampleParser.loadKallsyms(args.kallsyms)

# print("Not using skewed pc adjustment!")
# sampleParser.enableSkewedPCAdjustment()

profile['samples'] = len(csvProfile)
profile['samplingTime'] = float(csvProfile[-1][0])
avgSampleTime = float(csvProfile[-1][0]) / profile['samples']



i = 0
csvProfile.pop(0)
for sample in csvProfile:
    if (i % 1000 == 0):
        progress = int((i + 1) * 100 / len(csvProfile))
        print(f"Post processing... {progress}%\r", end="")
    i += 1

    power = float(sample[args.power_sensor])
    processedSample = []
    for cpu in useCpus:
        pc = int(sample[maxPowerSensors + cpu + 1])
        processedSample.append([cpu, avgSampleTime, sampleParser.parseFromPC(pc)])

    profile['profile'].append([power, (avgSampleTime * len(useCpus)), processedSample])

del csvProfile

profile['binaries'] = sampleParser.getBinaryMap()
profile['functions'] = sampleParser.getFunctionMap()
profile['files'] = sampleParser.getFileMap()

print("Post processing... finished!")

print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
if args.output.endswith(".bz2"):
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()

print("finished")
