#!/usr/bin/env python3

import bz2
import re
import os
import subprocess

LABEL_UNKNOWN = '_unknown'
LABEL_FOREIGN = '_foreign'
LABEL_KERNEL = '_kernel'


def parseRange(stringRange):
    result = []
    for part in stringRange.split(','):
        if '-' in part:
            a, b = part.split('-')
            a, b = int(a), int(b)
            result.extend(range(a, b + 1))
        else:
            a = int(part)
            result.append(a)
    return result


class sampleParser:
    useDemangling = True
    binaries = []
    kallsyms = []
    binaryMap = []
    functionMap = []
    _functionMap = []
    fileMap = []
    searchPaths = []
    _fetched_pc_data = {}
    _cross_compile = ""
    _pc_heuristic = False

    def __init__(self, labelUnknown=LABEL_UNKNOWN, labelForeign=LABEL_FOREIGN, labelKernel=LABEL_KERNEL, useDemangling=True, pcHeuristic=False):
        self.binaryMap = [labelUnknown, labelForeign]
        self.functionMap = [[labelUnknown, labelUnknown], [labelForeign, labelForeign]]
        self._functionMap = [x[0] for x in self.functionMap]
        self.fileMap = [labelUnknown, labelForeign]
        self.useDemangling = useDemangling
        self.LABEL_KERNEL = labelKernel
        self.LABEL_UNKNOWN = labelUnknown
        self.LABEL_FOREIGN = labelForeign
        self.searchPaths = []
        self._fetched_pc_data = {}
        self._cross_compile = "" if 'CROSS_COMPILE' not in os.environ else os.environ['CROSS_COMPILE']

    def addSearchPath(self, path):
        if not isinstance(path, list):
            path = [path]
        for p in path:
            if not os.path.isdir(p):
                raise Exception(f"Not a directory '{path}'")
        self.searchPaths.extend(path)

    def loadVMMap(self, fromFile=False, fromBuffer=False):
        if (not fromFile and not fromBuffer):
            raise Exception("Not enough arguments")
        if (fromFile and not os.path.isfile(fromFile)):
            raise Exception(f"File '{fromFile}' not found")

        if (fromFile):
            if fromFile.endswith("bz2"):
                fromBuffer = bz2.open(fromFile, 'rt').read()
            else:
                fromBuffer = open(fromFile, "r").read()

        for line in fromBuffer.split("\n"):
            if (len(line) > 2):
                (addr, size, label,) = line.split(" ", 2)
                addr = int(addr, 16)
                size = int(size, 16)

                found = False
                static = False
                for searchPath in self.searchPaths:
                    path = f"{searchPath}/{label}"
                    if (os.path.isfile(path)):
                        readelf = subprocess.run(f"readelf -h {path}", shell=True, stdout=subprocess.PIPE)
                        try:
                            readelf.check_returncode()
                            static = True if re.search("Type:[ ]+EXEC", readelf.stdout.decode('utf-8'), re.M) else False
                            found = True
                            break
                        except Exception as e:
                            pass

                if found:
                    self.binaries.append({
                        'binary': label,
                        'path': path,
                        'kernel': False,
                        'static': static,
                        'start': addr,
                        'size': size,
                        'end': addr + size
                    })
                    if self.pcHeuristic and not static:
                        naddr = False
                        if addr >> 32 == 0x55:
                            naddr = (0x7f << 32) | (addr & 0xffffffff)
                        elif addr >> 32 == 0x7f:
                            naddr = (0x55 << 32) | (addr & 0xffffffff)
                        if naddr is not False:
                            self.binaries.append({
                                'binary': 'heuristic_' + label, 'path': path, 'kernel': False, 'static': static,
                                'start': naddr, 'size': size, 'end': naddr + size
                            })
                else:
                    raise Exception(f"Could not find {label}")

    def loadKallsyms(self, fromFile=False, fromBuffer=False):
        if (not fromFile and not fromBuffer):
            raise Exception("Not enough arguments")
        if (fromFile and not os.path.isfile(fromFile)):
            raise Exception(f"File '{fromFile}' not found")
        if (fromFile):
            if fromFile.endswith("bz2"):
                fromBuffer = bz2.open(fromFile, 'rt').read()
            else:
                fromBuffer = open(fromFile, "r").read()

        for symbol in fromBuffer.split('\n'):
            s = symbol.split(" ")
            if len(s) >= 3:
                self.kallsyms.append([int(s[0], 16), s[2]])

        if len(self.kallsyms) <= 0:
            return

        self.binaries.append({
            'binary': '_kernel',
            'path': '_kernel',
            'kernel': True,
            'static': True,
            'start': self.kallsyms[0][0],
            'size': self.kallsyms[-1][0] - self.kallsyms[0][0],
            'end': self.kallsyms[-1][0]
        })

        self.kallsyms.reverse()

    def isPCKnown(self, pc):
        if self.getBinaryFromPC(pc) is False:
            return False
        return True

    def getBinaryFromPC(self, pc):
        for binary in self.binaries:
            if (pc >= binary['start'] and pc <= binary['end']):
                return binary
        return False

    def parseFromPC(self, pc):
        if pc in self._fetched_pc_data:
            return self._fetched_pc_data[pc]

        srcpc = pc
        srcbinary = self.LABEL_FOREIGN
        srcfunction = self.LABEL_FOREIGN
        srcdemangled = self.LABEL_FOREIGN
        srcfile = self.LABEL_FOREIGN
        srcline = 0

        binary = self.getBinaryFromPC(pc)
        if binary is not False:
            srcpc = pc if binary['static'] else pc - binary['start']
            srcbinary = binary['binary']
            srcfunction = self.LABEL_UNKNOWN
            srcdemangled = self.LABEL_UNKNOWN
            srcfile = self.LABEL_UNKNOWN
            srcline = 0

            if binary['kernel']:
                for f in self.kallsyms:
                    if f[0] <= srcpc:
                        srcfunction = f[1]
                        srcdemangled = f[1]
                        break
            else:
                addr2line = subprocess.run(f"{self._cross_compile}addr2line -f -s -e {binary['path']} -a {srcpc:x}", shell=True, stdout=subprocess.PIPE)
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

                if (srcfunction != self.LABEL_UNKNOWN):
                    cppfilt = subprocess.run(f"{self._cross_compile}c++filt -i '{srcfunction}'", shell=True, stdout=subprocess.PIPE)
                    cppfilt.check_returncode()
                    srcdemangled = cppfilt.stdout.decode('utf-8').split("\n")[0]

        if srcbinary not in self.binaryMap:
            self.binaryMap.append(srcbinary)
        if srcfunction not in self._functionMap:
            self.functionMap.append([srcfunction, srcdemangled])
            self._functionMap.append(srcfunction)
        if srcfile not in self.fileMap:
            self.fileMap.append(srcfile)

        result = [
            srcpc,
            self.binaryMap.index(srcbinary),
            self._functionMap.index(srcfunction),
            self.fileMap.index(srcfile),
            srcline
        ]

        self._fetched_pc_data[pc] = result
        return result

    def parseFromSample(self, sample):
        if sample[1] not in self.binaryMap:
            self.binaryMap.append(sample[1])
        if sample[2] not in self._functionMap:
            self.functionMap.append([sample[2], sample[3]])
            self._functionMap.append(sample[2])
        if sample[4] not in self.fileMap:
            self.fileMap.append(sample[4])

        result = [
            sample[0],
            self.binaryMap.index(sample[1]),
            self._functionMap.index(sample[2]),
            self.fileMap.index(sample[4]),
            sample[5]
        ]
        return result

    def getBinaryMap(self):
        return self.binaryMap

    def getFunctionMap(self):
        return self.functionMap

    def getFunctionDemangledMap(self):
        return self.FunctionDemangledMap

    def getFileMap(self):
        return self.fileMap


class sampleFormatter():
    binaryMap = []
    functionMap = []
    fileMap = []

    def __init__(self, binaryMap, functionMap, fileMap):
        self.binaryMap = binaryMap
        self.functionMap = functionMap
        self.fileMap = fileMap

    def getSample(self, data):
        return [
            data[0],
            self.binaryMap[data[1]],
            self.functionMap[data[2]][0],
            self.functionMap[data[2]][1],
            self.fileMap[data[3]],
            data[4]
        ]

    def formatData(self, data, displayKeys=[1, 3], delimiter=":", doubleSanitizer=[LABEL_FOREIGN, LABEL_UNKNOWN, LABEL_KERNEL], lStringStrip=False, rStringStrip=False):
        return self.sanitizeOutput(
            self.formatSample(self.getSample(data), displayKeys, delimiter),
            delimiter,
            doubleSanitizer,
            lStringStrip,
            rStringStrip
        )

    def formatSample(self, sample, displayKeys=[1, 3], delimiter=":"):
        return delimiter.join([f"0x{sample[x]:x}" if x == 0 else str(sample[x]) for x in displayKeys])

    def sanitizeOutput(self, output, delimiter=":", doubleSanitizer=[LABEL_FOREIGN, LABEL_UNKNOWN, LABEL_KERNEL], lStringStrip=False, rStringStrip=False):
        if doubleSanitizer is not False:
            if not isinstance(doubleSanitizer, list):
                doubleSanitizer = [doubleSanitizer]
            for double in doubleSanitizer:
                output = output.replace(f"{double}{delimiter}{double}", f"{double}")
        if lStringStrip is not False:
            if not isinstance(lStringStrip, list):
                lStringStrip = [lStringStrip]
            for lstrip in lStringStrip:
                output = re.sub(r"^" + re.escape(lstrip), "", output).lstrip(delimiter)
        if rStringStrip is not False:
            if not isinstance(rStringStrip, list):
                rStringStrip = [rStringStrip]
            for rstrip in rStringStrip:
                output = re.sub(re.escape(rstrip) + r"$", "", output).rstrip(delimiter)
        return output
