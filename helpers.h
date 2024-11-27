#ifndef HELPERS_H
#define HELPERS_H

#include "box2d/box2d.h"
#include "raylib.h"

#include <assert.h>
#include <stdio.h>

const float scale = 50.0f;

#define ERRORF(fmt, args...)                                                    \
    fprintf(stderr, " %s:%s:%d\n" fmt, __FILE__, __FUNCTION__, __LINE__, args); \
    exit(1)
#define ERROR(msg) ERRORF(msg, NULL)

#ifndef NDEBUG
#define DEBUG_LOGF(fmt, args...)                                                                                                           \
    {                                                                                                                                      \
        time_t t = time(NULL);                                                                                                             \
        struct tm *timeinfo;                                                                                                               \
        timeinfo = localtime(&t);                                                                                                          \
        printf(" %d:%d:%d %s:%s:%d\n" fmt, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, __FILE__, __FUNCTION__, __LINE__, args); \
        fflush(stdout);                                                                                                                    \
    }
#define DEBUG_LOG(msg) DEBUG_LOGF(msg, NULL)
#else
#define DEBUG_LOGF(fmt, args...)
#define DEBUG_LOG(msg)
#endif

#define INV_MASS(density, radius) (1.0f / (density * PI * radius * radius))

#define ASSERT_VEC(vec, min, max) \
    assert(vec.x <= max);         \
    assert(vec.x >= min);         \
    assert(vec.y <= max);         \
    assert(vec.y >= min);

b2Vec2 createb2Vec(const float x, const float y)
{
    return (b2Vec2){.x = x, .y = y};
}

bool b2VecEqual(const b2Vec2 v1, const b2Vec2 v2)
{
    return v1.x == v2.x && v1.y == v2.y;
}

float randFloat(float min, float max)
{
    float n = rand() / (float)RAND_MAX;
    return min + n * (max - min);
}

int randInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

#endif