#pragma once

#include "helpers.h"
#include "env.h"
#include "settings.h"
#include "types.h"

static inline bool entityTypeIsWall(const enum entityType type)
{
    return type == STANDARD_WALL_ENTITY || type == BOUNCY_WALL_ENTITY || type == DEATH_WALL_ENTITY;
}

static inline uint16_t posToCellIdx(const env *e, const b2Vec2 pos)
{
    const uint16_t col = ((pos.x / 2.0f) + (float)e->columns - 1.0f) / 2.0f;
    const uint16_t row = (pos.y + ((float)e->rows * 2.0f) - 2.0f) / 4.0f;
    return col + (row * e->columns);
}

bool overlapCallback(b2ShapeId shapeId, void *context)
{
    bool *overlaps = (bool *)context;
    *overlaps = true;
    return false;
}

bool isOverlapping(env *e, const b2Vec2 pos, const float distance, const enum shapeCategory type, const uint64_t maskBits)
{
    b2AABB bounds = {
        .lowerBound = {.x = pos.x - distance, .y = pos.y - distance},
        .upperBound = {.x = pos.x + distance, .y = pos.y + distance},
    };
    b2QueryFilter filter = {
        .categoryBits = type,
        .maskBits = maskBits,
    };
    bool overlaps = false;
    b2World_OverlapAABB(e->worldID, bounds, filter, overlapCallback, &overlaps);
    return overlaps;
}

bool findOpenPos(env *e, const enum shapeCategory type, b2Vec2 *emptyPos)
{
    uint8_t checkedCells[BITNSLOTS(MAX_CELLS)] = {0};
    const size_t nCells = cc_deque_size(e->cells);
    uint16_t attempts = 0;

    while (true)
    {
        if (attempts == nCells)
        {
            return false;
        }
        const int cellIdx = randInt(0, nCells - 1);
        if (bitTest(checkedCells, cellIdx))
        {
            continue;
        }
        bitSet(checkedCells, cellIdx);
        attempts++;

        const mapCell *cell = safe_deque_get_at(e->cells, cellIdx);
        if (cell->ent != NULL)
        {
            continue;
        }

        if (type == WEAPON_PICKUP_SHAPE)
        {
            if (isOverlapping(e, cell->pos, PICKUP_THICKNESS / 2.0f, WEAPON_PICKUP_SHAPE, WALL_SHAPE))
            {
                continue;
            }
        }
        else if (type == DRONE_SHAPE)
        {
            if (isOverlapping(e, cell->pos, DRONE_WALL_SPAWN_DISTANCE, DRONE_SHAPE, WALL_SHAPE | DRONE_SHAPE))
            {
                continue;
            }
            if (isOverlapping(e, cell->pos, DRONE_DRONE_SPAWN_DISTANCE, DRONE_SHAPE, DRONE_SHAPE))
            {
                continue;
            }
        }

        if (!isOverlapping(e, cell->pos, MIN_SPAWN_DISTANCE, type, FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE))
        {
            *emptyPos = cell->pos;
            return true;
        }
    }
}

entity *createWall(env *e, const float posX, const float posY, const float width, const float height, const enum entityType type, bool floating)
{
    ASSERT(entityTypeIsWall(type));

    const b2Vec2 pos = (b2Vec2){.x = posX, .y = posY};

    b2BodyDef wallBodyDef = b2DefaultBodyDef();
    wallBodyDef.position = pos;
    if (floating)
    {
        wallBodyDef.type = b2_dynamicBody;
        wallBodyDef.linearDamping = FLOATING_WALL_DAMPING;
        wallBodyDef.angularDamping = FLOATING_WALL_DAMPING;
        wallBodyDef.isAwake = false;
    }
    b2BodyId wallBodyID = b2CreateBody(e->worldID, &wallBodyDef);
    b2Vec2 extent = {.x = width / 2.0f, .y = height / 2.0f};
    b2ShapeDef wallShapeDef = b2DefaultShapeDef();
    wallShapeDef.density = WALL_DENSITY;
    wallShapeDef.restitution = 0.0f;
    wallShapeDef.filter.categoryBits = WALL_SHAPE;
    wallShapeDef.filter.maskBits = FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE;
    if (floating)
    {
        wallShapeDef.filter.categoryBits = FLOATING_WALL_SHAPE;
        wallShapeDef.filter.maskBits |= WALL_SHAPE | WEAPON_PICKUP_SHAPE;
        wallShapeDef.enableSensorEvents = true;
    }

    if (type == BOUNCY_WALL_ENTITY)
    {
        wallShapeDef.restitution = BOUNCY_WALL_RESTITUTION;
    }
    if (type == DEATH_WALL_ENTITY)
    {
        wallShapeDef.enableContactEvents = true;
    }

    wallEntity *wall = (wallEntity *)fastMalloc(sizeof(wallEntity));
    wall->bodyID = wallBodyID;
    wall->position = b2Vec2_zero;
    if (!floating)
    {
        wall->position = pos;
    }
    wall->extent = extent;
    wall->isFloating = floating;
    wall->type = type;

    entity *ent = (entity *)fastMalloc(sizeof(entity));
    ent->type = type;
    ent->entity = wall;

    wallShapeDef.userData = ent;
    const b2Polygon wallPolygon = b2MakeBox(extent.x, extent.y);
    wall->shapeID = b2CreatePolygonShape(wallBodyID, &wallShapeDef, &wallPolygon);

    if (floating)
    {
        cc_deque_add(e->floatingWalls, wall);
    }
    else
    {
        cc_deque_add(e->walls, wall);
    }

    return ent;
}

void destroyWall(wallEntity *wall)
{
    entity *ent = (entity *)b2Shape_GetUserData(wall->shapeID);
    fastFree(ent);

    b2DestroyBody(wall->bodyID);
    fastFree(wall);
}

void updateCellsWithWall(env *e, entity *wallEnt)
{
    ASSERT(entityTypeIsWall(wallEnt->type));

    wallEntity *wall = (wallEntity *)wallEnt->entity;
    ASSERT(!wall->isFloating);
    ASSERT(wall->extent.x == WALL_THICKNESS / 2.0f || wall->extent.y == WALL_THICKNESS / 2.0f);
    b2Vec2 pos = wall->position;

    if (wall->extent.y == WALL_THICKNESS / 2.0f)
    {
        const float originalXPos = pos.x;
        pos.x -= wall->extent.x;
        const uint16_t startIdx = posToCellIdx(e, pos);
        pos.x = originalXPos + (originalXPos - pos.x);
        const uint16_t endIdx = posToCellIdx(e, pos);
        for (uint16_t i = startIdx; i <= endIdx; i++)
        {
            mapCell *cell = safe_deque_get_at(e->cells, i);
            if (cell->ent != NULL && cell->ent->type == WEAPON_PICKUP_ENTITY)
            {
                weaponPickupEntity *pickup = (weaponPickupEntity *)cell->ent->entity;
                pickup->respawnWait = PICKUP_RESPAWN_WAIT;
            }
            cell->ent = wallEnt;
        }
    }
    else
    {
        pos.y -= wall->extent.y;
        uint16_t idx = posToCellIdx(e, pos);
        const uint16_t cellRows = (wall->extent.y * 2.0f) / WALL_THICKNESS;
        for (uint16_t i = 0; i < cellRows; i++)
        {
            mapCell *cell = safe_deque_get_at(e->cells, idx);
            if (cell->ent != NULL && cell->ent->type == WEAPON_PICKUP_ENTITY)
            {
                weaponPickupEntity *pickup = (weaponPickupEntity *)cell->ent->entity;
                pickup->respawnWait = PICKUP_RESPAWN_WAIT;
            }
            cell->ent = wallEnt;
            idx += e->rows;
        }
    }
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

enum weaponType randWeaponPickupType(const env *e)
{
    while (true)
    {
        const enum weaponType type = (enum weaponType)randInt(STANDARD_WEAPON + 1, NUM_WEAPONS - 1);
        if (type != e->defaultWeapon->type)
        {
            return type;
        }
    }
}

void createWeaponPickup(env *e)
{
    b2BodyDef pickupBodyDef = b2DefaultBodyDef();
    if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pickupBodyDef.position))
    {
        ERROR("no open position for weapon pickup");
    }
    b2BodyId pickupBodyID = b2CreateBody(e->worldID, &pickupBodyDef);
    b2ShapeDef pickupShapeDef = b2DefaultShapeDef();
    pickupShapeDef.filter.categoryBits = WEAPON_PICKUP_SHAPE;
    pickupShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE;
    pickupShapeDef.isSensor = true;

    weaponPickupEntity *pickup = (weaponPickupEntity *)fastMalloc(sizeof(weaponPickupEntity));
    pickup->bodyID = pickupBodyID;
    pickup->weapon = randWeaponPickupType(e);
    pickup->respawnWait = 0.0f;
    pickup->floatingWallsTouching = 0;

    entity *ent = (entity *)fastMalloc(sizeof(entity));
    ent->type = WEAPON_PICKUP_ENTITY;
    ent->entity = pickup;

    const uint16_t cellIdx = posToCellIdx(e, pickupBodyDef.position);
    mapCell *cell = safe_deque_get_at(e->cells, cellIdx);
    cell->ent = ent;

    pickupShapeDef.userData = ent;
    const b2Polygon pickupPolygon = b2MakeBox(PICKUP_THICKNESS / 2.0f, PICKUP_THICKNESS / 2.0f);
    pickup->shapeID = b2CreatePolygonShape(pickupBodyID, &pickupShapeDef, &pickupPolygon);

    cc_deque_add(e->pickups, pickup);
}

void destroyWeaponPickup(weaponPickupEntity *pickup)
{
    entity *ent = (entity *)b2Shape_GetUserData(pickup->shapeID);
    fastFree(ent);

    b2DestroyBody(pickup->bodyID);
    fastFree(pickup);
}

void createDrone(env *e)
{
    b2BodyDef droneBodyDef = b2DefaultBodyDef();
    droneBodyDef.type = b2_dynamicBody;
    if (!findOpenPos(e, DRONE_SHAPE, &droneBodyDef.position))
    {
        ERROR("no open position for drone");
    }
    droneBodyDef.fixedRotation = true;
    droneBodyDef.linearDamping = DRONE_LINEAR_DAMPING;
    b2BodyId droneBodyID = b2CreateBody(e->worldID, &droneBodyDef);
    b2ShapeDef droneShapeDef = b2DefaultShapeDef();
    droneShapeDef.density = DRONE_DENSITY;
    droneShapeDef.friction = 0.0f;
    droneShapeDef.restitution = 0.2f;
    droneShapeDef.filter.categoryBits = DRONE_SHAPE;
    droneShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    droneShapeDef.enableContactEvents = true;
    droneShapeDef.enableSensorEvents = true;
    const b2Circle droneCircle = {.center = b2Vec2_zero, .radius = DRONE_RADIUS};

    droneEntity *drone = (droneEntity *)fastMalloc(sizeof(droneEntity));
    drone->bodyID = droneBodyID;
    drone->weaponInfo = e->defaultWeapon;
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
    drone->weaponCooldown = 0.0f;
    drone->heat = 0;
    drone->charge = 0;
    drone->shotThisStep = false;
    drone->lastAim = (b2Vec2){.x = 0.0f, .y = -1.0f};
    drone->dead = false;

    entity *ent = (entity *)fastMalloc(sizeof(entity));
    ent->type = DRONE_ENTITY;
    ent->entity = drone;

    droneShapeDef.userData = ent;
    drone->shapeID = b2CreateCircleShape(droneBodyID, &droneShapeDef, &droneCircle);

    cc_deque_add(e->drones, drone);
}

void destroyDrone(droneEntity *drone)
{
    entity *ent = (entity *)b2Shape_GetUserData(drone->shapeID);
    fastFree(ent);

    b2DestroyBody(drone->bodyID);
    fastFree(drone);
}

void droneMove(const droneEntity *drone, const b2Vec2 direction)
{
    ASSERT_VEC_NORMALIZED(direction);

    b2Vec2 force = b2MulSV(DRONE_MOVE_MAGNITUDE, direction);
    b2Body_ApplyForceToCenter(drone->bodyID, force, true);
}

void createProjectile(env *e, droneEntity *drone, const b2Vec2 normAim)
{
    ASSERT_VEC_NORMALIZED(normAim);

    b2BodyDef projectileBodyDef = b2DefaultBodyDef();
    projectileBodyDef.type = b2_dynamicBody;
    projectileBodyDef.fixedRotation = true;
    projectileBodyDef.isBullet = drone->weaponInfo->isPhysicsBullet;
    projectileBodyDef.enableSleep = false;
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    float radius = drone->weaponInfo->radius;
    projectileBodyDef.position = b2MulAdd(dronePos, 1.0f + (radius * 1.5f), normAim);
    b2BodyId projectileBodyID = b2CreateBody(e->worldID, &projectileBodyDef);
    b2ShapeDef projectileShapeDef = b2DefaultShapeDef();
    projectileShapeDef.enableContactEvents = true;
    projectileShapeDef.density = drone->weaponInfo->density;
    projectileShapeDef.friction = 0.0f;
    projectileShapeDef.restitution = 1.0f;
    projectileShapeDef.filter.categoryBits = PROJECTILE_SHAPE;
    projectileShapeDef.filter.maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE;
    const b2Circle projectileCircle = {.center = b2Vec2_zero, .radius = radius};

    b2ShapeId projectileShapeID = b2CreateCircleShape(projectileBodyID, &projectileShapeDef, &projectileCircle);

    // add lateral drone velocity to projectile
    b2Vec2 droneVel = b2Body_GetLinearVelocity(drone->bodyID);
    b2Vec2 forwardVel = b2MulSV(b2Dot(droneVel, normAim), normAim);
    b2Vec2 lateralVel = b2Sub(droneVel, forwardVel);
    lateralVel = b2MulSV(projectileShapeDef.density / DRONE_MOVE_AIM_DIVISOR, lateralVel);
    b2Vec2 aim = weaponAdjustAim(drone->weaponInfo->type, drone->heat, normAim);
    b2Vec2 fire = b2MulAdd(lateralVel, weaponFire(drone->weaponInfo->type), aim);
    b2Body_ApplyLinearImpulseToCenter(projectileBodyID, fire, true);

    projectileEntity *projectile = (projectileEntity *)fastMalloc(sizeof(projectileEntity));
    projectile->bodyID = projectileBodyID;
    projectile->shapeID = projectileShapeID;
    projectile->weaponInfo = drone->weaponInfo;
    projectile->lastPos = projectileBodyDef.position;
    projectile->distance = 0.0f;
    projectile->bounces = 0;
    cc_slist_add(e->projectiles, projectile);

    entity *ent = (entity *)fastMalloc(sizeof(entity));
    ent->type = PROJECTILE_ENTITY;
    ent->entity = projectile;

    b2Shape_SetUserData(projectile->shapeID, ent);
}

void destroyProjectile(env *e, projectileEntity *projectile, const bool full)
{
    b2ExplosionDef explosion;
    if (weaponExplosion(projectile->weaponInfo->type, &explosion))
    {
        explosion.position = b2Body_GetPosition(projectile->bodyID);
        explosion.maskBits = FLOATING_WALL_SHAPE | DRONE_SHAPE;
        b2World_Explode(e->worldID, &explosion);
        e->explosion = explosion;
        e->explosionSteps = EXPLOSION_STEPS;
    }

    entity *ent = (entity *)b2Shape_GetUserData(projectile->shapeID);
    fastFree(ent);

    if (full)
    {
        const enum cc_stat res = cc_slist_remove(e->projectiles, projectile, NULL);
        MAYBE_UNUSED(res);
        ASSERT(res == CC_OK);
        b2DestroyBody(projectile->bodyID);
    }

    fastFree(projectile);
}

void destroyAllProjectiles(env *e)
{
    for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *p = (projectileEntity *)cur->data;
        destroyProjectile(e, p, false);
    }
}

void handleSuddenDeath(env *e)
{
    ASSERT(e->suddenDeathSteps == 0);

    e->suddenDeathWallCounter++;
    entity *topWall = createWall(
        e,
        e->bounds.min.x + (((e->columns / 2) - 1) * WALL_THICKNESS),
        e->bounds.min.y + ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        WALL_THICKNESS * (e->columns - (e->suddenDeathWallCounter * 2)),
        WALL_THICKNESS,
        DEATH_WALL_ENTITY,
        false);
    entity *bottomWall = createWall(
        e,
        e->bounds.max.x - (((e->columns / 2) - 1) * WALL_THICKNESS),
        e->bounds.max.y - ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        WALL_THICKNESS * (e->columns - (e->suddenDeathWallCounter * 2)),
        WALL_THICKNESS,
        DEATH_WALL_ENTITY,
        false);

    entity *leftWall = createWall(
        e,
        e->bounds.max.x - ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        e->bounds.min.y + (((e->rows / 2) - 1) * WALL_THICKNESS),
        WALL_THICKNESS,
        WALL_THICKNESS * (e->rows - (e->suddenDeathWallCounter * 2)),
        DEATH_WALL_ENTITY,
        false);
    entity *rightWall = createWall(
        e,
        e->bounds.min.x + ((WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2)),
        e->bounds.max.y - (((e->rows / 2) - 1) * WALL_THICKNESS),
        WALL_THICKNESS,
        WALL_THICKNESS * (e->rows - (e->suddenDeathWallCounter * 2)),
        DEATH_WALL_ENTITY,
        false);

    updateCellsWithWall(e, topWall);
    updateCellsWithWall(e, bottomWall);
    updateCellsWithWall(e, leftWall);
    updateCellsWithWall(e, rightWall);

    bool droneDead = false;
    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone = safe_deque_get_at(e->drones, i);
        const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
        if (isOverlapping(e, pos, DRONE_RADIUS, DRONE_SHAPE, WALL_SHAPE))
        {
            drone->dead = true;
            droneDead = true;
        }
    }
    if (droneDead)
    {
        return;
    }

    for (size_t i = 0; i < cc_deque_size(e->floatingWalls); i++)
    {
        const wallEntity *wall = safe_deque_get_at(e->floatingWalls, i);
        const b2Vec2 pos = b2Body_GetPosition(wall->bodyID);
        if (isOverlapping(e, pos, FLOATING_WALL_THICKNESS / 2.0f, FLOATING_WALL_SHAPE, WALL_SHAPE))
        {
            // TODO: destroy floating wall?
            b2Body_Disable(wall->bodyID);
        }
    }
}

void droneChangeWeapon(const env *e, droneEntity *drone, const enum weaponType newWeapon)
{
    // only top up ammo if the weapon is the same
    if (drone->weaponInfo->type != newWeapon)
    {
        drone->weaponCooldown = 0.0f;
        drone->charge = 0;
        drone->heat = 0;
    }
    drone->weaponInfo = &weaponInfos[newWeapon];
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
}

void droneShoot(env *e, droneEntity *drone, const b2Vec2 aim)
{
    ASSERT(drone->ammo != 0);

    drone->shotThisStep = true;
    // TODO: rework heat to only increase when projectiles are fired,
    // and only cool down after the next shot was skipped
    drone->heat++;
    if (drone->weaponCooldown != 0.0f)
    {
        return;
    }
    drone->charge++;
    if (drone->charge < weaponCharge(drone->weaponInfo->type))
    {
        return;
    }

    if (drone->ammo != INFINITE)
    {
        drone->ammo--;
    }
    drone->weaponCooldown = drone->weaponInfo->coolDown;
    drone->charge = 0.0f;

    b2Vec2 normAim = drone->lastAim;
    if (!b2VecEqual(aim, b2Vec2_zero))
    {
        normAim = b2Normalize(aim);
    }
    ASSERT_VEC_NORMALIZED(normAim);
    b2Vec2 recoil = b2MulSV(-drone->weaponInfo->recoilMagnitude, normAim);
    b2Body_ApplyLinearImpulseToCenter(drone->bodyID, recoil, true);

    for (int i = 0; i < drone->weaponInfo->numProjectiles; i++)
    {
        createProjectile(e, drone, normAim);
    }

    if (drone->ammo == 0)
    {
        droneChangeWeapon(e, drone, e->defaultWeapon->type);
        drone->weaponCooldown = drone->weaponInfo->coolDown;
    }
}

void droneStep(droneEntity *drone, const float frameTime)
{
    ASSERT(frameTime != 0.0f);

    // TODO: set drone pos

    drone->weaponCooldown = fmaxf(drone->weaponCooldown - frameTime, 0.0f);
    if (!drone->shotThisStep)
    {
        drone->charge = fmaxf(drone->charge - 1, 0);
        drone->heat = fmaxf(drone->heat - 1, 0);
    }
    else
    {
        drone->shotThisStep = false;
    }
    ASSERT(drone->shotThisStep == false);
}

void projectilesStep(env *e)
{
    CC_SListIter iter;
    cc_slist_iter_init(&iter, e->projectiles);
    projectileEntity *projectile;
    while (cc_slist_iter_next(&iter, (void **)&projectile) != CC_ITER_END)
    {
        const float maxDistance = projectile->weaponInfo->maxDistance;
        if (maxDistance == INFINITE)
        {
            continue;
        }
        const b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        const b2Vec2 distance = b2Sub(pos, projectile->lastPos);
        projectile->distance += b2Length(distance);
        if (projectile->distance >= maxDistance)
        {
            // we have to destroy the projectile using the iterator so
            // we can continue to iterate correctly, copy the body ID
            // so we can destroy it after the projectile has been freed
            cc_slist_iter_remove(&iter, NULL);
            const b2BodyId bodyID = projectile->bodyID;
            destroyProjectile(e, projectile, false);
            b2DestroyBody(bodyID);
            continue;
        }

        projectile->lastPos = pos;
    }
}

void weaponPickupStep(env *e, weaponPickupEntity *pickup, const float frameTime)
{
    ASSERT(frameTime != 0.0f);

    if (pickup->respawnWait != 0.0f)
    {
        pickup->respawnWait = fmaxf(pickup->respawnWait - frameTime, 0.0f);
        if (pickup->respawnWait == 0.0f)
        {
            b2Vec2 pos;
            if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pos))
            {
                // TODO: destroy weapon pickup
                pickup->respawnWait = FLT_MAX;
                return;
            }
            b2Body_SetTransform(pickup->bodyID, pos, b2Rot_identity);
            pickup->weapon = randWeaponPickupType(e);

            const uint16_t cellIdx = posToCellIdx(e, pos);
            mapCell *cell = safe_deque_get_at(e->cells, cellIdx);
            entity *ent = (entity *)b2Shape_GetUserData(pickup->shapeID);
            cell->ent = ent;
        }
    }
}

bool handleProjectileBeginContact(env *e, const entity *proj, const entity *ent)
{
    projectileEntity *projectile = (projectileEntity *)proj->entity;
    // e (shape B in the collision) will be NULL if it's another
    // projectile that was just destroyed
    if (ent == NULL || (ent != NULL && ent->type == PROJECTILE_ENTITY))
    {
        // always allow projectiles to bounce off each other
        return false;
    }
    else if (ent != NULL && ent->type == BOUNCY_WALL_ENTITY)
    {
        // always allow projectiles to bounce off bouncy walls
        return false;
    }
    else if (ent != NULL && ent->type != BOUNCY_WALL_ENTITY)
    {
        projectile->bounces++;
    }
    const uint8_t maxBounces = projectile->weaponInfo->maxBounces;
    if (projectile->bounces == maxBounces)
    {
        destroyProjectile(e, projectile, true);
        return true;
    }

    return false;
}

void handleProjectileEndContact(const entity *p)
{
    projectileEntity *projectile = (projectileEntity *)p->entity;
    b2Vec2 velocity = b2Body_GetLinearVelocity(projectile->bodyID);
    b2Vec2 newVel = b2MulSV(weaponFire(projectile->weaponInfo->type) * projectile->weaponInfo->invMass, b2Normalize(velocity));
    b2Body_SetLinearVelocity(projectile->bodyID, newVel);
}

void handleContactEvents(env *e)
{
    b2ContactEvents events = b2World_GetContactEvents(e->worldID);
    for (int i = 0; i < events.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent *event = events.beginEvents + i;
        entity *e1 = NULL;
        entity *e2 = NULL;

        if (b2Shape_IsValid(event->shapeIdA))
        {
            e1 = (entity *)b2Shape_GetUserData(event->shapeIdA);
            ASSERT(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB))
        {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            ASSERT(e2 != NULL);
        }

        if (e1 != NULL)
        {
            if (e1->type == PROJECTILE_ENTITY)
            {
                if (handleProjectileBeginContact(e, e1, e2))
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
                handleProjectileBeginContact(e, e2, e1);
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
            ASSERT(e1 != NULL);
        }
        if (b2Shape_IsValid(event->shapeIdB))
        {
            e2 = (entity *)b2Shape_GetUserData(event->shapeIdB);
            ASSERT(e2 != NULL);
        }

        if (e1 != NULL && e1->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(e1);
        }
        if (e2 != NULL && e2->type == PROJECTILE_ENTITY)
        {
            handleProjectileEndContact(e2);
        }
    }
}

void handleWeaponPickupBeginTouch(env *e, const entity *sensor, entity *visitor)
{
    weaponPickupEntity *pickup = (weaponPickupEntity *)sensor->entity;
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0)
    {
        return;
    }

    switch (visitor->type)
    {
    case DRONE_ENTITY:
        pickup->respawnWait = PICKUP_RESPAWN_WAIT;
        droneEntity *drone = (droneEntity *)visitor->entity;
        droneChangeWeapon(e, drone, pickup->weapon);
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching++;
        break;
    default:
        ERRORF("invalid weapon pickup begin touch visitor %d", visitor->type);
    }
}

void handleWeaponPickupEndTouch(env *e, const entity *sensor, entity *visitor)
{
    weaponPickupEntity *pickup = (weaponPickupEntity *)sensor->entity;
    if (pickup->respawnWait != 0.0f)
    {
        return;
    }

    switch (visitor->type)
    {
    case DRONE_ENTITY:
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        pickup->floatingWallsTouching--;
        break;
    default:
        ERRORF("invalid weapon pickup end touch visitor %d", visitor->type);
    }
}

void handleSensorEvents(env *e)
{
    b2SensorEvents events = b2World_GetSensorEvents(e->worldID);
    for (int i = 0; i < events.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent *event = events.beginEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId))
        {
            ERROR("could not find sensor shape for begin touch event");
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        ASSERT(s != NULL);
        ASSERT(s->type == WEAPON_PICKUP_ENTITY);

        if (!b2Shape_IsValid(event->visitorShapeId))
        {
            ERROR("could not find visitor shape for begin touch event");
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        ASSERT(v != NULL);

        handleWeaponPickupBeginTouch(e, s, v);
    }

    for (int i = 0; i < events.endCount; ++i)
    {
        const b2SensorEndTouchEvent *event = events.endEvents + i;
        if (!b2Shape_IsValid(event->sensorShapeId))
        {
            ERROR("could not find sensor shape for end touch event");
        }
        entity *s = (entity *)b2Shape_GetUserData(event->sensorShapeId);
        ASSERT(s != NULL);
        ASSERT(s->type == WEAPON_PICKUP_ENTITY);

        if (!b2Shape_IsValid(event->visitorShapeId))
        {
            ERROR("could not find visitor shape for end touch event");
        }
        entity *v = (entity *)b2Shape_GetUserData(event->visitorShapeId);
        ASSERT(v != NULL);

        handleWeaponPickupEndTouch(e, s, v);
    }
}
