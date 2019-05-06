#!/usr/bin/python3

import argparse
import os
import sys
import struct
import subprocess
import pickle
import bz2
import re

cross_compile = "" if 'CROSS_COMPILE' not in os.environ else os.environ['CROSS_COMPILE']

if cross_compile != "":
    print(f"Using cross compilation prefix '{cross_compile}'")

label_unknown = '_unknown'

profile = {
    'samples': 0,
    'samplingTimeUs': 0,
    'latencyTimeUs': 0,
    'volts': 0,
    'target': label_unknown,
    'binaries': [label_unknown],
    'functions': [label_unknown],
    'functions_mangled': [label_unknown],
    'files': [label_unknown],
    'fullProfile': [],
    'aggregatedProfile': {label_unknown: [0, 0, 0, 0, 0, 0]},
    'mean': 1
}

_fetched_pc_data = {}

_unknown_pcs = []

binaryMap = []


def fetchPCInfo(pc):
    if pc in _fetched_pc_data:
        if _fetched_pc_data[pc][1] == 0:
            _unknown_pcs.append(pc)
        return _fetched_pc_data[pc]
    found = False
    lookupPc = pc
    elf = ""
    result = [0, 0, 0, 0]
    for binary in binaryMap:
        if (pc >= binary['start'] and pc <= binary['end']):
            found = True
            elf = binary['binary']
            if not binary['static']:
                lookupPc -= binary['start']
            break

    if found:
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

    if (result[1] == 0):
        _unknown_pcs.append(pc)

    _fetched_pc_data[pc] = result
    return result


parser = argparse.ArgumentParser(description="Parse profiles from intrvelf sampler.")
parser.add_argument("profile", help="profile from intrvelf")
parser.add_argument("-v", "--volts", help="set pmu voltage")
parser.add_argument("-s", "--search-path", help="add search path", action="append")
parser.add_argument("-o", "--output", help="write postprocessed profile")
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile")
parser.add_argument("-l", "--little-endian", action="store_true", help="parse profile using little endianess")
parser.add_argument("-b", "--big-endian", action="store_true", help="parse profile using big endianess")
parser.add_argument("-u", "--unknown", action="store_true", help="dump unknown addresses after postprocessing")

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

if (args.bzip2):
    if not args.output.endswith(".bz2"):
        args.output += ".bz2"
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")

forced = False
endianess = "="

if (args.little_endian):
    endianess = "<"
    forced = True

if (args.big_endian):
    endianess = ">"
    forced = True

binOffset = 0

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
    (wallTime, cpuTime, sampleCount, vmmapCount) = struct.unpack_from(endianess + 'QQQI', binProfile, binOffset)
    binOffset += 28
except Exception as e:
    print("Unexpected end of file!")
    sys.exit(1)


if (sampleCount == 0):
    print("No samples found in profile!")
    sys.exit(1)

profile['samples'] = sampleCount
profile['samplingTimeUs'] = wallTime
profile['latencyTimeUs'] = cpuTime

aggregatedSamples = {}

rawSamples = []

for i in range(sampleCount):
    if (i % 100 == 0):
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
            (tid, pc, cputime, ) = struct.unpack_from(endianess + "IQQ", binProfile, binOffset)
            binOffset += 4 + 8 + 8
        except Exception as e:
            print("Unexpected end of file!")
            sys.exit(1)
        sample.append([tid, pc, cputime])

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
prevCpuTimes = {}
for sample in rawSamples:
    if (i % 100 == 0):
        progress = int((i + 1) * 100 / sampleCount)
        print(f"Post processing... {progress}%\r", end="")
    i += 1

    processedSample = []
    sampleCpuTime = 0
    threadCpuTimes = {}

    for thread in sample[1]:
        if not thread[0] in prevCpuTimes:
            prevCpuTimes[thread[0]] = thread[2]
        threadCpuTimes[thread[0]] = thread[2] - prevCpuTimes[thread[0]]
        prevCpuTimes[thread[0]] = thread[2]
        sampleCpuTime += threadCpuTimes[thread[0]]

    for thread in sample[1]:
        if (sampleCpuTime == 0):
            cpuShare = (1 / len(sample[1]))
        else:
            cpuShare = threadCpuTimes[thread[0]] / sampleCpuTime

        threadSample = [thread[0], cpuShare]
        pcInfo = fetchPCInfo(thread[1])
        threadSample.extend(pcInfo)

        processedSample.append(threadSample)

        if thread[1] in profile['aggregatedProfile']:
            profile['aggregatedProfile'][thread[1]][0] += 1  # /len(sample[1]);
            profile['aggregatedProfile'][thread[1]][1] += (sample[0] * cpuShare)
        else:
            asample = [1, (sample[0] * cpuShare)]  # /len(sample[1])
            asample.extend(pcInfo)
            profile['aggregatedProfile'][thread[1]] = asample

    profile['fullProfile'].append([sample[0], processedSample])

print("Post processing... finished!")

print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")

if len(_unknown_pcs) > 0 and args.unknown:
    _unknown_unique_pcs = list(set(_unknown_pcs))
    print(f'{len(_unknown_unique_pcs)} ({(len(_unknown_unique_pcs) * 100 / len(_fetched_pc_data)):.2f} %) unknown adresses found:')
    for pc in _unknown_unique_pcs:
        print(f"0x{pc:x} in file {profile['binaries'][_fetched_pc_data[pc][0]]}, {_unknown_pcs.count(pc)} ({_unknown_pcs.count(pc) * 100 / len(_unknown_pcs):.2f}%)")
