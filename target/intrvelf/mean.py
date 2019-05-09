#!/usr/bin/python3

import sys
import argparse
import bz2
import pickle

parser = argparse.ArgumentParser(description="Average multiple profiles from intrvelf sampler.")
parser.add_argument("profiles", help="postprocessed profiles from intrvelf", nargs="+")
parser.add_argument("-o", "--output", help="output mean aggregate only profile")
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile")


args = parser.parse_args()

if (not args.profiles) or (len(args.profiles) <= 1):
    print("ERROR: unsufficient amount of profiles passed")
    parser.print_help()
    sys.exit(1)

if (not args.output):
    print("ERROR: not output file defined!")
    parser.print_help()
    sys.exit(1)

if (args.bzip2):
    if not args.output.endswith('.bz2'):
        args.output += ".bz2"
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")


profiles = []

for profile in args.profiles:
    if profile.endswith(".bz2"):
        new = pickle.load(bz2.BZ2File(profile, mode="rb"))
    else:
        new = pickle.load(open(profile, mode="rb"))
    del new['fullProfile']
    profiles.append(new)

label_unknown = profiles[0]['binaries'][0]
label_foreign = profiles[0]['binaries'][1]
aggregateKeys = [1]

meanprofile = {
    'samples': 0,
    'samplingTimeUs': 0,
    'latencyTimeUs': 0,
    'volts': profiles[0]['volts'],
    'target': profiles[0]['target'],
    'binaries': [label_unknown, label_foreign],
    'functions': [label_unknown, label_foreign],
    'functions_mangled': [label_unknown, label_foreign],
    'files': [label_unknown, label_foreign],
    'fullProfile': [],
    'aggregatedProfile': {},
    'mean': 0
}

for profile in profiles:
    if 'mean' not in profile:
        profile['mean'] = 1
    meanprofile['mean'] += profile['mean']

print(f"Calculating mean of {len(profiles)} profiles...")

i = 0
for profile in profiles:
    progress = int((i + 1) * 100 / len(profiles))
    meanfac = profile['mean'] / meanprofile['mean']

    print(f"Processing profile {i} ({progress}%), contribution {meanfac}")
    sys.stdout.flush()
    i += 1

    if (profile['volts'] != meanprofile['volts']):
        print("ERROR: profile voltages do not match!")
        sys.exit(1)

    meanprofile['samples'] += meanfac * profile['samples']
    meanprofile['samplingTimeUs'] += meanfac * profile['samplingTimeUs']
    meanprofile['latencyTimeUs'] += meanfac * profile['latencyTimeUs']
    for aggregateIndex in profile['aggregatedProfile']:
        sample = profile['aggregatedProfile'][aggregateIndex]
        samples = sample[0] * meanfac
        current = sample[1] * meanfac
        srcbinary = profile['binaries'][sample[2]]
        srcfunction = profile['functions_mangled'][sample[3]]
        srcdemangled = profile['functions'][sample[3]]
        srcfile = profile['files'][sample[4]]
        srcline = sample[5]

        if srcbinary not in meanprofile['binaries']:
            meanprofile['binaries'].append(srcbinary)
        if srcfunction not in meanprofile['functions_mangled']:
            meanprofile['functions_mangled'].append(srcfunction)
            meanprofile['functions'].append(srcdemangled)
        if srcfile not in meanprofile['files']:
            meanprofile['files'].append(srcfile)

        pcInfo = [
            meanprofile['binaries'].index(srcbinary),
            meanprofile['functions_mangled'].index(srcfunction),
            meanprofile['files'].index(srcfile),
            srcline
        ]

        newAggregateIndex = ':'.join([str(pcInfo[i]) for i in aggregateKeys])

        if newAggregateIndex in meanprofile['aggregatedProfile']:
            meanprofile['aggregatedProfile'][newAggregateIndex][0] += samples
            meanprofile['aggregatedProfile'][newAggregateIndex][1] += current
        else:
            asample = [samples, current]
            asample.extend(pcInfo)
            meanprofile['aggregatedProfile'][newAggregateIndex] = asample

print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(meanprofile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")

sys.exit(0)
