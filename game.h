#ifndef GAME_H
#define GAME_H

#include "box2d/box2d.h"
#include "raylib.h"

#include "include/cc_deque.h"
#include "include/cc_slist.h"

#include "helpers.h"
#include "settings.h"

#include <assert.h>
#include <time.h>
#include <stdio.h>

// the result of b2MakeRot(0.0f) since I can't call that here
const b2Rot zeroB2Rot = {.c = 1.0f, .s = 0.0f};

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
    WALL_SHAPE = 0x1,
    PROJECTILE_SHAPE = 0x2,
    DRONE_SHAPE = 0x4,
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

wallEntity *createWall(const b2WorldId worldID, CC_Deque *entities, const float posX, const float posY, const float width, const float height, const enum entityType type, bool floating)
{
    assert(type != DRONE_ENTITY);
    assert(type != PROJECTILE_ENTITY);

    b2BodyDef wallBodyDef = b2DefaultBodyDef();
    wallBodyDef.position = createb2Vec(posX, posY);
    if (floating)
    {
        wallBodyDef.type = b2_dynamicBody;
        wallBodyDef.linearDamping = FLOATING_WALL_DAMPING;
        wallBodyDef.angularDamping = FLOATING_WALL_DAMPING;
    }
    b2BodyId wallBodyID = b2CreateBody(worldID, &wallBodyDef);
    b2Vec2 extent = {.x = width / 2.0f, .y = height / 2.0f};
    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    wallShapeDef.density = WALL_DENSITY;
    wallShapeDef.restitution = 0.0f;
    wallShapeDef.filter.categoryBits = WALL_SHAPE;
    wallShapeDef.filter.maskBits = WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;

    if (type == BOUNCY_WALL_ENTITY)
    {
        wallShapeDef.restitution = BOUNCY_WALL_RESTITUTION;
        wallShapeDef.enableSensorEvents = true;
    }
    if (type == DEATH_WALL_ENTITY)
    {
        wallShapeDef.enableContactEvents = true;
    }

    wallEntity *wall = (wallEntity *)calloc(1, sizeof(wallEntity));
    wall->bodyID = wallBodyID;
    if (!floating)
    {
        wall->position = wallBodyDef.position;
    }
    wall->extent = extent;
    wall->isFloating = floating;
    wall->type = type;

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = type;
    e->entity = wall;
    if (floating)
    {
        cc_deque_add(entities, e);
    }

    wallShapeDef.userData = e;
    const b2Polygon wallPolygon = b2MakeBox(extent.x, extent.y);
    wall->shapeID = b2CreatePolygonShape(wallBodyID, &wallShapeDef, &wallPolygon);

    return wall;
}

void destroyWall(const b2WorldId worldID, wallEntity *wall)
{
    assert(b2World_IsValid(worldID));
    assert(wall != NULL);

    entity *e = (entity *)b2Shape_GetUserData(wall->shapeID);
    free(e);

    b2DestroyBody(wall->bodyID);
    free(wall);
}

b2DistanceProxy makeDistanceProxy(const enum entityType type, bool *isCircle)
{
    b2DistanceProxy proxy = {0};
    switch (type)
    {
    case DRONE_ENTITY:
        *isCircle = true;
        proxy.radius = DRONE_RADIUS;
        break;
    case WEAPON_PICKUP_ENTITY:
        proxy.count = 4;
        proxy.points[0] = (b2Vec2){.x = -PICKUP_THICKNESS / 2.0f, .y = -PICKUP_THICKNESS / 2.0f};
        proxy.points[1] = (b2Vec2){.x = -PICKUP_THICKNESS / 2.0f, .y = +PICKUP_THICKNESS / 2.0f};
        proxy.points[2] = (b2Vec2){.x = +PICKUP_THICKNESS / 2.0f, .y = -PICKUP_THICKNESS / 2.0f};
        proxy.points[3] = (b2Vec2){.x = +PICKUP_THICKNESS / 2.0f, .y = +PICKUP_THICKNESS / 2.0f};
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        proxy.count = 4;
        proxy.points[0] = (b2Vec2){.x = -FLOATING_WALL_THICKNESS / 2.0f, .y = -FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[1] = (b2Vec2){.x = -FLOATING_WALL_THICKNESS / 2.0f, .y = +FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[2] = (b2Vec2){.x = +FLOATING_WALL_THICKNESS / 2.0f, .y = -FLOATING_WALL_THICKNESS / 2.0f};
        proxy.points[3] = (b2Vec2){.x = +FLOATING_WALL_THICKNESS / 2.0f, .y = +FLOATING_WALL_THICKNESS / 2.0f};
        break;
    default:
        ERRORF("unknown entity type for shape distance: %d", type);
    }

    return proxy;
}

b2Vec2 findOpenPos(const CC_Deque *entities, const CC_Deque *emptyCells, const enum entityType type)
{
    assert(entities != NULL);
    assert(emptyCells != NULL);

    const size_t nEmptyCells = cc_deque_size(emptyCells) - 1;
    assert(nEmptyCells > 0);
    const size_t nEntities = cc_deque_size(entities);
    b2DistanceCache cache = {0};
    while (true)
    {
        const int cellIdx = randInt(0, nEmptyCells);
        b2Vec2 *pos;
        cc_deque_get_at(emptyCells, cellIdx, (void **)&pos);

        bool found = true;
        bool shapeACircle = false;
        const b2DistanceProxy proxyA = makeDistanceProxy(type, &shapeACircle);
        for (int i = 0; i < nEntities; i++)
        {
            entity *e;
            cc_deque_get_at(entities, i, (void **)&e);
            b2Transform transform;
            switch (e->type)
            {
            case DRONE_ENTITY:
                const droneEntity *de = (droneEntity *)e->entity;
                transform = b2Body_GetTransform(de->bodyID);
                break;
            case WEAPON_PICKUP_ENTITY:
                const weaponPickupEntity *pe = (weaponPickupEntity *)e->entity;
                transform = b2Body_GetTransform(pe->bodyID);
                break;
            case STANDARD_WALL_ENTITY:
            case BOUNCY_WALL_ENTITY:
            case DEATH_WALL_ENTITY:
                const wallEntity *we = (wallEntity *)e->entity;
                transform = b2Body_GetTransform(we->bodyID);
                break;
            default:
                ERRORF("unknown entity type for shape distance: %d", type);
            }

            float distance = b2Distance(*pos, transform.p);
            if (distance < 7.5f)
            {
                found = false;
                break;
            }

            bool shapeBCircle = false;
            const b2DistanceProxy proxyB = makeDistanceProxy(e->type, &shapeBCircle);
            const b2DistanceInput input = {
                .proxyA = proxyA,
                .proxyB = proxyB,
                .transformA = {.p = *pos, .q = zeroB2Rot},
                .transformB = transform,
                .useRadii = shapeACircle || shapeBCircle,
            };
            const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);
            DEBUG_LOGF("cellIdx=%d i=%d type=%d posA=(%f, %f) posB=(%f, %f) distance=%f shapeDistance=%f", cellIdx, i, e->type, pos->x, pos->y, transform.p.x, transform.p.y, distance, output.distance);
            if (output.distance < 7.5)
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            return *pos;
        }
    }

    ERRORF("couldn't find empty position for entity %d", type);
}

weaponPickupEntity *createWeaponPickup(const b2WorldId worldID, CC_Deque *entities, const CC_Deque *emptyCells, const enum weaponType type)
{
    assert(b2World_IsValid(worldID));
    assert(emptyCells != NULL);
    assert(type != STANDARD_WEAPON);

    b2BodyDef pickupBodyDef = b2DefaultBodyDef();
    pickupBodyDef.position = findOpenPos(entities, emptyCells, WEAPON_PICKUP_ENTITY);
    b2BodyId pickupBodyID = b2CreateBody(worldID, &pickupBodyDef);
    b2ShapeDef pickupShapeDef = b2DefaultShapeDef();
    pickupShapeDef.filter.categoryBits = WEAPON_PICKUP_ENTITY;
    pickupShapeDef.filter.maskBits = WALL_SHAPE | DRONE_SHAPE;
    pickupShapeDef.isSensor = true;

    weaponPickupEntity *pickup = (weaponPickupEntity *)calloc(1, sizeof(weaponPickupEntity));
    pickup->bodyID = pickupBodyID;
    pickup->weapon = type;
    pickup->respawnWait = 0.0f;
    pickup->floatingWallsTouching = 0;

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = WEAPON_PICKUP_ENTITY;
    e->entity = pickup;
    cc_deque_add(entities, e);

    pickupShapeDef.userData = e;
    const b2Polygon pickupPolygon = b2MakeBox(PICKUP_THICKNESS / 2.0f, PICKUP_THICKNESS / 2.0f);
    pickup->shapeID = b2CreatePolygonShape(pickupBodyID, &pickupShapeDef, &pickupPolygon);

    return pickup;
}

void destroyWeaponPickup(weaponPickupEntity *pickup)
{
    assert(pickup != NULL);

    entity *e = (entity *)b2Shape_GetUserData(pickup->shapeID);
    free(e);

    b2DestroyBody(pickup->bodyID);
    free(pickup);
}

droneEntity *createDrone(const b2WorldId worldID, CC_Deque *entities, const CC_Deque *emptyCells)
{
    assert(b2World_IsValid(worldID));
    assert(entities != NULL);
    assert(emptyCells != NULL);

    b2BodyDef droneBodyDef = b2DefaultBodyDef();
    droneBodyDef.type = b2_dynamicBody;
    droneBodyDef.position = findOpenPos(entities, emptyCells, DRONE_ENTITY);
    droneBodyDef.linearDamping = DRONE_LINEAR_DAMPING;
    b2BodyId droneBodyID = b2CreateBody(worldID, &droneBodyDef);
    b2ShapeDef droneShapeDef = b2DefaultShapeDef();
    droneShapeDef.density = DRONE_DENSITY;
    droneShapeDef.friction = 0.0f;
    droneShapeDef.restitution = 0.2f;
    droneShapeDef.filter.categoryBits = DRONE_SHAPE;
    droneShapeDef.filter.maskBits = WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    droneShapeDef.enableContactEvents = true;
    droneShapeDef.enableSensorEvents = true;
    const b2Circle droneCircle = {.center = {.x = 0.0f, .y = 0.0f}, .radius = DRONE_RADIUS};

    droneEntity *drone = (droneEntity *)calloc(1, sizeof(droneEntity));
    drone->bodyID = droneBodyID;
    drone->weapon = DRONE_DEFAULT_WEAPON;
    drone->ammo = weaponAmmo(drone->weapon);
    drone->shotThisStep = false;
    drone->lastAim = (b2Vec2){.x = 0.0f, .y = -1.0f};

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = DRONE_ENTITY;
    e->entity = drone;
    cc_deque_add(entities, e);

    droneShapeDef.userData = e;
    drone->shapeID = b2CreateCircleShape(droneBodyID, &droneShapeDef, &droneCircle);

    return drone;
}

void destroyDrone(droneEntity *drone)
{
    assert(drone != NULL);

    entity *e = (entity *)b2Shape_GetUserData(drone->shapeID);
    free(e);

    b2DestroyBody(drone->bodyID);
    free(drone);
}

void droneMove(const droneEntity *drone, const b2Vec2 direction)
{
    assert(drone != NULL);
    ASSERT_VEC(direction, -1.0f, 1.0f);

    b2Vec2 force = b2MulSV(DRONE_MOVE_MAGNITUDE, direction);
    b2Body_ApplyForceToCenter(drone->bodyID, force, true);
}

void createProjectile(const b2WorldId worldID, CC_SList *projectiles, droneEntity *drone, const b2Vec2 normAim)
{
    assert(b2World_IsValid(worldID));
    assert(projectiles != NULL);
    assert(drone != NULL);
    ASSERT_VEC(normAim, -1.0f, 1.0f);

    b2BodyDef projectileBodyDef = b2DefaultBodyDef();
    projectileBodyDef.type = b2_dynamicBody;
    projectileBodyDef.fixedRotation = true;
    projectileBodyDef.isBullet = true;
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    float radius = weaponRadius(drone->weapon);
    projectileBodyDef.position = b2MulAdd(dronePos, 1.0f + (radius * 1.5f), normAim);
    b2BodyId projectileBodyID = b2CreateBody(worldID, &projectileBodyDef);
    b2ShapeDef projectileShapeDef = b2DefaultShapeDef();
    projectileShapeDef.enableContactEvents = true;
    projectileShapeDef.density = weaponDensity(drone->weapon);
    projectileShapeDef.friction = 0.0f;
    projectileShapeDef.restitution = 1.0f;
    projectileShapeDef.filter.categoryBits = PROJECTILE_SHAPE;
    projectileShapeDef.filter.maskBits = WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    const b2Circle projectileCircle = {.center = {.x = 0.0f, .y = 0.0f}, .radius = radius};

    b2ShapeId projectileShapeID = b2CreateCircleShape(projectileBodyID, &projectileShapeDef, &projectileCircle);

    // add lateral drone velocity to projectile
    b2Vec2 droneVel = b2Body_GetLinearVelocity(drone->bodyID);
    b2Vec2 forwardVel = b2MulSV(b2Dot(droneVel, normAim), normAim);
    b2Vec2 lateralVel = b2Sub(droneVel, forwardVel);
    lateralVel = b2MulSV(projectileShapeDef.density / DRONE_MOVE_AIM_DIVISOR, lateralVel);
    b2Vec2 aim = weaponAdjustAim(drone->weapon, drone->heat, normAim);
    b2Vec2 fire = b2MulAdd(lateralVel, weaponFire(drone->weapon), aim);
    b2Body_ApplyLinearImpulseToCenter(projectileBodyID, fire, true);

    projectileEntity *projectile = (projectileEntity *)calloc(1, sizeof(projectileEntity));
    projectile->bodyID = projectileBodyID;
    projectile->shapeID = projectileShapeID;
    projectile->type = drone->weapon;
    projectile->lastPos = projectileBodyDef.position;
    cc_slist_add(projectiles, projectile);

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = PROJECTILE_ENTITY;
    e->entity = projectile;

    b2Shape_SetUserData(projectile->shapeID, e);
}

void destroyProjectile(CC_SList *projectiles, projectileEntity *projectile, bool full)
{
    assert(projectile != NULL);

    entity *e = (entity *)b2Shape_GetUserData(projectile->shapeID);
    free(e);

    if (full)
    {
        cc_slist_remove(projectiles, projectile, NULL);
        b2DestroyBody(projectile->bodyID);
    }

    free(projectile);
}

void destroyAllProjectiles(CC_SList *projectiles)
{
    for (SNode *cur = projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        destroyProjectile(projectiles, projectile, false);
    }
}

void droneChangeWeapon(droneEntity *drone, const enum weaponType weapon)
{
    drone->weapon = weapon;
    drone->ammo = weaponAmmo(weapon);
    drone->weaponCooldown = 0.0f;
    drone->charge = 0;
    drone->heat = 0;
}

void droneShoot(const b2WorldId worldID, CC_SList *projectiles, droneEntity *drone, const b2Vec2 aim)
{
    assert(b2World_IsValid(worldID));
    assert(drone != NULL);
    assert(drone->ammo != 0);

    drone->shotThisStep = true;
    drone->charge++;
    drone->heat++;
    if (drone->weaponCooldown != 0.0f || drone->charge < weaponCharge(drone->weapon))
    {
        return;
    }
    if (drone->weapon != STANDARD_WEAPON && drone->ammo != INFINITE)
    {
        drone->ammo--;
    }
    drone->weaponCooldown = weaponCoolDown(drone->weapon);

    b2Vec2 normAim = drone->lastAim;
    if (!b2VecEqual(aim, b2Vec2_zero))
    {
        normAim = b2Normalize(aim);
    }
    b2Vec2 recoil = b2MulSV(-weaponRecoil(drone->weapon), normAim);
    b2Body_ApplyLinearImpulseToCenter(drone->bodyID, recoil, true);

    for (int i = 0; i < weaponProjectiles(drone->weapon); i++)
    {
        createProjectile(worldID, projectiles, drone, normAim);
    }

    if (drone->ammo == 0)
    {
        droneChangeWeapon(drone, DRONE_DEFAULT_WEAPON);
        drone->weaponCooldown = weaponCoolDown(DRONE_DEFAULT_WEAPON);
    }
}

void droneStep(droneEntity *drone, const float frameTime)
{
    assert(drone != NULL);
    assert(frameTime != 0.0f);

    drone->weaponCooldown = fmaxf(drone->weaponCooldown - frameTime, 0.0f);
    if (!drone->shotThisStep)
    {
        drone->charge = 0;
        drone->heat = fmaxf(drone->heat - 1, 0);
    }
    else
    {
        drone->shotThisStep = false;
    }
    assert(drone->shotThisStep == false);
}

void projectilesStep(CC_SList *projectiles)
{
    CC_SListIter iter;
    cc_slist_iter_init(&iter, projectiles);
    projectileEntity *projectile;
    while (cc_slist_iter_next(&iter, (void **)&projectile) != CC_ITER_END)
    {
        const float maxDistance = weaponMaxDistance(projectile->type);
        if (maxDistance == INFINITE)
        {
            continue;
        }
        const b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        const b2Vec2 distance = b2Sub(pos, projectile->lastPos);
        projectile->distance += b2Length(distance);
        if (projectile->distance >= maxDistance)
        {
            cc_slist_iter_remove(&iter, NULL);
            destroyProjectile(projectiles, projectile, true);
            continue;
        }

        projectile->lastPos = pos;
    }
}

void weaponPickupStep(const CC_Deque *entities, const CC_Deque *emptyCells, weaponPickupEntity *pickup, const float frameTime)
{
    assert(emptyCells != NULL);
    assert(pickup != NULL);
    assert(frameTime != 0.0f);

    if (pickup->respawnWait != 0.0f)
    {
        pickup->respawnWait = fmaxf(pickup->respawnWait - frameTime, 0.0f);
        if (pickup->respawnWait == 0.0f)
        {
            b2Vec2 pos = findOpenPos(entities, emptyCells, WEAPON_PICKUP_ENTITY);
            b2Body_SetTransform(pickup->bodyID, pos, b2MakeRot(0.0f));
        }
    }
}

bool handleProjectileBeginContact(CC_SList *projectiles, const entity *p, const entity *e)
{
    assert(projectiles != NULL);
    assert(p != NULL);

    projectileEntity *projectile = (projectileEntity *)p->entity;
    // e (shape B in the collision) will be NULL if it's another
    // projectile that was just destroyed
    if (e == NULL || (e != NULL && e->type == PROJECTILE_ENTITY))
    {
        // always allow projectiles to bounce off each other
        return false;
    }
    else if (e != NULL && e->type == BOUNCY_WALL_ENTITY)
    {
        // always allow projectiles to bounce off bouncy walls
        return false;
    }
    else if (e != NULL && e->type != BOUNCY_WALL_ENTITY)
    {
        projectile->bounces++;
    }
    const uint8_t maxBounces = weaponBounce(projectile->type);
    if (projectile->bounces == maxBounces)
    {
        destroyProjectile(projectiles, projectile, true);
        return true;
    }

    return false;
}

void handleProjectileEndContact(CC_SList *projectiles, const entity *p)
{
    assert(projectiles != NULL);
    assert(p != NULL);

    projectileEntity *projectile = (projectileEntity *)p->entity;
    b2Vec2 velocity = b2Body_GetLinearVelocity(projectile->bodyID);
    b2Vec2 newVel = b2MulSV(weaponFire(projectile->type) * weaponInvMass(projectile->type), b2Normalize(velocity));
    b2Body_SetLinearVelocity(projectile->bodyID, newVel);
}

void handleContactEvents(const b2WorldId worldID, CC_SList *projectiles)
{
    assert(b2World_IsValid(worldID));
    assert(projectiles != NULL);

    b2ContactEvents events = b2World_GetContactEvents(worldID);
    for (int i = 0; i < events.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent *event = events.beginEvents + i;
        entity *e1 = NULL;
        entity *e2 = NULL;

        if (b2Shape_IsValid(event->shapeIdA))
        {
            e1 = (entity *)b2Shape_GetUserData(event->shapeIdA);
            assert(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB))
        {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            assert(e2 != NULL);
        }

        if (e1 != NULL)
        {
            if (e1->type == PROJECTILE_ENTITY)
            {
                if (handleProjectileBeginContact(projectiles, e1, e2))
                {
                    e1 = NULL;
                }
            }
            else if (e1->type == DEATH_WALL_ENTITY && e2 != NULL && e2->type == DRONE_ENTITY)
            {
                droneEntity *drone = (droneEntity *)e2->entity;
                drone->dead = true;
            }
        }
        if (e2 != NULL)
        {
            if (e2->type == PROJECTILE_ENTITY)
            {
                handleProjectileBeginContact(projectiles, e2, e1);
            }
            else if (e2->type == DEATH_WALL_ENTITY && e1 != NULL && e1->type == DRONE_ENTITY)
            {
                droneEntity *drone = (droneEntity *)e1->entity;
                drone->dead = true;
            }
        }
    }

    for (int i = 0; i < events.endCount; ++i)
    {
        const b2ContactEndTouchEvent *event = events.endEvents + i;
        entity *e1 = NULL;
        entity *e2 = NULL;

        if (b2Shape_IsValid(event->shapeIdA))
        {
            e1 = (entity *)b2Shape_GetUserData(event->shapeIdA);
            assert(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB))
        {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            assert(e2 != NULL);
        }

        if (e1 != NULL && e1->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(projectiles, e1);
        }
        if (e2 != NULL && e2->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(projectiles, e2);
        }
    }
}

void handleWeaponPickupBeginTouch(const b2WorldId worldID, const entity *s, entity *v)
{
    assert(b2World_IsValid(worldID));
    assert(s != NULL);
    assert(v != NULL);

    weaponPickupEntity *pickup = (weaponPickupEntity *)s->entity;
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0)
    {
        return;
    }

    switch (v->type)
    {
    case DRONE_ENTITY:
        pickup->respawnWait = PICKUP_RESPAWN_WAIT;
        droneEntity *drone = (droneEntity *)v->entity;
        droneChangeWeapon(drone, pickup->weapon);
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching++;
        break;
    default:
        ERRORF("invalid weapon pickup begin touch visitor %d", v->type);
    }
}

void handleWeaponPickupEndTouch(const b2WorldId worldID, const entity *s, entity *v)
{
    assert(b2World_IsValid(worldID));
    assert(s != NULL);
    assert(v != NULL);

    weaponPickupEntity *pickup = (weaponPickupEntity *)s->entity;
    if (pickup->respawnWait != 0.0f)
    {
        return;
    }

    switch (v->type)
    {
    case DRONE_ENTITY:
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching--;
        break;
    default:
        ERRORF("invalid weapon pickup end touch visitor %d", v->type);
    }
}

void handleSensorEvents(const b2WorldId worldID)
{
    assert(b2World_IsValid(worldID));

    b2SensorEvents events = b2World_GetSensorEvents(worldID);
    for (int i = 0; i < events.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent *event = events.beginEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId))
        {
            ERROR("could not find sensor shape for begin touch event");
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        assert(s != NULL);
        assert(s->type == WEAPON_PICKUP_ENTITY);

        if (!b2Shape_IsValid(event->visitorShapeId))
        {
            ERROR("could not find visitor shape for begin touch event");
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        assert(v != NULL);

        handleWeaponPickupBeginTouch(worldID, s, v);
    }

    for (int i = 0; i < events.endCount; ++i)
    {
        const b2SensorEndTouchEvent *event = events.endEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId))
        {
            ERROR("could not find sensor shape for end touch event");
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        assert(s != NULL);
        assert(s->type == WEAPON_PICKUP_ENTITY);

        if (!b2Shape_IsValid(event->visitorShapeId))
        {
            ERROR("could not find visitor shape for end touch event");
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        assert(v != NULL);

        handleWeaponPickupEndTouch(worldID, s, v);
    }
}

#endif
