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

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell
{
    entity *ent;
    // should only be set if the entity is NULL
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

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
};

typedef struct weaponInformation
{
    enum weaponType type;
    uint8_t numProjectiles;
    float recoilMagnitude;
    float coolDown;
    float maxDistance;
    float radius;
    float density;
    float invMass;
    uint8_t maxBounces;
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

typedef struct projectileEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
    b2Vec2 lastPos;
    float distance;
    uint8_t bounces;
} projectileEntity;

typedef struct droneEntity
{
    b2BodyId bodyID;
    b2ShapeId shapeID;
    weaponInformation *weaponInfo;
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
    uint8_t columns;
    uint8_t rows;
    mapBounds bounds;
    CC_Deque *cells;
    CC_Deque *walls;
    CC_Deque *entities;
    CC_Deque *drones;
    CC_Deque *pickups;
    CC_SList *projectiles;

    uint8_t explosionSteps;
    b2ExplosionDef explosion;

    // steps left until sudden death
    uint16_t stepsLeft;
    // steps left until the next set of sudden death walls are spawned
    uint16_t suddenDeathSteps;
    // the amount of sudden death walls that have been spawned
    uint8_t suddenDeathWallCounter;
} env;

typedef struct droneInputs
{
    b2Vec2 move;
    b2Vec2 aim;
    bool shoot;
} droneInputs;