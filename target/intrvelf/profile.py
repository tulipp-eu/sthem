#!/usr/bin/python

import argparse
import os
import sys
import struct

parser = argparse.ArgumentParser(description="Parse profiles from intrvelf sampler.")
parser.add_argument("profile", help="profile from intrvelf")
parser.add_argument("-l", "--little-endian", action="store_true", help="parse profile using little endianess");
parser.add_argument("-b", "--big-endian", action="store_true", help="parse profile using big endianess");

args = parser.parse_args();

if (not args.profile) or (not os.path.isfile(args.profile)) :
    parser.print_help()
    sys.exit(1)


binProfile = open(args.profile, mode='rb').read(); 

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
    print "Unexpected end of file!"
    sys.exit(1)
    
if (magic != 0) and (magic != 1):
    print("Invalid profile")
    sys.exit(1);

aggregated=(magic == 1)

try:
    (wallTime, cpuTime, sampleCount) = struct.unpack_from(endianess + 'QQQ', binProfile, binOffset);
    binOffset += 24
except:
    print "Unexpected end of file!"
    sys.exit(1)


if (sampleCount == 0):
    print "No samples found in profile!"
    sys.exit(1)

timePerSample = wallTime/sampleCount
latencyPerSample = cpuTime/sampleCount
frequency = 1000000 / (wallTime/sampleCount)


print "Profile Type    : {:>14}".format("Aggregated" if aggregated else "Full")
print "Samples         : {:>14}".format(sampleCount)
print "Frequency       : {:>14} Hz".format(frequency)
print "Total Time      : {:>14} us (wall), {:>14} us (latency)".format(wallTime, cpuTime)
print "Time per Sample : {:>14} us (wall), {:>14} us (latency)".format(timePerSample, latencyPerSample)

try:
    (vmmapsCount,) = struct.unpack_from(endianess + "I", binProfile, binOffset);
    binOffset += 4
except:
    print "Unexpected end of file!"
    sys.exit(1)

vmmaps = []
for i in range(vmmapsCount):
    try:
        (addr, size, label,) = struct.unpack_from(endianess + "QQ256s", binProfile, binOffset)
        binOffset += 256 + 16
    except:
        print "Unexpected end of file!"
        sys.exit(1)
    vmmaps.append([addr, size, label])

for x in vmmaps:
    print "Addr {}, Size {}, Label {}".format(x[0],x[1],x[2]);

samples = []
if (not aggregated):
    for i in range(sampleCount):
        try:
            (current, threadCount, ) = struct.unpack_from(endianess + "QI", binProfile, binOffset)
            binOffset += 8 + 4
        except:
            print "Unexpected end of file!"
            sys.exit(1)
        samples.append([current, []])
        for j in range(threadCount):
            try:
                (tid, pc, ) = struct.unpack_from(endianess + "IQ", binProfile, binOffset)
                binOffset += 4 + 8
            except:
                print "Unexpected end of file!"
                sys.exit(1)
            samples[i][1].append([tid, pc])
else:
    try:
        (unknownSamples, unknownCurrent, ) = struct.unpack_from(endianess + "QQ", binProfile, binOffset)
        binOffset += 8 + 8
    except:
        print "Unexpected end of file!"
        sys.exit(1)
    samples.append([unknownSamples, unknownCurrent]);
    for i in range(vmmapsCount):
        try:
            (addr, ) = struct.unpack_from(endianess + "Q", binProfile, binOffset)
            binOffset += 8
        except:
            print "Unexpected end of file!"
            sys.exit(1)
        if (not addr == vmmaps[i][0]):
            print "Unexpected address found!"
            sys.exit(1)
        for j in range(vmmaps[i][1]):
            try:
                (addrSamples, addrCurrent, ) = struct.unpack_from(endianess + "QQ", binProfile, binOffset)
                binOffset += 8 + 8
            except:
                print "Unexpected end of file!"
                sys.exit(1)
            if (addrSamples > 0):
                samples.append([addr  + j, addrSamples, addrCurrent]);
        
        
