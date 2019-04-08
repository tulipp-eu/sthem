#include <time.h>
#include <sys/time.h>


#define CLOCKS_PER_MSEC (CLOCKS_PER_SEC / 1000)
#define CLOCKS_PER_USEC (CLOCKS_PER_SEC / 1000000)
#define DEF_TIMER(x) clock_t _timer_##x
#define RESET_TIMER(x) _timer_##x = clock()
#define GET_TIME(x)  ((uint64_t) ((clock() - _timer_##x) / CLOCKS_PER_USEC))

void timespecDifference (struct timespec *result, struct timespec *x, struct timespec *y) {
  if (x->tv_nsec < y->tv_nsec) {
    int nsec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
    y->tv_nsec -= 1000000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_nsec - y->tv_nsec > 1000000000) {
    int nsec = (x->tv_nsec - y->tv_nsec) / 1000000000;
    y->tv_nsec += 1000000000 * nsec;
    y->tv_sec -= nsec;
  }
  if (x->tv_sec < y->tv_sec) {
      result->tv_sec = 0;
      result->tv_nsec = 0;
  } else {
      result->tv_sec = x->tv_sec - y->tv_sec;
      result->tv_nsec = x->tv_nsec - y->tv_nsec;
  }
}

unsigned long long timespecToNanoseconds(struct timespec *t) {
    return (t->tv_sec * 1000000000) + t->tv_nsec;
}

unsigned long long timespecToMicroseconds(struct timespec *t) {
    return timespecToNanoseconds(t) / 1000;
}

unsigned long long timepsecToMilliseconds(struct timespec *t) {
    return timespecToMicroseconds(t) / 1000;
}

unsigned long long timespecToSeconds(struct timespec *t) {
    return t->tv_sec;
}

void frequencyToTimespec(struct timespec *t, double freq) {
    t->tv_sec = (time_t) (1.0 / freq);
    t->tv_nsec = ((long) (1000000000.0 / freq)) % 1000000000L;
}
