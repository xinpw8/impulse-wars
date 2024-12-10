#pragma once

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "box2d/box2d.h"
#include "raylib.h"

#define ERRORF(fmt, args...)                                                    \
    fprintf(stderr, " %s:%s:%d\n" fmt, __FILE__, __FUNCTION__, __LINE__, args); \
    exit(1)
#define ERROR(msg) ERRORF(msg, NULL)

#ifndef NDEBUG
#define DEBUG_LOGF(fmt, args...)                                                                                                              \
    {                                                                                                                                         \
        time_t _t = time(NULL);                                                                                                               \
        struct tm *_timeinfo;                                                                                                                 \
        _timeinfo = localtime(&_t);                                                                                                           \
        printf(" %d:%d:%d %s:%s:%d\n" fmt, _timeinfo->tm_hour, _timeinfo->tm_min, _timeinfo->tm_sec, __FILE__, __FUNCTION__, __LINE__, args); \
        fflush(stdout);                                                                                                                       \
    }
#define DEBUG_LOG(msg) DEBUG_LOGF(msg, NULL)
#else
#define DEBUG_LOGF(fmt, args...)
#define DEBUG_LOG(msg)
#endif

#define INV_MASS(density, radius) (1.0f / (density * PI * radius * radius))

#define ASSERT_VEC_NORMALIZED(vec) \
    assert(vec.x <= 1.0f);         \
    assert(vec.x >= -1.0f);        \
    assert(vec.y <= 1.0f);         \
    assert(vec.y >= -1.0f);

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
