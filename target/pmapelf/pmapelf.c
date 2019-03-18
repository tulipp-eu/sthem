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

static const void *PTRACE_NO_SIGNAL = 0;
static const void *PTRACE_IGNORE_PTR;

extern char **environ;

struct VMMaps {
  unsigned int count;
  uint64_t* start;
  uint64_t* length;
};

pid_t* getPIDs(unsigned int *count) {
  pid_t* result = NULL;
  int pid;
  *count = 0;
  unsigned int pidAlloc = 1;
  FILE *ps = popen("ps -e | tail -n+2 | awk '{print $1}'", "r");
  while (fscanf(ps,"%d", &pid) == 1) {
    if (*count == 0) {
      result = (pid_t *) malloc(pidAlloc * sizeof(pid_t));
    } else if (pidAlloc <= *count) {
      pidAlloc *= 2;
      result = (pid_t *) realloc(result, pidAlloc * sizeof(pid_t));
    }
    if (result == NULL) break;
    result[(*count)] = pid;
    (*count) = (*count) + 1;
  }
  pclose(ps);

  if (result == NULL) {
    return result;
  }
  
  result = (pid_t *) realloc(result, (*count) * sizeof(pid_t));
  return result;
}

struct VMMaps getProcessVMMaps(pid_t pid, unsigned int const limit) {
  unsigned int allocCount = 1;
  struct VMMaps result;
  result.count = 0;
  result.start = NULL;
  result.length = NULL;
  uint64_t start;
  uint64_t length;
  char cmd[1024];
  if (snprintf(cmd,1024,"pmap -q -d %d | awk '$3 ~ /x/ {print \"0x\"$1\" \"$2}'", pid) <= 0) {
    return result;
  }
  FILE *ps = popen(cmd, "r");
  while (fscanf(ps,"%lx %lu", &start, &length) == 2) {
    if (result.count == 0) {
      result.start = (uint64_t *) malloc(allocCount * sizeof(uint64_t));
      result.length = (uint64_t *) malloc(allocCount * sizeof(uint64_t));
    } else if (allocCount <= result.count) {
      allocCount *= 2;
      result.start = (uint64_t *) realloc(result.start, allocCount * sizeof(uint64_t));
      result.length = (uint64_t *) realloc(result.length, allocCount * sizeof(uint64_t));
    }
    if (result.start == NULL || result.length == NULL) {
      break;
    }
    result.start[result.count] = start;
    result.length[result.count] = length * 1024;
    result.count = result.count + 1;
    if (limit == result.count) break;
  }
  pclose(ps);
  
  if (result.start == NULL || result.length == NULL) {
    if (result.start != NULL) free(result.start);
    if (result.length != NULL) free(result.length);
    result.count = 0;
    return result;
  }
  
  result.start = (uint64_t *) realloc(result.start, result.count * sizeof(uint64_t));
  result.length = (uint64_t *) realloc(result.length, result.count * sizeof(uint64_t));
  
  return result;
}

void freeVMMaps( struct VMMaps map ) {
  if (map.start != NULL) free(map.start);
  if (map.length != NULL) free(map.length);
}

void dumpVMMaps( struct VMMaps map ) {
  for (unsigned int i = 0; i < map.count; i++) {
    printf("%02d: 0x%lx - 0x%lx\n", i, map.start[i], map.start[i] + map.length[i]);
  }
}

int VMMapCollision(struct VMMaps map1, struct VMMaps map2) {
  for (unsigned int i = 0; i < map1.count; i++) {
    uint64_t const m1start = map1.start[i];
    uint64_t const m1end = m1start + map1.length[i];
    for (unsigned int j = 0; j < map2.count; j++) {
      uint64_t const m2start = map2.start[j];
      uint64_t const m2end = map2.start[j] + map2.length[j];
      if (m1start >= m2start && m1start < m2end) return 1;
      if (m1end >= m2start && m1end < m2end) return 1;
    }
  }
  return 0;
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
    fprintf(out, "  -o, --output=<file>       write pmap to file\n");
    fprintf(out, "  -c, --collision           check on colissions\n");
    fprintf(out, "  -k, --key                 continue with keypress\n");
    fprintf(out, "  -t, --time=<milliseconds> continue after time\n");
    fprintf(out, "  -s, --short               output mapped text region\n");
    fprintf(out, "\n");
    fprintf(out, "Example: pmapelf -o /tmp/map -c -k -- stress-ng --cpu 4\n");
}

int main(int const argc, char **argv) {
    char *outputFilename = NULL;
    unsigned long waitTime = 0;
    int keyPress = -1;
    char **argsStart = NULL;
    int collisionDetection = -1;
    int shortOutput = -1;
    
    static struct option const long_options[] =  {
        {"help",      no_argument, 0, 'h'},
        {"short",     no_argument, 0, 's'},
        {"key",       no_argument, 0, 'k'},
        {"collision", no_argument, 0, 'c'},
        {"output",    required_argument, 0, 'o'},
        {"time",      required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    while (1) {
        char *endptr;
        int c;
        int option_index = 0;
        size_t len = 0;
        
        c = getopt_long (argc, argv, "skcho:t:", long_options, &option_index);
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
                    return -1;
                }
                break;
            case 'o':
                len = strlen(optarg);
                if (strlen(optarg) == 0) {
                    help(c ,optarg);
                    return -1;
                }
                outputFilename = (char *) malloc(len + 1);
                if (outputFilename == NULL) {
                    fprintf(stderr, "Memory allocation of %ld bytes failed!\n", len);
                    return -1;
                }
                memset(outputFilename, 0, len + 1);
                strncpy(outputFilename, optarg, len);
                break;
            case 'k':
                keyPress = 1;
                break;
            case 's':
                shortOutput = 1;
                break;
            case 'c':
                collisionDetection = 1;
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
        return -1;
    }

    pid_t targetPid;
    
    do {
        targetPid = fork();
    } while (targetPid == -1 && errno == EAGAIN);

    if (targetPid == -1) {
        fprintf(stderr, "ERROR: could not fork!\n");
        return -1;
    }

    if (targetPid == 0) {
        ptrace(PTRACE_TRACEME, PTRACE_IGNORE_PTR, PTRACE_IGNORE_PTR, PTRACE_IGNORE_PTR);
        execvp(argsStart[0], argsStart);
    } else {
        int status;
        waitpid(targetPid, &status, 0);
        if (WIFEXITED(status)) {
            fprintf(stderr,"ERROR: unexpected process termination\n");
            return -1;
        }

        if (shortOutput <= 0) {
            char tplStd[] = "pmap -d -q %d";
            char tplFile[] = "pmap -d -q %d 2>&1 >%2";
            char *tpl = tplStd;
            char *cmd;
            int alloc;
            if (outputFilename != NULL) {
                tpl = tplFile;
            }
            alloc = snprintf(cmd, 0, tpl, targetPid, outputFilename);
            cmd = (char *) malloc((++alloc) * sizeof(char *));
            if (cmd == NULL) {
                fprintf(stderr, "ERROR: could not allocate %d bytes", alloc);
                kill(targetPid, SIGKILL);
                return -1;
            }
            snprintf(cmd, alloc, tpl, targetPid, outputFilename);
            if (system(cmd) < 0) {
                fprintf(stderr, "ERROR: could not execute %s", cmd);
                kill(targetPid, SIGKILL);
                return -1;
            }
            free(cmd);
        }

        if (collisionDetection > 0 || shortOutput > 0) {
            struct VMMaps targetMap = getProcessVMMaps(targetPid, 1);
            if (targetMap.count == 0) {
                fprintf(stderr, "ERROR: could not find virtual memory mapping!\n");
                kill(targetPid, SIGKILL);
                return -1;
            }

            if (collisionDetection > 0) {
                unsigned int pidCount = 0;
                pid_t *pids = getPIDs(&pidCount);

                for (unsigned int i = 0; i < pidCount; i++) {
                    if (targetPid == pids[i]) continue;
                    struct VMMaps checkMap = getProcessVMMaps(pids[i],0);
                    if (VMMapCollision(targetMap, checkMap)) {
                        fprintf(stderr, "ERROR: current virtual memory mapping collides with pid %d\n", pids[i]);
                        freeVMMaps(checkMap);
                        freeVMMaps(targetMap);
                        free(pids);
                        kill(targetPid, SIGKILL);
                        return -1;
                    }
                    freeVMMaps(checkMap);
                }
                free(pids);
            }

            if (shortOutput > 0) {
                FILE *out = stdout;

                if (outputFilename != NULL) {
                    out = fopen(outputFilename, "w+");
                    if (out == NULL) {
                        fprintf(stderr, "ERROR: Could not open %s for writing!\n", outputFilename);
                        free(outputFilename);
                        return -1;
                    }
                }

                for (unsigned int i = 0; i < targetMap.count; i++) {
                    fprintf(out, "0x%lx - 0x%lx\n", targetMap.start[i], targetMap.start[i] + targetMap.length[i]);
                }
                
                if (outputFilename != NULL) {
                    fclose(out);
                    free(outputFilename);
                }
            }
            
            freeVMMaps(targetMap);
        }


        
        if (keyPress <= 0 && waitTime > 0) {
            usleep(waitTime * 1000);
        } else if (keyPress > 0) {
            fgetc(stdin);
        }

        ptrace(PTRACE_DETACH, targetPid, PTRACE_IGNORE_PTR, PTRACE_NO_SIGNAL);
        waitpid(targetPid, &status, 0);
        return status;
    }

    if (outputFilename) {
        free(outputFilename);
    }

    return 0;
}
