#ifndef __AFS_UTIL_H__
#define __AFS_UTIL_H__

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <assert.h>
#include <time.h>

#define MB (1024*1024)
#define GB (1024*1024*1024)


static inline double now_us ()
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec * (uint64_t) 1000000 + (double)tv.tv_nsec/1000);
}

#endif // AFS_UTIL_HPP_
