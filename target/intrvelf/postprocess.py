#!/usr/bin/python3

import argparse
import os
import sys
import struct
import subprocess
import numpy
import pickle
import bz2

cross_compile = "" if not 'CROSS_COMPILE' in os.environ else os.environ['CROSS_COMPILE']

vmmap = []


def getVmaOffset(pc):
    if (pc >= vmmap[1]) and (pc <= vmmap[2]):
            return vmmap[1];
    return 0

_fetched_pc_data = {};

def getPcData(pc, target):
    if pc in _fetched_pc_data:
        return _fetched_pc_data[pc]
    addr2line = subprocess.run(f"{cross_compile}addr2line -C -f -s -e {target} -a {pc:x}", shell=True, stdout=subprocess.PIPE)
    addr2line.check_returncode()
    result = addr2line.stdout.decode('utf-8').split("\n");
    fileAndLine = result[2].split(':')
    result = numpy.char.replace(numpy.array([ result[1], fileAndLine[0], fileAndLine[1].split(' ')[0].replace('?','0') ]), '??', '_unknown');
    _fetched_pc_data[pc] = result;
    return result;
   

parser = argparse.ArgumentParser(description="Parse profiles from intrvelf sampler.")
parser.add_argument("profile", help="profile from intrvelf")
parser.add_argument("-o", "--output", help="write postprocess profile");
parser.add_argument("-z", "--bzip2", action="store_true", help="bz2 zip postprocessed profile");
parser.add_argument("-l", "--little-endian", action="store_true", help="parse profile using little endianess");
parser.add_argument("-b", "--big-endian", action="store_true", help="parse profile using big endianess");

args = parser.parse_args();

if (not args.output):
    parser.print_help()
    sys.exit(1)

if (not args.profile) or (not os.path.isfile(args.profile)) :
    parser.print_help()
    sys.exit(1)


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

profile = {
    'samples' : sampleCount,
    'wallTime' : wallTime,
    'latencyTime' : cpuTime,
    'timePerSample' : wallTime/sampleCount,
    'latencyPerSample' : cpuTime/sampleCount,
    'samplingFrequency' : 1000000 / (wallTime / sampleCount)
}

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
    vmmaps.append([ label.decode('utf-8').rstrip('\0'), addr, addr+size, size ])

if (vmmapsCount == 0):
    print("No VMMaps found!")
    sys.exit(1)
    
elf = vmmaps[0][0]

# Save first vmmap, we can only cover one within an elf file
vmmap = vmmaps[0]    
profile['vmmap'] = vmmap;
    

if not os.path.isfile(elf):
    try:
        whereis = subprocess.run(f"which {elf}", shell=True, stdout=subprocess.PIPE)
        whereis.check_returncode()
    except subprocess.CalledProcessError as e:
        sys.exit(1)
    elf = whereis.stdout.decode('utf-8')
        

profile['elf'] = elf;
 
aggregatedSamples = {}

if (not aggregated):
    fullSamples = []
    for i in range(sampleCount):
        if (i % 100 == 0):
            progress = int((i+1) * 100 / sampleCount)
            print(f"Processing... {progress}%\r", end="")
        try:
            (current, threadCount, ) = struct.unpack_from(endianess + "QI", binProfile, binOffset)
            binOffset += 8 + 4
        except:
            print("Unexpected end of file!")
            sys.exit(1)
        threads = []
        for j in range(threadCount):
            try:
                (tid, pc, ) = struct.unpack_from(endianess + "IQ", binProfile, binOffset)
                binOffset += 4 + 8
            except:
                print("Unexpected end of file!")
                sys.exit(1)

            pc = pc - getVmaOffset(pc)
            pcInfo = getPcData(pc,elf)
            thread = [tid, pc]
            thread.extend(pcInfo)
            threads.append(thread);
                                   
            if pc in aggregatedSamples:
                aggregatedSamples[pc]['samples'] += 1;
                aggregatedSamples[pc]['current'] += current
            else:
                aggregatedSamples[pc] =  { 'samples' : 1, 'current' : current, 'info' : pcInfo };

        fullSamples.append([ current , threads ])

    profile['fullProfile'] = fullSamples
    print("Processing... finished!")

else:
    try:
        (unknownSamples, unknownCurrent, ) = struct.unpack_from(endianess + "QQ", binProfile, binOffset)
        binOffset += 8 + 8
    except:
        print("Unexpected end of file!")
        sys.exit(1)
    aggregatedSamples['_unknown'] = [unknownSamples, unknownCurrent, [ 0, '_unknown', '_unknown', 0]];
    
    try:
        (addr, ) = struct.unpack_from(endianess + "Q", binProfile, binOffset)
        binOffset += 8
    except:
        print("Unexpected end of file!")
        sys.exit(1)
        
    if (not addr == vmmap[1]):
        print("Unexpected address found!")
        sys.exit(1)
        
    for offset in range(vmmap[3]):
        if (offset % 100 == 0): 
            progress = int((offset+1) * 100 / vmmap[3])
            print(f"Processing... {progress}%\r", end="")
        try:
            (samples, current, ) = struct.unpack_from(endianess + "QQ", binProfile, binOffset)
            binOffset += 8 + 8
        except:
            print("Unexpected end of file!")
            sys.exit(1)
            
        if (samples > 0):
            aggregatedSamples[offset] = [samples, current, getPcData(offset, elf)];

    print("Processing... finished!")
    
profile['aggregatedProfile'] = aggregatedSamples

print(f"Writing {args.output}... ", end="")
sys.stdout.flush();
pickle.dump(profile, outProfile, pickle.HIGHEST_PROTOCOL);
print("finished")
