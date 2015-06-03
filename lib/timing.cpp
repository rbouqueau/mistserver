/// \file timing.cpp
/// Utilities for handling time and timestamps.

#include "timing.h"
#include <time.h>//for time and nanosleep

//emulate clock_gettime() for OSX compatibility
#if defined(__APPLE__) || defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#define CLOCK_REALTIME CALENDAR_CLOCK
#define CLOCK_MONOTONIC SYSTEM_CLOCK
void clock_gettime(int ign, struct timespec * ts) {
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), ign, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
}
#endif

/// Sleeps for the indicated amount of milliseconds or longer.
/// Will not sleep if ms is negative.
/// Will not sleep for longer than 10 minutes (600000ms).
/// If interrupted by signal, resumes sleep until at least ms milliseconds have passed.
/// Can be slightly off (in positive direction only) depending on OS accuracy.
void Util::wait(int ms){
  if (ms < 0) {
    return;
  }
  if (ms > 600000) {
    ms = 600000;
  }
  long long start = getMS();
  long long now = start;
  while (now < start+ms){
    sleep(start+ms-now);
    now = getMS();
  }
}

/// Sleeps for roughly the indicated amount of milliseconds.
/// Will not sleep if ms is negative.
/// Will not sleep for longer than 100 seconds (100000ms).
/// Can be interrupted early by a signal, no guarantee of minimum sleep time.
/// Can be slightly off depending on OS accuracy.
void Util::sleep(int ms) {
  if (ms < 0) {
    return;
  }
  if (ms > 100000) {
    ms = 100000;
  }
  struct timespec T;
  T.tv_sec = ms / 1000;
  T.tv_nsec = 1000000 * (ms % 1000);
  nanosleep(&T, 0);
}

long long Util::getNTP() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((((long long)t.tv_sec) + 2208988800) << 32) + (t.tv_nsec * 4.2949);
}

/// Gets the current time in milliseconds.
long long Util::getMS() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long)t.tv_sec) * 1000 + t.tv_nsec / 1000000;
}

long long Util::bootSecs() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec;
}

/// Gets the current time in microseconds.
long long unsigned int Util::getMicros() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_nsec / 1000;
}

/// Gets the time difference in microseconds.
long long unsigned int Util::getMicros(long long unsigned int previous) {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_nsec / 1000 - previous;
}

/// Gets the amount of seconds since 01/01/1970.
long long Util::epoch() {
  return time(0);
}
