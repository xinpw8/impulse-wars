#pragma once

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "box2d/box2d.h"

#include "include/cc_deque.h"

#include "include/dlmalloc.h"

#ifndef NDEBUG
#define ON_ERROR __builtin_trap()
#define DEBUG_LOGF(fmt, args...)                                                                                                              \
    do                                                                                                                                        \
    {                                                                                                                                         \
        time_t _t = time(NULL);                                                                                                               \
        struct tm *_timeinfo;                                                                                                                 \
        _timeinfo = localtime(&_t);                                                                                                           \
        printf(fmt " %d:%d:%d %s:%s:%d\n", args, _timeinfo->tm_hour, _timeinfo->tm_min, _timeinfo->tm_sec, __FILE__, __FUNCTION__, __LINE__); \
        fflush(stdout);                                                                                                                       \
    } while (0)
#define DEBUG_LOG(msg) DEBUG_LOGF(msg, NULL)
#define ASSERT(condition)                                                                                   \
    do                                                                                                      \
    {                                                                                                       \
        if (!(condition))                                                                                   \
        {                                                                                                   \
            printf("\nASSERTION FAILED: %s at %s:%s:%d\n\n", #condition, __FILE__, __FUNCTION__, __LINE__); \
            fflush(stdout);                                                                                 \
            ON_ERROR;                                                                                       \
        }                                                                                                   \
    } while (0)
#define ASSERTF(condition, fmt, args...)                                                                                    \
    do                                                                                                                      \
    {                                                                                                                       \
        if (!(condition))                                                                                                   \
        {                                                                                                                   \
            printf("\nASSERTION FAILED: %s; " fmt "; at %s:%s:%d\n\n", #condition, args, __FILE__, __FUNCTION__, __LINE__); \
            fflush(stdout);                                                                                                 \
            ON_ERROR;                                                                                                       \
        }                                                                                                                   \
    } while (0)
#else
#define ON_ERROR abort()
#define DEBUG_LOGF(fmt, args...)
#define DEBUG_LOG(msg)
#define ASSERT(condition)
#define ASSERTF(condition, fmt, args...)
#endif

#define ERRORF(fmt, args...)                                                    \
    fprintf(stderr, " %s:%s:%d\n" fmt, __FILE__, __FUNCTION__, __LINE__, args); \
    fflush(stderr);                                                             \
    ON_ERROR
#define ERROR(msg) ERRORF(msg, NULL)

// ignore compiler warnings about unused variables for variables that are
// only used in debug builds
#define MAYBE_UNUSED(x) (void)x

#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef RAD2DEG
#define RAD2DEG (180.0f / PI)
#endif

#define INV_MASS(density, radius) (1.0f / (density * PI * radius * radius))

#define ASSERT_VEC_NORMALIZED(vec)               \
    ASSERTF(vec.x <= 1.0f, "vec.x: %f", vec.x);  \
    ASSERTF(vec.x >= -1.0f, "vec.x: %f", vec.x); \
    ASSERTF(vec.y <= 1.0f, "vec.y: %f", vec.y);  \
    ASSERTF(vec.y >= -1.0f, "vec.y: %f", vec.y);

// use malloc when debugging so the address sanitizer can find issues with
// heap memory, use dlmalloc in release mode for performance
#ifndef NDEBUG
#define fastMalloc(size) malloc(size)
#define fastCalloc(nmemb, size) calloc(nmemb, size)
#define fastFree(ptr) free(ptr)
#else
#define fastMalloc(size) dlmalloc(size)
#define fastCalloc(nmemb, size) dlcalloc(nmemb, size)
#define fastFree(ptr) dlfree(ptr)
#endif

// returns a pointer to a copy of data that is allocated on the heap;
// allows the caller to assign a heap-allocated struct with const members
static inline void *createConstStruct(const void *const data, const size_t size)
{
    void *const ptr = fastMalloc(size);
    memcpy(ptr, data, size);
    return ptr;
}

// automatically checks that the index is valid and returns the value
// so callers can use it as a constant expression
static inline void *safe_deque_get_at(const CC_Deque *const deque, size_t index)
{
    void *val;
    const enum cc_stat res = cc_deque_get_at(deque, index, &val);
    ASSERT(res == CC_OK);
    MAYBE_UNUSED(res);
    return val;
}

static inline bool b2VecEqual(const b2Vec2 v1, const b2Vec2 v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

static inline float randFloat(const float min, const float max)
{
    float n = rand() / (float)RAND_MAX;
    return min + n * (max - min);
}

static inline int randInt(const int min, const int max)
{
    return min + rand() % (max - min + 1);
}

static inline float logBasef(const float v, const float b)
{
    return log2f(v) / log2(b);
}

// normalize value to be between 0 and max, and clamp to 0 and max;
// minIsZero determines if the min value is 0 or -max
static inline float scaleObsValue(const float v, const float max, const bool minIsZero)
{
    ASSERTF(v <= max, "v: %f, max: %f", v, max);
    ASSERTF(!minIsZero || v >= 0, "v: %f", v);
    ASSERTF(minIsZero || v >= -max, "v: %f, -max: %f", v, -max);

    float scaled = 0.0f;
    if (minIsZero)
    {
        scaled = v / max;
    }
    else
    {
        scaled = (v + max) / (max * 2.0f);
    }
    return fmaxf(fminf(scaled, max), 0.0f);
}

#define BITNSLOTS(nb) ((nb + sizeof(uint8_t) - 1) / sizeof(uint8_t))

static inline uint16_t bitMask(const uint16_t n)
{
    return 1 << (n % sizeof(uint8_t));
}

static inline uint16_t bitSlot(const uint16_t n)
{
    return n / sizeof(uint8_t);
}

static inline void bitSet(uint8_t *b, const uint16_t n)
{
    b[bitSlot(n)] |= bitMask(n);
}

static inline bool bitTest(const uint8_t *b, const uint16_t n)
{
    return b[bitSlot(n)] & bitMask(n);
}
