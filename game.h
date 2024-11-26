#ifndef GAME_H
#define GAME_H

#include "box2d/box2d.h"
#include "raylib.h"

#include "include/cc_slist.h"
#include "include/hashmap.h"

#include "helpers.h"
#include "settings.h"

#include <assert.h>
#include <time.h>
#include <stdio.h>

enum entityType
{
    STANDARD_WALL,
    FLOATING_STANDARD_WALL,
    BOUNCY_WALL,
    FLOATING_BOUNCY_WALL,
    DEATH_WALL,
    FLOATING_DEATH_WALL,
    PROJECTILE_ENTITY,
    DRONE_ENTITY,
};

typedef struct entity
{
    enum entityType type;
    void *entity;
} entity;

typedef struct wallEntity
{
    b2BodyId bodyID;
    b2ShapeId *shapeID;
    b2Vec2 extent;
    enum entityType type;
} wallEntity;

typedef struct droneEntity droneEntity;

typedef struct projectileEntity
{
    b2BodyId bodyID;
    b2ShapeId *shapeID;
    enum weaponType type;
    b2Vec2 lastPos;
    float distance;
    uint8_t bounces;
    uint8_t bouncyWallBounces;
} projectileEntity;

typedef struct droneEntity
{
    b2BodyId bodyID;
    b2ShapeId *shapeID;
    enum weaponType weapon;
    int8_t ammo;
    float weaponCooldown;
    bool shotThisStep;
    uint16_t triggerHeldSteps;
    b2Vec2 lastAim;
} droneEntity;

wallEntity *createWall(const b2WorldId worldID, struct hashmap_s *shapeMap, const float posX, const float posY, const float width, const float height, const enum entityType type)
{
    assert(shapeMap != NULL);
    assert(type != DRONE_ENTITY);
    assert(type != PROJECTILE_ENTITY);

    b2BodyDef wallBodyDef = b2DefaultBodyDef();
    wallBodyDef.position = createb2Vec(posX, posY);
    b2BodyId wallBodyID = b2CreateBody(worldID, &wallBodyDef);
    b2Vec2 extent = {.x = width / 2.0f, .y = height / 2.0f};
    b2Polygon bouncyWall = b2MakeBox(extent.x, extent.y);
    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    wallShapeDef.restitution = 0.0f;

    b2ShapeId *wallShapeID = (b2ShapeId *)calloc(1, sizeof(b2ShapeId));
    *wallShapeID = b2CreatePolygonShape(wallBodyID, &wallShapeDef, &bouncyWall);

    wallEntity *wall = (wallEntity *)calloc(1, sizeof(wallEntity));
    wall->bodyID = wallBodyID;
    wall->shapeID = wallShapeID;
    wall->extent = extent;
    wall->type = type;

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = type;
    e->entity = wall;

    if (0 != hashmap_put(shapeMap, wallShapeID, sizeof(wallShapeID), e))
    {
        ERROR("error adding wall shape to hashmap");
    }
    return wall;
}

droneEntity *createDrone(const b2WorldId worldID, struct hashmap_s *shapeMap, const float posX, const float posY)
{
    assert(shapeMap != NULL);

    b2BodyDef droneBodyDef = b2DefaultBodyDef();
    droneBodyDef.type = b2_dynamicBody;
    droneBodyDef.position = createb2Vec(posX, posY);
    b2BodyId droneBodyID = b2CreateBody(worldID, &droneBodyDef);
    b2ShapeDef droneShapeDef = b2DefaultShapeDef();
    droneShapeDef.enableContactEvents = true;
    droneShapeDef.density = DRONE_DENSITY;
    droneShapeDef.friction = 0.0f;
    droneShapeDef.restitution = 0.3f;
    b2Circle droneCircle = {.center = {.x = 0.0f, .y = 0.0f}, .radius = DRONE_RADIUS};

    b2ShapeId *droneShapeID = (b2ShapeId *)calloc(1, sizeof(b2ShapeId));
    *droneShapeID = b2CreateCircleShape(droneBodyID, &droneShapeDef, &droneCircle);

    droneEntity *drone = (droneEntity *)calloc(1, sizeof(droneEntity));
    drone->bodyID = droneBodyID;
    drone->shapeID = droneShapeID;
    drone->weapon = DRONE_DEFAULT_WEAPON;
    drone->ammo = weaponAmmo(drone->weapon);
    drone->shotThisStep = false;
    drone->lastAim = (b2Vec2){.x = 0.0f, .y = -1.0f};

    entity *e = (entity *)calloc(1, sizeof(entity));
    e->type = DRONE_ENTITY;
    e->entity = drone;

    if (0 != hashmap_put(shapeMap, droneShapeID, sizeof(droneShapeID), e))
    {
        ERROR("error adding drone shape to hashmap");
    }
    return drone;
}

void destroyDrone(struct hashmap_s *shapeMap, droneEntity *drone)
{
    assert(shapeMap != NULL);
    assert(drone != NULL);

    entity *e = (entity *)hashmap_get(shapeMap, drone->shapeID, sizeof(drone->shapeID));
    SAFE_FREE(e);
    hashmap_remove(shapeMap, drone->shapeID, sizeof(drone->shapeID));

    b2DestroyBody(drone->bodyID);
    SAFE_FREE(drone->shapeID);
    SAFE_FREE(drone);
}

void droneMove(const droneEntity *drone, const b2Vec2 direction)
{
    assert(drone != NULL);
    ASSERT_VEC(direction, -1.0f, 1.0f);

    b2Vec2 force = b2MulSV(DRONE_MOVE_MAGNITUDE, direction);
    b2Body_ApplyForceToCenter(drone->bodyID, force, true);
}

void createProjectile(const b2WorldId worldID, struct hashmap_s *shapeMap, CC_SList *projectiles, droneEntity *drone, const b2Vec2 normAim, const b2Vec2 aimRecoil)
{
    assert(shapeMap != NULL);
    assert(drone != NULL);
    ASSERT_VEC(normAim, -1.0f, 1.0f);

    b2BodyDef projectileBodyDef = b2DefaultBodyDef();
    projectileBodyDef.type = b2_dynamicBody;
    // TODO: make fixed rotation?
    projectileBodyDef.rotation = b2MakeRot(b2Atan2(normAim.x, normAim.y));
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    float radius = weaponRadius(drone->weapon);
    projectileBodyDef.position = b2Add(dronePos, b2MulSV(1.0f + radius * 1.5f, normAim));
    b2BodyId projectileBodyID = b2CreateBody(worldID, &projectileBodyDef);
    b2ShapeDef projectileShapeDef = b2DefaultShapeDef();
    projectileShapeDef.enableContactEvents = true;
    projectileShapeDef.density = weaponDensity(drone->weapon);
    projectileShapeDef.friction = 0.0f;
    projectileShapeDef.restitution = 1.0f;
    b2Circle projectileCircle = {.center = {.x = 0.0f, .y = 0.0f}, .radius = radius};

    b2ShapeId *projectileShapeID = (b2ShapeId *)calloc(1, sizeof(b2ShapeId));
    *projectileShapeID = b2CreateCircleShape(projectileBodyID, &projectileShapeDef, &projectileCircle);

    // add lateral drone velocity to projectile
    b2Vec2 droneVel = b2Body_GetLinearVelocity(drone->bodyID);
    b2Vec2 forwardVel = b2MulSV(b2Dot(droneVel, normAim), normAim);
    b2Vec2 lateralVel = b2Sub(droneVel, forwardVel);
    lateralVel = b2MulSV(weaponDroneMoveCoef(drone->weapon), lateralVel);
    b2Vec2 fire = b2MulAdd(lateralVel, weaponFire(drone->weapon), normAim);
    fire = b2Add(fire, aimRecoil);
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

    if (0 != hashmap_put(shapeMap, projectileShapeID, sizeof(projectileShapeID), e))
    {
        ERROR("error adding projectile to hashmap");
    }
}

void destroyProjectile(struct hashmap_s *shapeMap, CC_SList *projectiles, projectileEntity *projectile, bool full)
{
    assert(shapeMap != NULL);
    assert(projectile != NULL);

    entity *e = (entity *)hashmap_get(shapeMap, projectile->shapeID, sizeof(projectile->shapeID));
    SAFE_FREE(e);
    hashmap_remove(shapeMap, projectile->shapeID, sizeof(projectile->shapeID));

    if (full)
    {
        cc_slist_remove(projectiles, projectile, NULL);
        b2DestroyBody(projectile->bodyID);
    }

    SAFE_FREE(projectile->shapeID);
    SAFE_FREE(projectile);
}

void destroyAllProjectiles(struct hashmap_s *shapeMap, CC_SList *projectiles)
{
    for (SNode *cur = projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        destroyProjectile(shapeMap, projectiles, projectile, false);
    }
}

void droneShoot(const b2WorldId worldID, struct hashmap_s *shapeMap, CC_SList *projectiles, droneEntity *drone, const b2Vec2 aim)
{
    assert(shapeMap != NULL);
    assert(drone != NULL);
    assert(drone->ammo != 0);

    drone->shotThisStep = true;
    drone->triggerHeldSteps++;
    if (drone->weaponCooldown != 0.0f)
    {
        return;
    }
    if (drone->weapon != STANDARD_WEAPON && drone->ammo != INFINITE_AMMO)
    {
        drone->ammo--;
    }
    drone->weaponCooldown = weaponCoolDown(drone->weapon);

    b2Vec2 normAim = drone->lastAim;
    if (!b2VecEqual(aim, b2Vec2_zero))
    {
        normAim = b2Normalize(aim);
        drone->lastAim = normAim;
    }
    b2Vec2 aimRecoil = weaponAimRecoil(drone->weapon);
    b2Vec2 recoil = b2MulAdd(aimRecoil, -weaponRecoil(drone->weapon), normAim);
    b2Body_ApplyLinearImpulseToCenter(drone->bodyID, recoil, true);

    createProjectile(worldID, shapeMap, projectiles, drone, normAim, aimRecoil);

    if (drone->ammo == 0)
    {
        drone->weapon = STANDARD_WEAPON;
        drone->ammo = weaponAmmo(STANDARD_WEAPON);
    }
}

void droneStep(struct hashmap_s *shapeMap, droneEntity *drone, const float frameTime)
{
    assert(shapeMap != NULL);
    assert(drone != NULL);

    drone->weaponCooldown = b2MaxFloat(drone->weaponCooldown - frameTime, 0.0f);
    if (!drone->shotThisStep)
    {
        drone->triggerHeldSteps = 0;
    }
    else
    {
        drone->shotThisStep = false;
    }
    assert(drone->shotThisStep == false);

    b2Vec2 velocity = b2Body_GetLinearVelocity(drone->bodyID);
    if (!b2VecEqual(velocity, b2Vec2_zero))
    {
        b2Vec2 dragForce = {
            .x = -DRONE_DRAG_COEFFICIENT * velocity.x,
            .y = -DRONE_DRAG_COEFFICIENT * velocity.y,
        };
        b2Body_ApplyForceToCenter(drone->bodyID, dragForce, false);
    }
}

void projectilesStep(struct hashmap_s *shapeMap, CC_SList *projectiles)
{
    CC_SListIter iter;
    cc_slist_iter_init(&iter, projectiles);
    projectileEntity *projectile;
    while (cc_slist_iter_next(&iter, (void **)&projectile) != CC_ITER_END)
    {
        b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        b2Vec2 distance = b2Sub(pos, projectile->lastPos);
        projectile->distance += b2Length(distance);
        if (projectile->distance >= weaponMaxDistance(projectile->type))
        {
            cc_slist_iter_remove(&iter, NULL);
            destroyProjectile(shapeMap, projectiles, projectile, true);
            continue;
        }

        projectile->lastPos = pos;
    }
}

bool handleProjectileBeginContact(struct hashmap_s *shapeMap, CC_SList *projectiles, const entity *e1, const entity *e2)
{
    assert(shapeMap != NULL);
    assert(e1 != NULL);
    assert(e2 != NULL);

    projectileEntity *projectile = (projectileEntity *)e1->entity;
    if (e2->type != BOUNCY_WALL && e2->type != FLOATING_BOUNCY_WALL)
    {
        projectile->bounces++;
    }
    else
    {
        projectile->bouncyWallBounces++;
    }
    uint8_t maxBounces = weaponBounce(projectile->type);
    if (projectile->bounces == maxBounces || projectile->bouncyWallBounces == maxBounces * 3)
    {
        destroyProjectile(shapeMap, projectiles, projectile, true);
        return true;
    }

    return false;
}

void handleProjectileEndContact(const entity *e)
{
    assert(e != NULL);

    projectileEntity *projectile = (projectileEntity *)e->entity;
    b2Vec2 velocity = b2Body_GetLinearVelocity(projectile->bodyID);
    b2Vec2 newVel = b2MulSV(weaponFire(projectile->type) * weaponInvMass(projectile->type), b2Normalize(velocity));
    b2Body_SetLinearVelocity(projectile->bodyID, newVel);
}

void handleContactEvents(const b2WorldId worldID, struct hashmap_s *shapeMap, CC_SList *projectiles)
{
    assert(shapeMap != NULL);

    b2ContactEvents events = b2World_GetContactEvents(worldID);
    for (int i = 0; i < events.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent *event = events.beginEvents + i;
        entity *e1 = (entity *)hashmap_get(shapeMap, &event->shapeIdA, sizeof(event->shapeIdA));
        if (e1 == NULL)
        {
            continue;
        }
        entity *e2 = (entity *)hashmap_get(shapeMap, &event->shapeIdB, sizeof(event->shapeIdB));
        if (e2 == NULL)
        {
            continue;
        }

        bool projectileDestroyed = false;
        if (e1->type == PROJECTILE_ENTITY)
        {
            projectileDestroyed = handleProjectileBeginContact(shapeMap, projectiles, e1, e2);
        }
        if (!projectileDestroyed && e2->type == PROJECTILE_ENTITY)
        {
            handleProjectileBeginContact(shapeMap, projectiles, e2, e1);
        }
    }

    for (int i = 0; i < events.endCount; ++i)
    {
        const b2ContactEndTouchEvent *event = events.endEvents + i;
        entity *e1 = (entity *)hashmap_get(shapeMap, &event->shapeIdA, sizeof(event->shapeIdA));
        if (e1 != NULL && e1->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(e1);
        }
        entity *e2 = (entity *)hashmap_get(shapeMap, &event->shapeIdB, sizeof(event->shapeIdB));
        if (e2 != NULL && e2->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(e2);
        }
    }
}

#endif
