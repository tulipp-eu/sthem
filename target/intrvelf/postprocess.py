#!/usr/bin/python3

import argparse
import os
import sys
import struct
import subprocess
import numpy
import pickle
import bz2
import re

cross_compile = "" if not 'CROSS_COMPILE' in os.environ else os.environ['CROSS_COMPILE']

if cross_compile != "":
    print(f"Using cross compilation prefix '{cross_compile}'")

label_unknown = '_unknown'

profile={
    'samples' : 0,
    'samplingTimeUs' : 0,
    'latencyTimeUs' : 0,
    'volts' : 0,
    'elf' : label_unknown ,
    'functions' : [ label_unknown ],
    'files' : [ label_unknown ],
    'fullProfile' : [],
    'aggregatedProfile' : { label_unknown : [0,0,0,0,0]}
}

_fetched_pc_data = {};

def fetchPcData(pc, target):
    if pc in _fetched_pc_data:
        return _fetched_pc_data[pc]
    addr2line = subprocess.run(f"{cross_compile}addr2line -C -f -s -e {profile['elf']} -a {pc:x}", shell=True, stdout=subprocess.PIPE)
    addr2line.check_returncode()
    result = addr2line.stdout.decode('utf-8').split("\n");
    srcfunction = result[1].replace('??', label_unknown )
    srcfile = result[2].split(':')[0].replace('??', label_unknown )
    srcline = int(result[2].split(':')[1].split(' ')[0].replace('?','0'))
    if not srcfunction in profile['functions']:
        profile['functions'].append(srcfunction)
    if not srcfile in profile['files']:
        profile['files'].append(srcfile)

    result = [ profile['functions'].index(srcfunction), profile['files'].index(srcfile), srcline ]
    _fetched_pc_data[pc] = result;
    return result;
   
def pcVmaOffset(pc, vmmap):
    if (pc >= vmmap['start']) and (pc <= vmmap['end']):
            return vmmap['start'];
    return 0


parser = argparse.ArgumentParser(description="Parse profiles from intrvelf sampler.")
parser.add_argument("profile", help="profile from intrvelf")
parser.add_argument("-v", "--volts", help="set pmu voltage")
parser.add_argument("-e", "--elf", help="overwrite target elf");
parser.add_argument("-o", "--output", help="write postprocessed profile");
parser.add_argument("-z", "--bzip2", action="store_true", help="compress postprocessed profile");
parser.add_argument("-l", "--little-endian", action="store_true", help="parse profile using little endianess");
parser.add_argument("-b", "--big-endian", action="store_true", help="parse profile using big endianess");

args = parser.parse_args();

if (not args.output):
    print("ERROR: not output file defined!")
    parser.print_help()
    sys.exit(1)

if (not args.profile) or (not os.path.isfile(args.profile)) :
    print("ERROR: profile not found!")
    parser.print_help()
    sys.exit(1)

if (args.volts):
    profile['volts'] = float(args.volts)
    

binProfile = open(args.profile, mode='rb').read()

if (args.bzip2):
    args.output += ".bz2"
    outProfile = bz2.BZ2File(args.output, mode='wb')
else:
    outProfile = open(args.output, mode="wb")

forced=False
endianess="="
    
if (args.little_endian):
    endianess="<"
    forced=True

if (args.big_endian):
    endianess=">"
    forced=True

binOffset = 0
    
try:
    (magic,) = struct.unpack_from(endianess + "I", binProfile, binOffset);
    binOffset += 4
except:
    print("Unexpected end of file!")
    sys.exit(1)
    
if (magic != 0) and (magic != 1):
    print("Invalid profile")
    sys.exit(1);

aggregated=(magic == 1)

try:
    (wallTime, cpuTime, sampleCount) = struct.unpack_from(endianess + 'QQQ', binProfile, binOffset);
    binOffset += 24
except:
    print("Unexpected end of file!")
    sys.exit(1)


if (sampleCount == 0):
    print("No samples found in profile!")
    sys.exit(1)

profile['samples'] = sampleCount
profile['samplingTimeUs'] = wallTime
profile['latencyTimeUs'] = cpuTime

try:
    (vmmapsCount,) = struct.unpack_from(endianess + "I", binProfile, binOffset);
    binOffset += 4
except:
    print("Unexpected end of file!")
    sys.exit(1)

vmmaps = []
for i in range(vmmapsCount):
    try:
        (addr, size, label,) = struct.unpack_from(endianess + "QQ256s", binProfile, binOffset)
        binOffset += 256 + 16
    except:
        print("Unexpected end of file!")
        sys.exit(1)
    vmmaps.append({ 'label' : label.decode('utf-8').rstrip('\0'), 'start' : addr, 'end' : addr+size, 'size' : size })

if (len(vmmaps) == 0):
    print("No VMMaps found!")
    sys.exit(1)

if (not args.elf):
    elf = vmmaps[0]['label']
else:
    elf = args.elf

# Save first vmmap, we can only cover one within an elf file
vmmap = vmmaps[0]    

if not os.path.isfile(elf):
    whereis = subprocess.run(f"which {elf}", shell=True, stdout=subprocess.PIPE)
    whereis.check_returncode()
    elf = whereis.stdout.decode('utf-8').rstrip('\n');
        

readelf = subprocess.run(f"readelf -h {elf}", shell=True, stdout=subprocess.PIPE)
readelf.check_returncode()
static = True if re.search("Type:[ ]+EXEC",readelf.stdout.decode('utf-8'), re.M) else False

if static:
    print("Static binary detected")
else:
    print("Dynamic binary detected")
    
profile['elf'] = elf;

aggregatedSamples = {}

if (not aggregated):
    for i in range(sampleCount):
        if (i % 100 == 0):
            progress = int((i+1) * 100 / sampleCount)
            print(f"Processing... {progress}%\r", end="")

        try:
            (current, threadCount, ) = struct.unpack_from(endianess + "dI", binProfile, binOffset)
            binOffset += 8 + 4
        except:
            print("Unexpected end of file!")
            sys.exit(1)
            
        sample = []
        for j in range(threadCount):
            try:
                (tid, pc, ) = struct.unpack_from(endianess + "IQ", binProfile, binOffset)
                binOffset += 4 + 8
            except:
                print("Unexpected end of file!")
                sys.exit(1)

            if not static:
                pc = pc - pcVmaOffset(pc,vmmap)
            thread = [tid]
            pcInfo = fetchPcData(pc, profile['elf'])
            thread.extend(pcInfo)
            sample.append(thread);
                                   
            if pc in profile['aggregatedProfile']:
                profile['aggregatedProfile'][pc][0] += 1;
                profile['aggregatedProfile'][pc][1] += current
            else:
                asample = [ 1, current ]
                asample.extend(pcInfo)
                profile['aggregatedProfile'][pc] =  asample;

        profile['fullProfile'].append([ current , sample ])

    print("Processing... finished!")

else:
    try:
        (unknownSamples, unknownCurrent, ) = struct.unpack_from(endianess + "Qd", binProfile, binOffset)
        binOffset += 8 + 8
    except:
        print("Unexpected end of file!")
        sys.exit(1)
    profile['aggregatedProfile'][label_unknown] = [unknownSamples, unknownCurrent, 0, 0, 0];
    
    try:
        (addr, ) = struct.unpack_from(endianess + "Q", binProfile, binOffset)
        binOffset += 8
    except:
        print("Unexpected end of file!")
        sys.exit(1)
        
    if (not addr == vmmap['start']):
        print("Unexpected address found!")
        sys.exit(1)
        
    for offset in range(vmmap['size']):
        if (offset % 100 == 0): 
            progress = int((offset+1) * 100 / vmmap['size'])
            print(f"Processing... {progress}%\r", end="")
        try:
            (samples, current, ) = struct.unpack_from(endianess + "Qd", binProfile, binOffset)
            binOffset += 8 + 8
        except:
            print("Unexpected end of file!")
            sys.exit(1)
            
        if (samples > 0):
            sample = [samples,current]
            sample.extend(fetchPcData(offset if not static else offset + addr, profile['elf']))
            profile['aggregatedProfile'][offset] = sample;

    print("Processing... finished!")
    
print(f"Writing {args.output}... ", end="")
sys.stdout.flush()
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL)
outProfile.close()
print("finished")
