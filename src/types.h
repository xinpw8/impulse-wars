#pragma once
#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "box2d/box2d.h"

#include "include/cc_deque.h"
#include "include/cc_slist.h"

#include "settings.h"

#define NUM_ENTITY_TYPES 6
#define NUM_WALL_TYPES 3

enum entityType
{
    INVALID_ENTITY,
    STANDARD_WALL_ENTITY,
    BOUNCY_WALL_ENTITY,
    DEATH_WALL_ENTITY,
    PROJECTILE_ENTITY,
    DRONE_ENTITY,
    // this needs to be last so map cell type observations can be
    // calculated correctly
    WEAPON_PICKUP_ENTITY,
};

// the category bit that will be set on each entity's shape; this is
// used to control what entities can collide with each other
enum shapeCategory
{
    WALL_SHAPE = 1,
    FLOATING_WALL_SHAPE = 2,
    PROJECTILE_SHAPE = 4,
    WEAPON_PICKUP_SHAPE = 8,
    DRONE_SHAPE = 16,
};

// general purpose entity object
typedef struct entity
{
    enum entityType type;
    void *entity;
} entity;

#define NUM_WEAPONS 5

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
};

typedef struct mapEntry
{
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t floatingStandardWalls;
    const uint8_t floatingBouncyWalls;
    const uint8_t floatingDeathWalls;
    const uint16_t weaponPickups;
    const enum weaponType defaultWeapon;
} mapEntry;

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell
{
    entity *ent;
    b2Vec2 pos;
} mapCell;

typedef struct mapBounds
{
    b2Vec2 min;
    b2Vec2 max;
} mapBounds;

typedef struct wallEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    b2Vec2 position;
    b2Vec2 extent;
    bool isFloating;
    enum entityType type;
} wallEntity;

typedef struct weaponInformation
{
    const enum weaponType type;
    const bool isPhysicsBullet;
    const uint8_t numProjectiles;
    const float recoilMagnitude;
    const float coolDown;
    const float maxDistance;
    const float radius;
    const float density;
    const float invMass;
    const uint8_t maxBounces;
} weaponInformation;

typedef struct weaponPickupEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    float respawnWait;
    uint8_t floatingWallsTouching;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

// TODO: add drone index
typedef struct projectileEntity
{
    uint8_t droneIdx;

    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    b2Vec2 lastPos;
    float distance;
    uint8_t bounces;
} projectileEntity;

typedef struct stepHitInfo
{
    bool shotHit;
    bool explosionHit;
} stepHitInfo;

typedef struct droneStats
{
    uint16_t shotsFired[NUM_WEAPONS];
    uint16_t shotsHit[NUM_WEAPONS];
    uint16_t shotsTaken[NUM_WEAPONS];
} droneStats;

typedef struct droneEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    int8_t ammo;
    float weaponCooldown;
    uint16_t heat;
    uint16_t charge;
    bool shotThisStep;

    uint8_t idx;
    b2Vec2 lastAim;
    b2Vec2 lastVelocity;
    stepHitInfo hitInfo;
    bool dead;
} droneEntity;

typedef struct env
{
    float *obs;
    float *rewards;
    float *actions;
    unsigned char *terminals;

    uint64_t randState;
    bool needsReset;

    b2WorldId worldID;
    uint8_t columns;
    uint8_t rows;
    mapBounds bounds;
    weaponInformation *defaultWeapon;
    CC_Deque *cells;
    CC_Deque *walls;
    CC_Deque *floatingWalls;
    CC_Deque *drones;
    CC_Deque *pickups;
    CC_SList *projectiles;

    // used for rendering explosions
    // TODO: use hitInfo
    uint8_t explosionSteps;
    b2ExplosionDef explosion;

    // steps left until sudden death
    uint16_t stepsLeft;
    // steps left until the next set of sudden death walls are spawned
    uint16_t suddenDeathSteps;
    // the amount of sudden death walls that have been spawned
    uint8_t suddenDeathWallCounter;
} env;

#endif
