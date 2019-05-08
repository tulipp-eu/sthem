#include <stdio.h>
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
#include <stdbool.h>

#include "vmmap.h"

struct pidList {
    uint64_t count;
    pid_t *pids;
};

struct pidList getPIDList() {
    struct pidList result = {};
    unsigned int pidAlloc = 1;
    int pid;
    FILE *ps = popen("ps -e | tail -n+2 | awk '{print $1}'", "r");
    while (fscanf(ps,"%d", &pid) == 1) {
        if (result.count == 0) {
            result.pids = (pid_t *) malloc(pidAlloc * sizeof(pid_t));
        } else if (pidAlloc <= result.count) {
            pidAlloc *= 2;
            result.pids = (pid_t *) realloc(result.pids, pidAlloc * sizeof(pid_t));
        }
        if (result.pids == NULL) break;
        result.pids[result.count] = pid;
        result.count++;
    }
    pclose(ps);
    
    if (result.pids == NULL) {
        result.count = 0;
        return result;
    }
    
    result.pids = (pid_t *) realloc(result.pids, result.count * sizeof(pid_t));
    return result;
}

void freePIDList(struct pidList list) {
    if (list.pids != NULL && list.count > 0) {
        free(list.pids);
    }
}

bool collision(pid_t const targetPid ,struct VMMaps const targetMap, pid_t *whom) {
    struct pidList list = getPIDList();
    for (unsigned int i = 0; i < list.count; i++) {
        if (targetPid == list.pids[i]) continue;
        struct VMMaps checkMap = getProcessVMMaps(list.pids[i], 0);
        if (VMMapCollision(targetMap, checkMap)) {
            *whom = list.pids[i];
            freeVMMaps(checkMap);
            freeVMMaps(targetMap);
            freePIDList(list);
            return true;
        }
        freeVMMaps(checkMap);
    }
    freePIDList(list);
    return false;
}


void help(char const opt, char const *optarg) {
    FILE *out = stdout; 
    if (opt != 0) {
        out = stderr;
        if (optarg) {
            fprintf(out, "Invalid parameter - %c %s\n", opt, optarg);
        } else {
            fprintf(out, "Invalid parameter - %c\n", opt);
        }
    }
    fprintf(out, "pmapelf [options] -- <command> [arguments]\n");
    fprintf(out, "\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -v, --vmmap=<file>        write vm map to file\n");
    fprintf(out, "  -o, --offset=<file>       write process offset to file\n");
    fprintf(out, "  -d, --deattach            deattach immediatly, skips vmmap!\n");
    fprintf(out, "  -k, --key                 continue with keypress\n");
    fprintf(out, "  -t, --time=<milliseconds> continue after time\n");
    fprintf(out, "  -c, --collision           check on colissions\n");
    fprintf(out, "  -p, --pid=<file>          write pid of colliding process to file\n");
    fprintf(out, "\n");
    fprintf(out, "Example: pmapelf -o /tmp/offset -v /tmp/vmap -c -k -- stress-ng --cpu 4\n");
}

int main(int const argc, char **argv) {
    FILE *offsetout = stdout; 
    FILE *vmmapout = stdout; 
    char *vmmapFilename = NULL;
    char *offsetFilename = NULL;
    char *pidFilename = NULL;
    char **argsStart = NULL;
    bool keyPress = false;
    unsigned long waitTime = 0;
    bool collisionDetection = false;
    bool deattach = false;
    
    static struct option const long_options[] =  {
        {"help",      no_argument, 0, 'h'},
        {"key",       no_argument, 0, 'k'},
        {"collision", no_argument, 0, 'c'},
        {"deattach",  no_argument, 0, 'd'},
        {"pid",       required_argument, 0, 'p'},
        {"offset",    required_argument, 0, 'o'},
        {"vmmap",     required_argument, 0, 'v'},
        {"time",      required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    while (1) {
        char *endptr;
        int c;
        
        int option_index = 0;
        size_t len = 0;
        
        c = getopt_long (argc, argv, "dkchv:o:t:p:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                break;
            case 'h':
                help(0, NULL);
                return 0;
            case 't':
                waitTime = strtol(optarg, &endptr, 10);
                if (endptr == optarg) {
                    help(c, optarg);
                    return 1;
                }
                break;
            case 'p':
                len = strlen(optarg);
                if (strlen(optarg) == 0) {
                    help(c ,optarg);
                    return 1;
                }
                pidFilename = (char *) malloc(len + 1);
                if (pidFilename == NULL) {
                    fprintf(stderr, "Memory allocation of %ld bytes failed!\n", len);
                    return 1;
                }
                memset(pidFilename, 0, len + 1);
                strncpy(pidFilename, optarg, len);
                break;
            case 'v':
                len = strlen(optarg);
                if (strlen(optarg) == 0) {
                    help(c ,optarg);
                    return 1;
                }
                vmmapFilename = (char *) malloc(len + 1);
                if (vmmapFilename == NULL) {
                    fprintf(stderr, "Memory allocation of %ld bytes failed!\n", len);
                    return 1;
                }
                memset(vmmapFilename, 0, len + 1);
                strncpy(vmmapFilename, optarg, len);
                break;
            case 'o':
                len = strlen(optarg);
                if (strlen(optarg) == 0) {
                    help(c ,optarg);
                    return 1;
                }
                offsetFilename = (char *) malloc(len + 1);
                if (offsetFilename == NULL) {
                    fprintf(stderr, "Memory allocation of %ld bytes failed!\n", len);
                    return 1;
                }
                memset(offsetFilename, 0, len + 1);
                strncpy(offsetFilename, optarg, len);
                break;
            case 'k':
                keyPress = true;
                break;
            case 'd':
                deattach = true;
                break;
            case 'c':
                collisionDetection = true;
                break;
            default:
                abort();
        }
    }


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0 && (i + 1) < argc) {
            argsStart = &argv[i + 1];
        }
    }

    if (argsStart == NULL) {
        help(' ', "no command specified");
        return 1;
    }

    pid_t targetPid;
    pid_t intrTarget;

    int ret = 0;

    long rptrace;
    uint64_t interrupts = 0;
    
    do {
        targetPid = fork();
    } while (targetPid == -1 && errno == EAGAIN);

    if (targetPid == -1) {
        fprintf(stderr, "ERROR: could not fork!\n");
        ret = 1; goto exit; 
    }

    if (targetPid == 0) {
        if (ptrace(PTRACE_TRACEME, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "ERROR: ptrace traceme failed!\n");
            return 1;
        }
        if (execvp(argsStart[0], argsStart) != 0) {
            fprintf(stderr, "ERROR: failed to execute");
            for (unsigned int i = 0; argsStart[i] != NULL; i++) {
                fprintf(stderr, " %s", argsStart[i]);
            }
            fprintf(stderr, "\n");
            return 1;
            
        }
    } else {
        int status;

        do {
            intrTarget = waitpid(targetPid, &status, 0);
        } while (intrTarget == -1 && errno == EAGAIN);

        if (WIFEXITED(status)) {
            fprintf(stderr,"ERROR: unexpected process termination\n");
            ret = 2; goto exit;
        }


        if (ptrace(PTRACE_SETOPTIONS, targetPid, NULL,  PTRACE_O_EXITKILL | PTRACE_O_TRACEEXIT) == -1) {
            fprintf(stderr, "ERROR: Could not set ptrace options!\n");
            ret = 1; goto exitWithTarget;
        }
        
        struct VMMaps targetMap = getProcessVMMaps(targetPid, 1);
        if (targetMap.count == 0) {
            fprintf(stderr, "ERROR: could not find process memory mapping!\n");
            ret = 1; goto exitWithTarget;
        }

        if (collisionDetection) {
            pid_t whom;
            if (collision(targetPid, targetMap, &whom)) {
                if (pidFilename != NULL) {
                    FILE *pidout = fopen(pidFilename, "w+");
                    if (pidout == NULL) {
                        fprintf(stderr, "ERROR: Could not open %s for writing!\n", pidFilename);
                        ret = 1; goto exitWithTarget;
                    }
                    fprintf(pidout, "%i", whom);
                    fclose(pidout);
                }
                fprintf(stderr, "ERROR: current virtual memory mapping collides with pid %d\n", whom);
                ret = 1; goto exitWithTarget;
            }
        }

        { //Output Offset
            if (offsetFilename != NULL) {
                offsetout = fopen(offsetFilename, "w+");
                if (offsetout == NULL) {
                    fprintf(stderr, "ERROR: Could not open %s for writing!\n", offsetFilename);
                    ret = 1; goto exitWithTarget;
                }
            }
            
            fprintf(offsetout, "0x%016lx 0x%016lx %s\n", targetMap.maps[0].addr, targetMap.maps[0].size, targetMap.maps[0].label);
            
            if (offsetFilename != NULL) {
                fclose(offsetout);
            }
        }
       
        freeVMMaps(targetMap);

        if (keyPress <= 0 && waitTime > 0) {
            usleep(waitTime * 1000);
        } else if (keyPress > 0) {
            fgetc(stdin);
        }

        if (!deattach) {
            int signal = 0;
            do {
                rptrace = ptrace(PTRACE_CONT, targetPid, NULL, signal);
                if (rptrace == -1 && errno == ESRCH) {
                    fprintf(stderr, "ERROR: death under ptrace!\n");
                    ret = 1; goto exit;
                }
                
                do {
                    intrTarget = waitpid(targetPid, &status, 0);
                } while (intrTarget == -1 && errno == EAGAIN);
                
                interrupts++;
                signal = WSTOPSIG(signal);
                //fprintf(stderr, "DEBUG: interrupt signal %d\n", WSTOPSIG(status));
                
            } while (status>>8 != (SIGTRAP | (PTRACE_EVENT_EXIT<<8)));
            
            
            targetMap = getProcessVMMaps(targetPid, 0);
            if (targetMap.count == 0) {
                fprintf(stderr, "ERROR: could not find final process memory mapping!\n");
                ret = 1; goto exitWithTarget;
            }

            { // VMMap Output
                if (vmmapFilename != NULL) {
                    vmmapout = fopen(vmmapFilename, "w+");
                    if (vmmapout == NULL) {
                        fprintf(stderr, "ERROR: Could not open %s for writing!\n", offsetFilename);
                        ret = 1; goto exitWithTarget;
                    }
                }

                for( unsigned int i = 0; i < targetMap.count; i++) {
                    fprintf(vmmapout, "0x%016lx 0x%016lx %s\n", targetMap.maps[i].addr, targetMap.maps[i].size, targetMap.maps[i].label);
                }

                if (vmmapFilename != NULL) {
                    fclose(vmmapout);
                }
            }
            /*
            if (collisionDetection) {
                pid_t whom;
                if (collision(targetPid, targetMap, &whom)) {
                    if (pidFilename != NULL) {
                        FILE *pidout = fopen(pidFilename, "w+");
                        if (pidout == NULL) {
                            fprintf(stderr, "ERROR: Could not open %s for writing!\n", pidFilename);
                            ret = 1; goto exitWithTarget;
                        }
                        fprintf(pidout, "%i", whom);
                        fclose(pidout);
                    }
                    fprintf(stderr, "ERROR: virtual memory mapping collided with pid %d\n", whom);
                    ret = 1; goto exitWithTarget;
                }
            }
            */
            freeVMMaps(targetMap);
           
        }

        ptrace(PTRACE_DETACH, targetPid, NULL, NULL, NULL);
        do {
            intrTarget = waitpid(targetPid, &status, 0);
        } while (intrTarget == -1 && errno == EAGAIN);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            ret = 0;
        } else {
            ret = 2;
        }

        
    }

    if (interrupts > 1) {
        fprintf(stderr, "WARNING: %ld unexpected appplication interrupts occured, non-intrusive execution was not preserved!\n", interrupts - 1);
    }

    goto exit;

 exitWithTarget:
    kill(targetPid, SIGKILL);
    ptrace(PTRACE_DETACH, targetPid, NULL, NULL);
 exit:
    
    if (vmmapFilename) {
        free(vmmapFilename);
    }

    if (offsetFilename) {
        free(offsetFilename);
    }

    return ret;
}
