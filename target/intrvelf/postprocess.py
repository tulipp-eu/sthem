#!/usr/bin/python3

import argparse
import os
import sys
import struct
import pickle
import bz2
import profileLib

_profileVersion = "0.1"

profile = {
    'version': _profileVersion,
    'samples': 0,
    'samplingTime': 0,
    'latencyTime': 0,
    'volts': 0,
    'cpus': 0,
    'target': "",
    'binaries': [],
    'functions': [],
    'files': [],
    'profile': [],
}

parser = argparse.ArgumentParser(description="Parse profiles from intrvelf sampler.")
parser.add_argument("profile", help="profile from intrvelf")
parser.add_argument("-v", "--volts", help="set pmu voltage")
parser.add_argument("-s", "--search-path", help="add search path", action="append")
parser.add_argument("-o", "--output", help="write postprocessed profile")
parser.add_argument("-c", "--cpus", help="list of active cpu cores", default="0-3")
parser.add_argument("-l", "--little-endian", action="store_true", help="parse profile using little endianess")
parser.add_argument("-b", "--big-endian", action="store_true", help="parse profile using big endianess")

args = parser.parse_args()

if (not args.output):
    print("ERROR: not output file defined!")
    parser.print_help()
    sys.exit(1)

if (not args.profile) or (not os.path.isfile(args.profile)):
    print("ERROR: profile not found!")
    parser.print_help()
    sys.exit(1)

if (args.volts):
    profile['volts'] = float(args.volts)

if (not args.search_path):
    args.search_path = []
args.search_path.append(os.getcwd())

binProfile = open(args.profile, mode='rb').read()

forced = False
endianess = "="

if (args.little_endian):
    endianess = "<"
    forced = True

if (args.big_endian):
    endianess = ">"
    forced = True

binOffset = 0

useCpus = list(set(profileLib.parseRange(args.cpus)))
profile['cpus'] = len(useCpus)

try:
    (magic,) = struct.unpack_from(endianess + "I", binProfile, binOffset)
    binOffset += 4
except Exception as e:
    print("Unexpected end of file!")
    sys.exit(1)

if (magic != 1):
    print("Invalid profile")
    sys.exit(1)


try:
    (wallTimeUs, latencyTimeUs, sampleCount, vmmapCount) = struct.unpack_from(endianess + 'QQQI', binProfile, binOffset)
    binOffset += 28
except Exception as e:
    print("Unexpected end of file!")
    sys.exit(1)


if (sampleCount == 0):
    print("No samples found in profile!")
    sys.exit(1)

profile['samples'] = sampleCount
profile['samplingTime'] = (wallTimeUs / 1000000.0)
profile['latencyTime'] = (latencyTimeUs / 1000000.0)

rawSamples = []

for i in range(sampleCount):
    if (i % 1000 == 0):
        progress = int((i + 1) * 100 / sampleCount)
        print(f"Reading raw samples... {progress}%\r", end="")

    try:
        (current, threadCount, ) = struct.unpack_from(endianess + "dI", binProfile, binOffset)
        binOffset += 8 + 4
    except Exception as e:
        print("Unexpected end of file!")
        sys.exit(1)

    sample = []
    for j in range(threadCount):
        try:
            (tid, pc, cpuTimeNs, ) = struct.unpack_from(endianess + "IQQ", binProfile, binOffset)
            binOffset += 4 + 8 + 8
        except Exception as e:
            print("Unexpected end of file!")
            sys.exit(1)
        sample.append([tid, pc, (cpuTimeNs / 1000000000.0)])

    rawSamples.append([current, sample])

print("Reading raw samples... finished!")
vmmaps = []
for i in range(vmmapCount):
    try:
        (addr, size, label,) = struct.unpack_from(endianess + "QQ256s", binProfile, binOffset)
        binOffset += 256 + 16
    except Exception as e:
        print("Unexpected end of file!")
        sys.exit(1)
    vmmaps.append([addr, size, label.decode('utf-8').rstrip('\0')])

print("Reading raw vm maps... finished!")

del binProfile

vmmapString = '\n'.join([f"{x[0]:x} {x[1]:x} {x[2]}" for x in vmmaps])
print(vmmapString)
sampleParser = profileLib.sampleParser()
sampleParser.addSearchPath(args.search_path)
sampleParser.loadVMMap(fromBuffer=vmmapString)
del vmmaps
del vmmapString

profile['target'] = sampleParser.binaries[0]['binary']

i = 0
prevThreadCpuTimes = {}
for sample in rawSamples:
    if (i % 1000 == 0):
        progress = int((i + 1) * 100 / sampleCount)
        print(f"Post processing... {progress}%\r", end="")
    i += 1

    processedSample = []
    sampleCpuTime = 0
    for thread in sample[1]:
        if not thread[0] in prevThreadCpuTimes:
            prevThreadCpuTimes[thread[0]] = thread[2]

        threadCpuTime = thread[2] - prevThreadCpuTimes[thread[0]]
        prevThreadCpuTimes[thread[0]] = thread[2]
        sampleCpuTime += threadCpuTime

        processedSample.append([thread[0], threadCpuTime, sampleParser.parseFromPC(thread[1])])

    profile['profile'].append([sample[0], sampleCpuTime, processedSample])

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
