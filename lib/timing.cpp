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

/* MSVC compat from http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows*/
#ifdef _MSC_VER
#include <Winsock2.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
LARGE_INTEGER
getFILETIMEoffset()
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

int
clock_gettime(int X, struct timeval *tv)
{
	LARGE_INTEGER           t;
	FILETIME            f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter) {
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
		}
		else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.;
		}
	}
	if (usePerformanceCounter) QueryPerformanceCounter(&t);
	else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = (LONGLONG)microseconds;
	tv->tv_sec = (long)(t.QuadPart / 1000000);
	tv->tv_usec = t.QuadPart % 1000000;
	return (0);
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
#ifdef _MSC_VER
  //struct timeval T;
  //T.tv_sec = ms / 1000;
  //T.tv_usec = 1000 * (ms % 1000);
  sleep(ms);
#else
  struct timespec T;
  T.tv_sec = ms / 1000;
  T.tv_nsec = 1000000 * (ms % 1000);
  nanosleep(&T, 0);
#endif
}

long long Util::getNTP() {
#ifdef _MSC_VER
  struct timeval t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((((long long)t.tv_sec) + 2208988800) << 32) + (t.tv_usec*1000 * 4.2949);
#else
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((((long long)t.tv_sec) + 2208988800) << 32) + (t.tv_nsec * 4.2949);
#endif
}

/// Gets the current time in milliseconds.
long long Util::getMS() {
#ifdef _MSC_VER
  struct timeval t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long)t.tv_sec) * 1000 + t.tv_usec / 1000;
#else
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long)t.tv_sec) * 1000 + t.tv_nsec / 1000000;
#endif
}

long long Util::bootSecs() {
#ifdef _MSC_VER
  struct timeval t;
#else
  struct timespec t;
#endif
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec;
}

/// Gets the current time in microseconds.
long long unsigned int Util::getMicros() {
#ifdef _MSC_VER
  struct timeval t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_usec;
#else
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_nsec / 1000;
#endif
}

/// Gets the time difference in microseconds.
long long unsigned int Util::getMicros(long long unsigned int previous) {
#ifdef _MSC_VER
	struct timeval t;
	clock_gettime(CLOCK_REALTIME, &t);
	return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_usec - previous;
#else
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return ((long long unsigned int)t.tv_sec) * 1000000 + t.tv_nsec / 1000 - previous;
#endif
}

/// Gets the amount of seconds since 01/01/1970.
long long Util::epoch() {
  return time(0);
}
