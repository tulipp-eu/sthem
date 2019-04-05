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
#include <sys/ptrace.h>
#include <sys/user.h>
#include <spawn.h>
#include <getopt.h>

#include "lynsyn.h"
#include "vmmap.h"

#define CLOCKS_PER_MSEC (CLOCKS_PER_SEC / 1000)
#define CLOCKS_PER_USEC (CLOCKS_PER_SEC / 1000000)

#ifdef DEBUG
#define debug_printf(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#else
#define debug_printf(fmt,...)
#endif


#define DEF_TIMER(x) clock_t _timer_##x
#define RESET_TIMER(x) _timer_##x = clock()
#define GET_TIME(x)  ((uint64_t) ((clock() - _timer_##x) / CLOCKS_PER_USEC))
#define DEF_WALL_TIMER(x) uint64_t _wall_timer_##x
#define RESET_WALL_TIMER(x) _wall_timer_##x = getWallTimeUs()
#define GET_WALL_TIME(x)  ((uint64_t) (getWallTimeUs() - _wall_timer_##x))

unsigned long getWallTimeUs() {
    static struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (spec.tv_sec * 1000000) + (spec.tv_nsec / 1000);
}

static int _ptrace_group_stop = 0;
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
    struct sigaction  signalOldAction;
    struct sigaction  signalAction;
};

struct callbackData {
    pid_t tid;
    int   signal;
    int   block;
};

static struct callbackData _callback_data;


void timerCallback(int sig) {
    if (_callback_data.block == 0) {
        _callback_data.block = 1;
        int r;
        do {
            r = kill(_callback_data.tid, _callback_data.signal);
        } while (r == -1 && errno == EAGAIN);
        debug_printf("[%d] send %d\n", _callback_data.tid, _callback_data.signal);
    }
}


void setTimerFrequency(struct timerData *timer, double freq) {
    timer->time.it_value.tv_sec = 0;
    if (freq == 0.0) { 
        timer->time.it_value.tv_nsec = 0;
    } else {
      timer->time.it_value.tv_nsec = 1;
      timer->time.it_interval.tv_sec = (time_t) (1.0 / freq);
      timer->time.it_interval.tv_nsec = ((long) (1000000000.0 / freq)) % 1000000000L;
    }
}

int startTimer(struct timerData *timer) {
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
    if (timer_settime(timer->timer, 0, &timer->time, NULL) != 0)
        goto start_error;
    
    timer->active = 1;
    return 0;
start_error:
    return -1;
    
}

int stopTimer(struct timerData *timer) {
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
        fseek(output, sizeof(unsigned int) + 3 * sizeof(uint64_t), SEEK_SET);
        //Write VMMaps
        fwrite((void *) &targetMap.count, sizeof(unsigned int), 1, output);
        fwrite((void *) targetMap.maps, sizeof(struct VMMap), targetMap.count, output);
    }

    
    struct timerData timer = {};
    uint64_t samples = 0;
    uint64_t interrupts = 0;
    setTimerFrequency(&timer, samplingFrequency);

    _callback_data.tid = samplingTarget;
    _callback_data.signal = SIGUSR1;
    _callback_data.block = 0;

    DEF_TIMER(total);
    RESET_TIMER(total);
    DEF_WALL_TIMER(total);
    RESET_WALL_TIMER(total);
    
    if (startTimer(&timer) != 0) {
        fprintf(stderr, "ERROR: could not start sampling timer\n");
        goto exitWithTarget;
    }
    addTask(samplingTarget);

    long r;
    do {
        r = ptrace(PTRACE_CONT, samplingTarget, NULL, NULL);
    } while (r == -1L && (errno == EBUSY || errno == EFAULT || errno == ESRCH));

    do {
        int groupStop = 0;
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
                        break;
                    } else {
                        continue;
                    }
                }
            }

            if (!WIFSTOPPED(status)) {
                fprintf(stderr, "unexpected process state of tid %d\n", intrTarget);
                goto exitWithTarget;
            }

            signal = WSTOPSIG(status);

            if (signal == SIGUSR1 && groupStop == 0) {
                debug_printf("[%d] initiate group stop\n", intrTarget);
                groupStop = 1;
                stopCount = 0;
                PTRACE_CONTINUE(intrTarget, SIGSTOP);
                continue;
            }

            if (signal == SIGSTOP) {
                if (!taskExists(intrTarget)) {
                    debug_printf("[%d] new child detected\n", intrTarget);
                    addTask(intrTarget);
                    signal = 0;
                }
                if (groupStop == 1) {
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
        }

        double current = getCurrentFromLynsyn(sensor);

        debug_printf("[sample] current: %f A\n", current);
             
        for (unsigned int i = 0; i < taskCount; i++) {
            static struct user_regs_struct regs = {};
            do {
                rp = ptrace(PTRACE_GETREGS, tasks[i].tid, &regs, &regs);
            } while (rp == -1L && errno == ESRCH);
            tasks[i].pc = regs.rip;
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
            fwrite((void *) &taskCount, sizeof(unsigned int), 1, output);
            fwrite((void *) tasks, sizeof(struct task), taskCount, output);
        }
        
        samples++;
        
        groupStop = 0;
        _callback_data.block = 0;
        
        for (unsigned int i = 0; i < taskCount; i++) {
            PTRACE_CONTINUE(tasks[i].tid, NULL);
        }
    } while(taskCount > 0);

 exitSampler: ; 

    uint64_t totalTime = GET_TIME(total);
    uint64_t totalWallTime = GET_WALL_TIME(total);
    
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
        fwrite((void *) &totalWallTime, sizeof(uint64_t), 1, output);
        fwrite((void *) &totalTime, sizeof(uint64_t), 1, output);
        fwrite((void *) &samples, sizeof(uint64_t), 1, output);
    }

    //Write Header -> Samples, Threads, Offset, sample interval (us)
    
    if (debugOutput) {
        unsigned long samplingInterval = (timer.time.it_interval.tv_sec * 1000000) + (timer.time.it_interval.tv_nsec / 1000);
        printf("[DEBUG] time       : %10lu us (ideal), %10lu us (actual)\n", totalWallTime - totalTime, totalWallTime);
        printf("[DEBUG] interrupts : %10lu    (total), %10lu    (foreign) \n", interrupts + samples, interrupts );
        printf("[DEBUG] samples    : %10lu    (ideal), %10lu    (actual)  \n", (samplingInterval > 0) ? totalWallTime / samplingInterval : 0, samples);
        printf("[DEBUG] latency    : %10lu us (total), %10lu us (sample)\n", totalTime, (samples > 0) ? totalTime / samples : 0);
        
        printf("[DEBUG] frequency  : %10.2f Hz (ideal), %10.2f Hz (actual)\n", samplingFrequency, (samples > 0) ? 1000000.0 / ((double) totalWallTime / samples) : 0);
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
