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

#define CLOCKS_PER_MSEC (CLOCKS_PER_SEC / 1000)
#define CLOCKS_PER_USEC (CLOCKS_PER_SEC / 1000000)

#define DEBUG 0
#define debug_printf(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while(0)


#define DEF_TIMER(x) unsigned long _timer_##x
#define RESET_TIMER(x) _timer_##x = getTimeUs()
#define GET_TIME(x)  ((unsigned long) (getTimeUs() - _timer_##x))

unsigned long getTimeUs() {
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


#define dynamicExecutableThreshold     0x0000500000000000
#define dynamicExecutableCodeThreshold 0x0000700000000000

void realPC(unsigned long long *pc, unsigned long long const offset) {
    if ((*pc > dynamicExecutableThreshold) && (*pc < dynamicExecutableCodeThreshold)) {
            *pc = *pc - offset;
        }
}
    
int getProcessOffset(pid_t pid, unsigned long long *offset) {
    int ret = 0;
    char cmd[1024];
    if (snprintf(cmd,1024,"pmap -q -d %d | awk '$3 ~ /x/ && $5 != \"000:00000\" {print \"0x\"$1}'", pid) <= 0) {
        return -1;
    }
    FILE *ps = popen(cmd, "r");
    if (fscanf(ps,"%llx", offset) != 1) {
        ret = -1;
    }
    pclose(ps);
    if (*offset < dynamicExecutableThreshold) 
        *offset = 0;
    return ret;
}

unsigned int taskCount = 0;
unsigned int allocTasks = 0;

struct task {
    pid_t tid;
    unsigned long long int pc;
};

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
    fprintf(out, "  -s, --sensor=<1-7>        lynsyn sensor\n");
    fprintf(out, "  -f, --frequency=<hertz>   sampling frequency\n");
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


int main(int const argc, char **argv) {
    char const *stdOutput = "profile.bin";
    FILE *output = stdout;
    char **argsStart = NULL;
    int sensor = -1;
    int debugOutput = 0;
    double samplingFrequency = 10000;
    unsigned long long offset = 0;
    
    static struct option const long_options[] =  {
        {"help",         no_argument, 0, 'h'},
        {"debug",        no_argument, 0, 'd'},
        {"sensor",        no_argument, 0, 's'},
        {"frequency",    required_argument, 0, 'f'},
        {"output",       required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    static char const * short_options = "hdf:o:s:";

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
            case 'd':
                debugOutput = 1;
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

    if (output == stdout) {
        if (access(stdOutput, F_OK) == -1) {
            output = fopen(stdOutput, "w+");
        }
        if (output == stdout || output == NULL) {
            fprintf(stderr, "either %s already exists or it cannot be created\n", stdOutput);
            return 1;
        }
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
        fprintf(stderr, "Could not set ptrace options!\n");
        ret = 1; goto exitWithTarget;
    }

    if (getProcessOffset(samplingTarget, &offset) != 0) {
       fprintf(stderr, "ERROR: could not detect process offset\n");
       ret = 1; goto exitWithTarget;
    }


    struct timerData timer = {};
    unsigned long long int samples = 0;
    unsigned long long int interrupts = 0;
    setTimerFrequency(&timer, samplingFrequency);

    _callback_data.tid = samplingTarget;
    _callback_data.signal = SIGUSR1;
    _callback_data.block = 0;

    DEF_TIMER(total);
    RESET_TIMER(total);
    
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

        double energy = (sensor >= 0) ? getCurrent(sensor) : 0;
        
        fwrite((void *) &taskCount, sizeof(unsigned int), 1, output);
        fwrite((void *) &energy, sizeof(double), 1, output);
             
        for (unsigned int i = 0; i < taskCount; i++) {
            debug_printf("[%d] sample\n", tasks[i].tid);
            static struct user_regs_struct regs = {};
            do {
                rp = ptrace(PTRACE_GETREGS, tasks[i].tid, &regs, &regs);
            } while (rp == -1L && errno == ESRCH);
            realPC(&regs.rip, offset);
            fwrite((void *) &regs.rip, sizeof(unsigned long long int), 1, output);
                
        }
        interrupts++;
        samples++;
        
        groupStop = 0;
        _callback_data.block = 0;
        
        for (unsigned int i = 0; i < taskCount; i++) {
            PTRACE_CONTINUE(tasks[i].tid, NULL);
        }
    } while(taskCount > 0);

exitSampler:
    
    if (stopTimer(&timer) != 0) {
        fprintf(stderr, "Could not stop sampling timer\n");
        ret = 1; goto exit;
    }


    //Write Header -> Samples, Threads, Offset, sample interval (us)
    
    if (debugOutput) {
        unsigned long long totalTime = GET_TIME(total);
        unsigned long long samplingInterval = (timer.time.it_interval.tv_sec * 1000000) + (timer.time.it_interval.tv_nsec / 1000);
        printf("[SAMPLER] offset    : 0x%llx\n", offset);
        printf("[SAMPLER] time      : %10llu us (total)\n", totalTime);
        printf("[SAMPLER] interrupts: %10llu    (total), %10llu    (foreign) \n", interrupts, interrupts - samples );
        if (samplingInterval >0 ) {
            printf("[SAMPLER] samples   : %10llu    (ideal), %10llu    (actual)  \n", totalTime / samplingInterval, samples);
        }
        //printf("[SAMPLER] latency   : %10llu us (total), %10llu us (sample)  \n", totalLatency, totalLatency / samples);
        if (samples > 0) {
            printf("[SAMPLER] frequency : %10.2f Hz (ideal), %10.2f Hz (actual)\n", samplingFrequency, 1000000.0 / ((double) totalTime / samples));
        }
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
    fclose(output);
    return ret;
}
