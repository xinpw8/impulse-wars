#pragma once

#include "game.h"
#include "map.h"
#include "settings.h"
#include "types.h"

const mapEntry *map = &prototypeArenaMap;

env *createEnv(void)
{
    env *e = fastCalloc(1, sizeof(env));

    initWeapons();

    cc_deque_new(&e->cells);
    cc_deque_new(&e->walls);
    cc_deque_new(&e->floatingWalls);
    cc_deque_new(&e->drones);
    cc_deque_new(&e->pickups);
    cc_slist_new(&e->projectiles);

    return e;
}

void setupEnv(env *e)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);

    e->stepsLeft = ROUND_STEPS;
    e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
    e->suddenDeathWallCounter = 0;

    createMap(e, map);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_deque_size(e->walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->walls, i, (void **)&wall);
        bounds.min.x = fminf(wall->position.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->position.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->position.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->position.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    for (int i = 0; i < NUM_DRONES; i++)
    {
        createDrone(e);
    }

    placeFloatingWalls(e, map);

    for (int i = 0; i < map->weaponPickups; i++)
    {
        createWeaponPickup(e);
    }
}

void clearEnv(env *e)
{
    for (size_t i = 0; i < cc_deque_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup;
        cc_deque_get_at(e->pickups, i, (void **)&pickup);
        destroyWeaponPickup(pickup);
    }

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        destroyDrone(drone);
    }

    destroyAllProjectiles(e);

    for (size_t i = 0; i < cc_deque_size(e->walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->walls, i, (void **)&wall);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_deque_size(e->floatingWalls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->floatingWalls, i, (void **)&wall);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_deque_size(e->cells); i++)
    {
        mapCell *cell;
        cc_deque_get_at(e->cells, i, (void **)&cell);
        fastFree(cell);
    }

    cc_deque_remove_all(e->cells);
    cc_deque_remove_all(e->walls);
    cc_deque_remove_all(e->floatingWalls);
    cc_deque_remove_all(e->drones);
    cc_deque_remove_all(e->pickups);
    cc_slist_remove_all(e->projectiles);

    b2DestroyWorld(e->worldID);
}

void destroyEnv(env *e)
{
    fastFree(weaponInfos);

    clearEnv(e);

    cc_deque_destroy(e->cells);
    cc_deque_destroy(e->walls);
    cc_deque_destroy(e->floatingWalls);
    cc_deque_destroy(e->drones);
    cc_deque_destroy(e->pickups);
    cc_slist_destroy(e->projectiles);

    fastFree(e);
}

void resetEnv(env *e)
{
    clearEnv(e);
    setupEnv(e);
}

void stepEnv(env *e, float deltaTime)
{
    e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
    if (e->stepsLeft == 0)
    {
        e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
        if (e->suddenDeathSteps == 0)
        {
            handleSuddenDeath(e);
            e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
        }
    }

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        droneStep(drone, deltaTime);
    }

    projectilesStep(e);

    for (size_t i = 0; i < cc_deque_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup;
        cc_deque_get_at(e->pickups, i, (void **)&pickup);
        weaponPickupStep(e, pickup, deltaTime);
    }

    b2World_Step(e->worldID, deltaTime, BOX2D_SUBSTEPS);

    handleContactEvents(e);
    handleSensorEvents(e);
}

bool envTerminated(env *e)
{
    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        if (drone->dead)
        {
            return true;
        }
    }
    return false;
}
