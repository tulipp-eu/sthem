#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define VMMAP_LABEL_LENGTH 255
#define STRINGIFY_EVAL(x) #x
#define STRINGIFY(x) STRINGIFY_EVAL(x)

struct VMMap {
    uint64_t addr;
    uint64_t size;
    char label[VMMAP_LABEL_LENGTH + 1];
} __attribute__((packed));

struct VMMaps {
    uint32_t count;
    struct VMMap *maps;
};

struct VMMaps getProcessVMMaps(pid_t pid, unsigned int const limit) {
    unsigned int allocCount = 1;
    struct VMMap map = {};
    struct VMMaps result = {};
    char cmd[1024];
    if (snprintf(cmd,1024,"pmap -q -d %d | awk '$3 ~ /x/ && $5 != \"000:00000\" {print \"0x\"$1\" \"$2\" \"$6}'", pid) <= 0) {
        return result;
    }
    FILE *ps = popen(cmd, "r");
    while (fscanf(ps,"%lx %lu %"STRINGIFY(VMMAP_LABEL_LENGTH)"s", &map.addr, &map.size, map.label) == 3) {
        if (result.count == 0) {
            result.maps = (struct VMMap *) malloc(allocCount * sizeof(struct VMMap));
        } else if (allocCount <= result.count) {
            allocCount *= 2;
            result.maps = (struct VMMap *) realloc(result.maps, allocCount * sizeof(struct VMMap));
        }
        if (result.maps == NULL) {
            break;
        }
        //kilobyte to byte
        map.size *= 1024;
        memcpy((void *) &result.maps[result.count], (void *) &map, sizeof(struct VMMap));
        result.count++;
        if (limit == result.count) break;
        memset((void *) &map, '\0', sizeof(struct VMMap));
    }
    pclose(ps);

    if (result.maps == NULL) {
        result.count = 0;
        return result;
    }
    result.maps = (struct VMMap *) realloc(result.maps, result.count * sizeof(struct VMMap));

    return result;
}

void freeVMMaps( struct VMMaps map ) {
    if (map.maps != NULL) free(map.maps);
}

void dumpVMMaps( struct VMMaps map ) {
    for (unsigned int i = 0; i < map.count; i++) {
        printf("%02d: 0x%lx - 0x%lx - %s\n", i, map.maps[i].addr, map.maps[i].size, map.maps[i].label);
    }
}

int VMMapCollision(struct VMMaps map1, struct VMMaps map2) {
    for (unsigned int i = 0; i < map1.count; i++) {
        uint64_t const m1start = map1.maps[i].addr;
        uint64_t const m1end = m1start + map1.maps[i].size;
        for (unsigned int j = 0; j < map2.count; j++) {
            uint64_t const m2start = map2.maps[j].addr;
            uint64_t const m2end = m2start + map2.maps[j].size;
            if (m1start >= m2start && m1start < m2end) return 1;
            if (m1end >= m2start && m1end < m2end) return 1;
        }
    }
    return 0;
}
