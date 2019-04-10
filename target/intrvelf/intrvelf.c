#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <spawn.h>
#include <getopt.h>

#include <elf.h>
#include <sys/uio.h>
#include <sys/ptrace.h>


#include "lynsyn.h"
#include "vmmap.h"
#include "timeHelper.h"

// Switch between a fixed frequency timer, which blocks and a
// single shot timer, which is engaged after every
// cycle taking latency into account. The first one has low overhead
// but inconsitent actual sampling frequencies, the last one is more
// consistent but adds overhead due to time calculations and engaging
// the timer every sample
#define ADAPTIVE_FREQUENCY

//Estimating the latency accounts cpu time in the sampler as latency
//bascially ignoring any ptrace overhead, or all kernel calls at all
//#define ESTIMATE_LATENCY

#ifdef DEBUG
#define debug_printf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#else
#define debug_printf(fmt,...)
#endif

#if !defined(__amd64__) && !defined(__aarch64__)
#error "Architecture not supported!"
#endif

static int _ptrace_return = 0;

#define PTRACE_WAIT(target, status) do { \
    do { \
        target = waitpid(-1, &status, __WALL);   \
    } while(target == -1 && errno == EAGAIN)

#define PTRACE_CONTINUE(target, signal) do { \
        _ptrace_return = ptrace(PTRACE_CONT, target, NULL, signal); \
    } while (_ptrace_return == -1L && (errno == EBUSY || errno == EFAULT || errno == ESRCH)) 


uint32_t taskCount = 0;
uint32_t allocTasks = 0;

struct task {
    uint32_t tid;
    uint64_t pc;
} __attribute__((packed));

struct aggregatedSample {
    uint64_t samples;
    double current;
} __attribute__((packed));

struct task *tasks = NULL;

int reallocTaskList() {
    if (tasks == NULL) {
        allocTasks = 1;
        tasks = (struct task *) malloc(sizeof(pid_t));
    } else {
        allocTasks *= 2;
        tasks = (struct task *) realloc(tasks, allocTasks * 2 * sizeof(struct task));
    }

    if (tasks == NULL) {
        allocTasks = 0;
        return 1;
    }
    return 0;
}

int addTask(pid_t const task) {
    if (taskCount == allocTasks && reallocTaskList() != 0) {
        return 1;
    }
    tasks[taskCount].tid = task;
    tasks[taskCount].pc = 0;
    taskCount++;
    return 0;
}

int removeTask(pid_t const task) {
    for (unsigned int i = 0; i < taskCount; i++) {
        if (tasks[i].tid == task) {
            taskCount--;
            for (unsigned int j = i; j < taskCount; j++) {
                tasks[j].tid = tasks[j+1].tid;
            }
            return 0;
        }
    }
    return 1;
}

int taskExists(pid_t const task) {
    for (unsigned int i = 0; i < taskCount; i++) {
        if (tasks[i].tid == task)
            return 1;
    }
    return 0;
}

void help(char const opt, char const *optarg) {
    FILE *out = stdout;
    int debug = -1;
    if (opt != 0) {
        out = stderr;
        if (optarg) {
            fprintf(out, "Invalid parameter - %c %s\n", opt, optarg);
        } else {
            fprintf(out, "Invalid parameter - %c\n", opt);
        }
    }
    fprintf(out, "intrvelf [options] -- <command> [arguments]\n");
    fprintf(out, "\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -o, --output=<file>       write to file\n");
    fprintf(out, "  -s, --sensor=<1-%d>        lynsyn sensor\n", LYNSYN_SENSORS);
    fprintf(out, "  -f, --frequency=<hertz>   sampling frequency\n");
    fprintf(out, "  -a, --aggregate           write aggregated profile\n");
    fprintf(out, "  -d, --debug               output debug messages\n");
    fprintf(out, "  -h, --help                shows help\n");
    fprintf(out, "\n");
    fprintf(out, "Example: intrvelf -o /tmp/map -f 10000 -o -- stress-ng --cpu 4\n");
}


struct timerData {
    int               active; 
    timer_t           timer;
    struct itimerspec time;
    struct timespec   samplingInterval;
    struct sigaction  signalOldAction;
    struct sigaction  signalAction;
};

struct callbackData {
    pid_t tid;
#ifdef ADAPTIVE_FREQUENCY
    struct timespec lastInterrupt;
#else
    int   block;
#endif
};

static struct callbackData _callback_data;

#define TRACEE_INTERRUPT_SIGNAL SIGUSR2

void timerCallback(int sig) {
#ifndef ADAPTIVE_FREQUENCY
    if (_callback_data.block == 0) {
        _callback_data.block = 1;
#endif
        int r;
        do {
            r = kill(_callback_data.tid, TRACEE_INTERRUPT_SIGNAL);
        } while (r == -1 && errno == EAGAIN);
        debug_printf("[%d] send %d\n", _callback_data.tid, TRACEE_INTERRUPT_SIGNAL);
#ifndef ADAPTIVE_FREQUENCY
    }
#else
    clock_gettime(CLOCK_REALTIME, &_callback_data.lastInterrupt);
#endif
}

#ifdef ADAPTIVE_FREQUENCY
int shootTimerIn(struct timerData *timer, struct timespec *time) {
    if (timer->active == 0) 
        return 0;
    
    if (time->tv_sec < 0 || (time->tv_nsec <= 0 && time->tv_sec <= 0)) {
        // Minimal time, to ensure it fires
        timer->time.it_value.tv_sec = 0;
        timer->time.it_value.tv_nsec = 1;
    } else {
        // Set next time
        timer->time.it_value = *time;
    }
    if (timer_settime(timer->timer, 0, &timer->time, NULL) != 0)
        return 1;
    return 0;
}
#endif

int startTimer(struct timerData *timer) {
    //If sampling interval is 0, no need for a timer
    if (timer->samplingInterval.tv_sec == 0 && timer->samplingInterval.tv_nsec == 0)
        return 0;
    
    //If already active, abort
    if (timer->active)
        goto start_error;

    //Setup SIGNAL action
    if (sigfillset(&timer->signalAction.sa_mask) != 0)
        goto start_error;
    timer->signalAction.sa_flags = SA_RESTART;
    timer->signalAction.sa_handler = &timerCallback;
    
    if (sigaction(SIGALRM, &timer->signalAction, &timer->signalOldAction) != 0)
        goto start_error;
    if (timer_create(CLOCK_REALTIME, NULL, &timer->timer) != 0)
        goto start_error;
    
#ifndef ADAPTIVE_FREQUENCY
    timer->time.it_value.tv_sec = 0;
    if (freq == 0.0) {
        timer->time.it_value.tv_nsec = 0;
    } else {
        timer->time.it_value.tv_nsec = 1;
    }
    timer->time.it_interval = timer->samplingInterval;
        
    if (timer_settime(timer->timer, 0, &timer->time, NULL) != 0)
        goto start_error;
#endif
    
    timer->active = 1;
    return 0;
start_error:
    return -1;
    
}

int stopTimer(struct timerData *timer) {
    if (timer->active == 0)
        return 0;
    if (timer_delete(timer->timer) != 0)
        goto stop_error;
    if (sigaction(SIGALRM, &timer->signalOldAction, NULL) != 0)
        goto stop_error;

    timer->active = 0;
    return 0;
 stop_error:
    return -1;
}

double getCurrentFromLynsyn(int const sensor) {
    if (sensor < 0 || sensor >= LYNSYN_SENSORS)
        return 0.0;
    return adjustCurrent(retrieveCurrents()[sensor], sensor);
}


int main(int const argc, char **argv) {
    FILE *output = NULL;
    char **argsStart = NULL;
    int sensor = -1;
    bool debugOutput = 0;
    bool aggregate = 0;
    double samplingFrequency = 10000;
    
    static struct option const long_options[] =  {
        {"help",         no_argument, 0, 'h'},
        {"debug",        no_argument, 0, 'd'},
        {"aggregate",    no_argument, 0, 'a'},
        {"sensor",       required_argument, 0, 's'},
        {"frequency",    required_argument, 0, 'f'},
        {"output",       required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    static char const * short_options = "hdaf:o:s:";

    while (1) {
        char *endptr;
        int c;
        int option_index = 0;
        size_t len = 0;
        
        c = getopt_long (argc, argv, short_options, long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;
            case 'h':
                help(0, NULL);
                return 0;
            case 's':
                sensor = strtol(optarg, &endptr, 10);
                if (endptr == optarg || sensor > 7 || sensor < 1) {
                    help(c, optarg);
                    return 1;
                }
                sensor--;
                break;
            case 'f':
                samplingFrequency = strtod(optarg, &endptr);
                if (endptr == optarg) {
                    help(c, optarg);
                    return 1;
                }

                break;
            case 'o':
                len = strlen(optarg);
                if (strlen(optarg) == 0) {
                    help(c ,optarg);
                    return 1;
                }
                output = fopen(optarg, "w+");
                if (output == NULL) {
                    help(c, optarg);
                    return 1;
                }
                break;
            case 'a':
                aggregate = true;
                break;
            case 'd':
                debugOutput = true;
                break;
            default:
                abort();
        }
    }


    for (unsigned int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0 && (i + 1) < argc) {
            argsStart = &argv[i + 1];
        }
    }

    if (argsStart == NULL) {
        help(' ', "no command specified");
        return 1;
    }
       
    if (sensor >= 0 && !initLynsyn()) {
        goto lynsynError;
    }

    
    int ret = 0; // this application return code
    long rp = 0; // ptrace return code

    pid_t samplingTarget = 0;
    pid_t intrTarget = 0;
    int intrStatus = 0;
    int intrSignal = 0;
        
    do {
        samplingTarget = fork();
    } while (samplingTarget == -1 && errno == EAGAIN);

    if (samplingTarget == -1) {
        fprintf(stderr, "ERROR: could not fork!\n");
        ret = 1; goto exit;
    }

    if (samplingTarget == 0) {
        if (ptrace(PTRACE_TRACEME, NULL, NULL, NULL) == -1) {
            fprintf(stderr,"ptrace traceme failed!\n");
            return 1;
        }
        if (execvp(argsStart[0], argsStart) != 0) {
            fprintf(stderr, "ERROR: failed to execute");
            for (unsigned int i = 0; argsStart[i] != NULL; i++) {
                fprintf(stderr, " %s", argsStart[i]);
            }
            fprintf(stderr,"\n");
            return 1;
        }
    }

    do {
        intrTarget = waitpid(samplingTarget, &intrStatus, __WALL);
    } while (intrTarget == -1 && errno == EINTR);
    
    if (WIFEXITED(intrStatus)) {
        fprintf(stderr,"ERROR: unexpected process termination\n");
        ret = 2; goto exit;
    }

    if (samplingTarget != intrTarget) {
        fprintf(stderr, "ERROR: unexpected pid stopped\n");
        ret = 2; goto exitWithTarget;
    }

    if (ptrace(PTRACE_SETOPTIONS, samplingTarget, NULL, PTRACE_O_TRACECLONE) == -1) {
        fprintf(stderr, "ERROR: Could not set ptrace options!\n");
        ret = 1; goto exitWithTarget;
    }

    struct VMMaps targetMap = {};
    targetMap = getProcessVMMaps(samplingTarget, 1);
    if (targetMap.count == 0) {
       fprintf(stderr, "ERROR: could not detect process vmmap\n");
       ret = 1; goto exitWithTarget;
    }

    struct aggregatedSample aggregateUnknown = {};
    struct aggregatedSample **aggregateMap = NULL;
    if (aggregate) {
        aggregateMap = (struct aggregatedSample **) malloc(targetMap.count * sizeof(struct aggregatedSample *));
        if (aggregateMap == NULL) {
            fprintf(stderr, "ERROR: could not allocate memory for aggregation map\n");
            goto exitWithTarget;
        }
        for (unsigned int i = 0; i < targetMap.count; i++) {
            aggregateMap[i] = (struct aggregatedSample *) malloc(targetMap.maps[i].size * sizeof(struct aggregatedSample));
            if (aggregateMap[i] == NULL) {
                fprintf(stderr, "ERROR: could not allocate memory for aggregation map\n");
                goto exitWithTarget;
            }
            memset(aggregateMap[i], '\0', targetMap.maps[i].size * sizeof(struct aggregatedSample));
        }
    }

#ifdef DEBUG
    for (unsigned int i = 0; i < targetMap.count; i ++) {
        printf("[DEBUG] VMMap %u: 0x%014lx, 0x%014lx, %s\n", i, targetMap.maps[i].addr, targetMap.maps[i].size, targetMap.maps[i].label);
    }
#endif

    if (output != NULL) {
        // Leave place for Magic Number, Wall Time, Time, Samples
        fseek(output, sizeof(uint32_t) + 3 * sizeof(uint64_t), SEEK_SET);
        //Write VMMaps
        fwrite((void *) &targetMap.count, sizeof(uint32_t), 1, output);
        fwrite((void *) targetMap.maps, sizeof(struct VMMap), targetMap.count, output);
    }

    static struct user_regs_struct regs = {};
#ifdef __aarch64__
    static struct iovec rvec = { .iov_base = &regs, .iov_len = sizeof(regs) };
#endif
    
    _callback_data.tid = samplingTarget;
    addTask(samplingTarget);

    struct timerData timer = {};
    uint64_t samples = 0;
    uint64_t interrupts = 0;
    struct timespec timeDiff = {};
    struct timespec currentTime = {};
    
    frequencyToTimespec(&timer.samplingInterval, samplingFrequency);

#ifdef ADAPTIVE_FREQUENCY
    struct timespec nextInterrupt = {};
    struct timespec nextPlannedInterrupt = {};
#else
    _callback_data.block = 0;
#endif

    struct timespec samplerStartTime = {};
    clock_gettime(CLOCK_REALTIME, &samplerStartTime);

#ifdef ESTIMATE_LATENCY    
    clock_t latencyCpuTime = clock();
#else
    struct timespec groupStopStartTime = {};
    struct timespec totalLatencyWallTime = {};
#endif
    
    if (startTimer(&timer) != 0) {
        fprintf(stderr, "ERROR: could not start sampling timer\n");
        goto exitWithTarget;
    }

#ifdef ADAPTIVE_FREQUENCY
    // first sample as soon as possible
    shootTimerIn(&timer, &nextInterrupt);
#endif

    long r;
    do {
        r = ptrace(PTRACE_CONT, samplingTarget, NULL, NULL);
    } while (r == -1L && (errno == EBUSY || errno == EFAULT || errno == ESRCH));

    do {
        bool groupStop = false;
        int stopCount = 0;

        
        while(taskCount > 0) {
            int status;
            int signal;
            pid_t intrTarget;
            do {
                intrTarget = waitpid(-1, &status, __WALL);
            } while (intrTarget == -1 && errno == EAGAIN);
            
                                              
            if (WIFEXITED(status)) {
                removeTask(intrTarget);
                if (taskCount == 0) {
                    debug_printf("[%d] root tracee died\n", intrTarget);
                    goto exitSampler;
                } else {
                    debug_printf("[%d] tracee died\n", intrTarget);
                    if (groupStop == 1 && stopCount == taskCount) {
                        // We waited for this thread to stop
                        // but it died, so grab that sample
                        break;
                    } 
                    continue;
                }
            }

            if (!WIFSTOPPED(status)) {
                fprintf(stderr, "unexpected process state of tid %d\n", intrTarget);
                goto exitWithTarget;
            }

            signal = WSTOPSIG(status);

            if (signal == TRACEE_INTERRUPT_SIGNAL && !groupStop) {
                debug_printf("[%d] initiate group stop\n", intrTarget);
                signal = SIGSTOP;
                groupStop = true;
                stopCount = 0;
            } else if (signal == SIGSTOP) {
                if (!taskExists(intrTarget)) {
                    debug_printf("[%d] new child detected\n", intrTarget);
                    addTask(intrTarget);
                    signal = 0;
                }
                if (groupStop) {
                    debug_printf("[%d] group stop\n", intrTarget);
                    if (++stopCount == taskCount) {
                        break;
                    } else {
                        continue;
                    }
                }
            } else {
                /*
                if (signal == SIGTRAP && (status >> 16) == PTRACE_EVENT_CLONE) {
                    unsigned long eventMessage;
                    if (ptrace(PTRACE_GETEVENTMSG, intrTarget, NULL, &eventMessage) == -1) {
                        fprintf(stderr, "Could not retrieve ptrace event message\n");
                        goto exitWithTarget;
                    }
                    debug_printf("[%d] child born %lu\n", intrTarget, eventMessage);
                    addTask(eventMessage);
                    PTRACE_CONTINUE(intrTarget, NULL);
                } 
                */
                debug_printf("[%d] not traced signal %d\n", intrTarget, signal);
                interrupts++;
            }

            PTRACE_CONTINUE(intrTarget, signal);
#ifndef ESTIMATE_LATENCY
            //Measure time from group stop start, until samples are taken
            //not a perfect latency measurement, but real latency would need
            //a lot of time measurements, impacting latency itself
            if (groupStop) 
                clock_gettime(CLOCK_REALTIME, &groupStopStartTime);
#endif
        }

        double current = getCurrentFromLynsyn(sensor);
        debug_printf("[sample] current: %f A\n", current);
             
        for (unsigned int i = 0; i < taskCount; i++) {
            do {
#ifdef __aarch64__
                rp = ptrace(PTRACE_GETREGSET, tasks[i].tid, NT_PRSTATUS, &rvec);
#else   
                rp = ptrace(PTRACE_GETREGS, tasks[i].tid, NULL, &regs);
#endif
            } while (rp == -1L && errno == ESRCH);
#ifdef __aarch64__
            tasks[i].pc = regs.pc;
#else
            tasks[i].pc = regs.rip;
#endif
            debug_printf("[%d] pc: 0x%lx\n", tasks[i].tid, tasks[i].pc);
            
            if (output != NULL && aggregate) {
                bool counted = false;
                for (unsigned int j = 0; j < targetMap.count; j++) {
                    if (targetMap.maps[j].addr > tasks[i].pc)
                        continue;
                    uint64_t const offset = tasks[i].pc - targetMap.maps[j].addr;
                    if (offset < targetMap.maps[j].size) {
                        aggregateMap[j][offset].samples++;
                        aggregateMap[j][offset].current += current;
                        debug_printf("[%d] aggregate to addr 0x%lx with offset 0x%lx\n", tasks[i].tid, targetMap.maps[j].addr, offset);
                        debug_printf("[%d] %lu samples accumalated %f A\n", tasks[i].tid, aggregateMap[j][offset].samples, aggregateMap[j][offset].current);
                        counted = true;
                        break;
                    }
                }
                if (!counted) {
                    aggregateUnknown.samples++;
                    aggregateUnknown.current += current;
                    debug_printf("[%d] unknown pc 0x%lx\n", tasks[i].tid, tasks[i].pc);
                    debug_printf("[%d] %lu unknown samples accumalated %f A\n", tasks[i].tid, aggregateUnknown.samples, aggregateUnknown.current);
                }
            }
        }

        if (output != NULL && !aggregate) {
            fwrite((void *) &current, sizeof(double), 1, output);
            fwrite((void *) &taskCount, sizeof(uint32_t), 1, output);
            fwrite((void *) tasks, sizeof(struct task), taskCount, output);
        }
        
        samples++;
        groupStop = false;
        
#ifdef ADAPTIVE_FREQUENCY
        timespecAdd(&nextPlannedInterrupt, &_callback_data.lastInterrupt, &timer.samplingInterval);
        clock_gettime(CLOCK_REALTIME, &currentTime);
        timespecSub(&nextInterrupt, &nextPlannedInterrupt, &currentTime);
        shootTimerIn(&timer, &nextInterrupt);
        debug_printf("[DEBUG] next timer in %llu us\n", timespecToMicroseconds(&nextInterrupt));
#else
        _callback_data.block = 0;
#endif
   
#ifndef ESTIMATE_LATENCY
        clock_gettime(CLOCK_REALTIME, &currentTime);
        timespecSub(&timeDiff, &currentTime, &groupStopStartTime);
        timespecAddStore(&totalLatencyWallTime, &timeDiff);
#endif
       for (unsigned int i = 0; i < taskCount; i++) {
            PTRACE_CONTINUE(tasks[i].tid, NULL);
        }
     } while(taskCount > 0);

 exitSampler: ; 

#ifdef ESTIMATE_LATENCY
    uint64_t totalWallLatencyUs = (clock() - latencyCpuTime) * 1000000 / CLOCKS_PER_SEC;
#else
    uint64_t totalWallLatencyUs = timespecToMicroseconds(&totalLatencyWallTime);
#endif

    clock_gettime(CLOCK_REALTIME, &currentTime);
    timespecSub(&timeDiff, &currentTime, &samplerStartTime);
    uint64_t totalWallTimeUs = timespecToMicroseconds(&timeDiff);
    
    if (stopTimer(&timer) != 0) {
        fprintf(stderr, "Could not stop sampling timer\n");
        ret = 1; goto exit;
    }

    if (output != NULL) {
        if (aggregate) {
            fwrite((void *) &aggregateUnknown, sizeof(struct aggregatedSample), 1, output);
            for (unsigned int i = 0; i < targetMap.count; i++) {
                fwrite((void *) &targetMap.maps[i].addr, sizeof(uint64_t), 1, output);
                fwrite((void *) aggregateMap[i], sizeof(struct aggregatedSample), targetMap.maps[i].size, output);
            }
        }
        //HEADER
        uint32_t magic = (aggregate) ? 1 : 0;
        fseek(output, 0, SEEK_SET);
        fwrite((void *) &magic, sizeof(uint32_t), 1, output);
        fwrite((void *) &totalWallTimeUs, sizeof(uint64_t), 1, output);
        fwrite((void *) &totalWallLatencyUs, sizeof(uint64_t), 1, output);
        fwrite((void *) &samples, sizeof(uint64_t), 1, output);
    }

    //Write Header -> Samples, Threads, Offset, sample interval (us)
    
    if (debugOutput) {
        printf("[DEBUG] time       : %10lu us (ideal), %10lu us (actual)\n", totalWallTimeUs - totalWallLatencyUs, totalWallTimeUs);
        printf("[DEBUG] interrupts : %10lu    (total), %10lu    (foreign) \n", interrupts + samples, interrupts );
        printf("[DEBUG] samples    : %10llu    (ideal), %10lu    (actual)  \n", (timespecToMicroseconds(&timer.samplingInterval) > 0) ? totalWallTimeUs / timespecToMicroseconds(&timer.samplingInterval) : 0, samples);
        printf("[DEBUG] latency    : %10lu us (total), %10lu us (sample)\n", totalWallLatencyUs, (samples > 0) ? totalWallLatencyUs / samples : 0);
        
        printf("[DEBUG] frequency  : %10.2f Hz (ideal), %10.2f Hz (actual)\n", samplingFrequency, (samples > 0) ? 1000000.0 / ((double) totalWallTimeUs / samples) : 0);
    }
    ret = 0; goto exit;

exitWithTarget:
    kill(samplingTarget, SIGKILL);
    ptrace(PTRACE_DETACH, samplingTarget, NULL, NULL, NULL);
exit:
    if (sensor >= 0) {
        releaseLynsyn();
    }
lynsynError:
    if (output != NULL) {
        fclose(output);
    }
    return ret;
}
