# Intrusive ELF Profiler

```text
intrvelf [options] -- <command> [arguments]

Options:
  -o, --output=<file>       write to file
  -s, --sensor=<1-7>        lynsyn sensor
  -f, --frequency=<hertz>   sampling frequency
  -a, --aggregate           write aggregated profile
  -d, --debug               output debug messages
  -h, --help                shows help
```

* Support for full profiles
  * profiles contain current, TID and PC for every sample and thread
  * very IO heavy depending on sampling frequency and execution time
  * could lead to very big profiles
* Support for aggregated profiles
  * every known pc value has an sample counter and aggregated current value
  * unknown pc values are accumulated to a separate counter and current value
  * no IO is generated during sampling
* all profiles contain execution time (wall), time spend in profiler (latency), number of samples and target VMMap
* output file is optional, if not specified no IO is generated
* sensor is optional, if not specified PMU sampling is omitted
* frequency is a target, actual reached frequency is displayed in debug output
* threads are fully supported, **though fork, vfork and vclone is not supported**!
* execl, execlp, execle, execv, execvp and execvpe are replacing the current process with new VMMaps and are therefore not supported!

## Profile Header

* Included in every profile
* Magic number defines profile type
  * 0 - full profile
  * 1 - aggregated profile

```
[ 4 bytes / uint32_t ] Magic Number
[ 8 bytes / uint64_t ] Wall Time
[ 8 bytes / uint64_t ] CPU Time (latency)
[ 8 bytes / uint64_t ] Number of Samples

[ 4 bytes / uint32_t ] Number of VMMaps
VMMap{
    [ 8 bytes / uint64_t ] Address
    [ 8 bytes / uint64_t ] Size
    [ 256 bytes / char * ] Label
} // Repeated "Number of VMMaps" times
```

### Full Profile

* big and IO heavy
* is written on every sample, does not add any IO buffer (apart from system file buffers)
* frequency, number of threads and sampling time determines size

```
Sample{
    [ 8 bytes / double   ] Current
    [ 4 bytes / uint32_t ] Number of Threads
    Thread{
        [ 4 bytes / uint32_t ] Thread ID
        [ 8 bytes / uint64_t ] PC
    } // Repeated "Number of Threads" times
} // Repeated "Number of Samples" times
```

### Aggregated Profile

* no IO during sampling process, profile is written afterwards
* text section determines size
* no informations about individual threads, only which PC caused which average current

```
[ 8 bytes / uint64_t ] Unknown Samples
[ 8 bytes / double   ] Unknwon Current
VMMap{
    [ 8 Bytes / uint64_t ] Address
    Offset{
        [ 8 bytes / uint64_t ] Number of Samples
        [ 8 bytes / double   ] Aggregated Current
    } // Repeated "VMMap->Size" times
} // Repeated "Number of VMMaps" times
```
