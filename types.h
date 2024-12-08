#pragma once

#include "box2d/box2d.h"

#include "include/cc_deque.h"
#include "include/cc_slist.h"

#include "settings.h"

enum entityType
{
    STANDARD_WALL_ENTITY,
    BOUNCY_WALL_ENTITY,
    DEATH_WALL_ENTITY,
    WEAPON_PICKUP_ENTITY,
    PROJECTILE_ENTITY,
    DRONE_ENTITY,
};

enum shapeCategory
{
    WALL_SHAPE = 1,
    FLOATING_WALL_SHAPE = 2,
    PROJECTILE_SHAPE = 4,
    WEAPON_PICKUP_SHAPE = 8,
    DRONE_SHAPE = 16,
};

typedef struct entity
{
    enum entityType type;
    void *entity;
} entity;

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

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
};

typedef struct weaponPickupEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    float respawnWait;
    uint8_t floatingWallsTouching;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

typedef struct projectileEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType type;
    b2Vec2 lastPos;
    float distance;
    uint8_t bounces;
} projectileEntity;

typedef struct droneEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    enum weaponType weapon;
    int8_t ammo;
    float weaponCooldown;
    bool shotThisStep;
    uint16_t heat;
    uint16_t charge;
    b2Vec2 lastAim;
    bool dead;
} droneEntity;

typedef struct env
{
    b2WorldId worldID;
    mapBounds bounds;
    CC_Deque *entities;
    CC_Deque *walls;
    CC_Deque *emptyCells;
    CC_Deque *drones;
    CC_Deque *pickups;
    CC_SList *projectiles;

} env;

typedef struct droneInputs
{
    b2Vec2 move;
    b2Vec2 aim;
    bool shoot;
} droneInputs;