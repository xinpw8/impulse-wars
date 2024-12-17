#pragma once
#ifndef IMPULSE_WARS_ENV_H
#define IMPULSE_WARS_ENV_H

#include "game.h"
#include "map.h"
#include "settings.h"
#include "types.h"

const uint16_t scalarObsOffset = SCALAR_OBS_SIZE;
const uint16_t droneObsOffset = scalarObsOffset + (NUM_DRONES * DRONE_OBS_SIZE);
const uint16_t projectileObsOffset = droneObsOffset + (NUM_PROJECTILE_OBS * PROJECTILE_OBS_SIZE);
const uint16_t floatingWallObsOffset = projectileObsOffset + (NUM_FLOATING_WALL_OBS * FLOATING_WALL_OBS_SIZE);
const uint16_t mapCellObsOffset = floatingWallObsOffset + (MAX_MAP_COLUMNS * MAX_MAP_ROWS);

void computeObs(env *e)
{
    uint16_t offset = 0;

    // add scalar observations
    offset += oneHotEncode(e->obs, offset, 0, NUM_DRONES);
    e->obs[offset++] = scaleValue(e->stepsLeft, ROUND_STEPS, true);

    // compute drone observations
    for (size_t i = 0; i < cc_array_size(e->drones); i++)
    {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);

        int8_t ammo = drone->ammo;
        int8_t maxAmmo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
        float scaledAmmo;
        if (ammo == INFINITE)
        {
            scaledAmmo = 0.0f;
        }
        else
        {
            scaledAmmo = scaleValue(ammo, maxAmmo, true);
        }

        // will be processed in an embedding layer separately
        // add 1 so 0 can be used to represent no weapon
        e->obs[offset++] = (float)(drone->weaponInfo->type + 1);
        e->obs[offset++] = scaleValue(pos.x, MAX_X_POS, false);
        e->obs[offset++] = scaleValue(pos.y, MAX_Y_POS, false);
        e->obs[offset++] = scaleValue(drone->lastVelocity.x, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(drone->lastVelocity.x, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(drone->lastAim.x, 1.0f, false);
        e->obs[offset++] = scaleValue(drone->lastAim.y, 1.0f, false);
        e->obs[offset++] = scaledAmmo;
        e->obs[offset++] = scaleValue(drone->weaponCooldown, drone->weaponInfo->coolDown, true);
        e->obs[offset++] = scaleValue(drone->charge, weaponCharge(drone->weaponInfo->type), true);

        ASSERT(i * DRONE_OBS_SIZE <= NUM_DRONES * DRONE_OBS_SIZE);
        ASSERT(offset <= droneObsOffset);

        // DEBUG_LOGF("drone cell index: %d", posToCellIdx(e, pos));
    }
    ASSERT(offset == droneObsOffset);

    //  compute projectile observations
    uint16_t projIdx = 0;
    for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next)
    {
        if (projIdx > NUM_PROJECTILE_OBS)
        {
            DEBUG_LOGF("Too many projectiles %zu, truncating", cc_slist_size(e->projectiles));
            break;
        }

        const projectileEntity *projectile = (projectileEntity *)cur->data;
        const b2Vec2 vel = b2Body_GetLinearVelocity(projectile->bodyID);

        // will be processed in an embedding layer separately
        // add 1 so 0 can be used to represent no weapon
        e->obs[offset++] = (float)(projectile->weaponInfo->type + 1);
        e->obs[offset++] = scaleValue(projectile->lastPos.x, MAX_X_POS, false);
        e->obs[offset++] = scaleValue(projectile->lastPos.y, MAX_Y_POS, false);
        e->obs[offset++] = scaleValue(vel.x, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(vel.y, MAX_SPEED, false);

        ASSERT(projIdx * PROJECTILE_OBS_SIZE <= NUM_PROJECTILE_OBS * PROJECTILE_OBS_SIZE);
        ASSERT(offset <= projectileObsOffset);
        projIdx++;
    }
    // zero out any remaining projectile observations
    const uint16_t projectileObsSet = abs(offset - droneObsOffset);
    const uint16_t projectileObsUnset = (NUM_PROJECTILE_OBS * PROJECTILE_OBS_SIZE) - projectileObsSet;
    if (projectileObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, projectileObsUnset * sizeof(float));
        offset += projectileObsUnset;
    }
    ASSERT(offset == projectileObsOffset);

    // compute floating wall observations
    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++)
    {
        const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        const b2Vec2 pos = b2Body_GetPosition(wall->bodyID);
        const b2Vec2 vel = b2Body_GetLinearVelocity(wall->bodyID);
        const float angle = b2Rot_GetAngle(b2Body_GetRotation(wall->bodyID)) * RAD2DEG;

        // will be processed in an embedding layer separately
        // 1 doesn't need to be added here as the first valid wall type is 1 already
        e->obs[offset++] = (float)(wall->type);
        e->obs[offset++] = scaleValue(pos.x, MAX_X_POS, false);
        e->obs[offset++] = scaleValue(pos.y, MAX_Y_POS, false);
        e->obs[offset++] = scaleValue(vel.x, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(vel.y, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(angle, 180.0f, false);

        ASSERT(i * FLOATING_WALL_OBS_SIZE <= NUM_FLOATING_WALL_OBS * FLOATING_WALL_OBS_SIZE);
        ASSERT(offset <= floatingWallObsOffset);
    }
    // zero out any remaining floating wall observations
    const uint16_t floatingWallObsSet = abs(offset - projectileObsOffset);
    const uint16_t floatingWallObsUnset = (NUM_FLOATING_WALL_OBS * FLOATING_WALL_OBS_SIZE) - floatingWallObsSet;
    if (floatingWallObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, floatingWallObsUnset * sizeof(float));
        offset += floatingWallObsUnset;
    }
    ASSERT(offset == floatingWallObsOffset);

    // compute map cell observations
    // TODO: add discretized positions of drones and maybe projectiles?
    for (size_t i = 0; i < cc_array_size(e->cells); i++)
    {
        const mapCell *cell = safe_array_get_at(e->cells, i);
        // empty cells are set as 1 so 0 can be used to represent the end of the map
        float cellType = 1.0f;
        if (cell->ent != NULL)
        {
            if (cell->ent->type == WEAPON_PICKUP_ENTITY)
            {
                // add weapon pickup type to cell type so that the type
                // of the weapon pickup can be observed
                const weaponPickupEntity *pickup = (weaponPickupEntity *)cell->ent->entity;
                cellType = (float)(cell->ent->type + pickup->weapon + 1);
                ASSERTF(cellType <= OBS_HIGH, "cellType %f", cellType);
            }
            else
            {
                cellType = (float)(cell->ent->type + 1);
            }
        }
        // will be processed in an embedding layer separately
        e->obs[offset++] = cellType;

        ASSERT(i < MAX_MAP_COLUMNS * MAX_MAP_ROWS);
        ASSERT(offset <= OBS_SIZE);
    }
    // zero out any remaining map cell observations
    const uint16_t mapCellObsSet = abs(offset - floatingWallObsOffset);
    const uint16_t mapCellObsUnset = (MAX_MAP_COLUMNS * MAX_MAP_ROWS) - mapCellObsSet;
    if (mapCellObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, mapCellObsUnset * sizeof(float));
        offset += mapCellObsUnset;
    }
    ASSERT(offset == OBS_SIZE);

    // copy observations for other agent
    // TODO: handle more than 2 drones

    // add scalar observations
    e->obs[OBS_SIZE] = e->obs[0];
    oneHotEncode(e->obs, OBS_SIZE + 1, 1, NUM_DRONES);

    // reorder drone observation so that the second drone is first
    memcpy(
        e->obs + OBS_SIZE + scalarObsOffset,
        e->obs + droneObsOffset,
        droneObsOffset * sizeof(float));
    memcpy(
        e->obs + OBS_SIZE + droneObsOffset,
        e->obs,
        droneObsOffset * sizeof(float));
    // copy the rest of the observations over unmodified
    memcpy(
        e->obs + OBS_SIZE + droneObsOffset,
        e->obs + droneObsOffset,
        (OBS_SIZE - droneObsOffset) * sizeof(float));

    // assert that the observations are the same after the drone reordering
    ASSERT(
        memcmp(
            e->obs + droneObsOffset,
            e->obs + OBS_SIZE + droneObsOffset,
            (OBS_SIZE - droneObsOffset) * sizeof(float)) == 0);
}

void setupEnv(env *e)
{
    e->needsReset = false;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);

    e->stepsLeft = ROUND_STEPS;
    e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
    e->suddenDeathWallCounter = 0;

    DEBUG_LOG("creating map");
    const int mapIdx = randInt(&e->randState, 0, NUM_MAPS - 1);
    createMap(e, mapIdx);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_array_size(e->walls); i++)
    {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        bounds.min.x = fminf(wall->position.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->position.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->position.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->position.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    DEBUG_LOG("creating drones");
    for (int i = 0; i < NUM_DRONES; i++)
    {
        createDrone(e, i);
    }

    DEBUG_LOG("placing floating walls");
    placeRandFloatingWalls(e, mapIdx);

    DEBUG_LOG("creating weapon pickups");
    for (int i = 0; i < maps[mapIdx]->weaponPickups; i++)
    {
        createWeaponPickup(e);
    }

    computeObs(e);
}

env *initEnv(env *e, float *obs, float *actions, float *rewards, unsigned char *terminals, uint64_t seed)
{
    e->obs = obs;
    e->actions = actions;
    e->rewards = rewards;
    e->terminals = terminals;

    e->randState = seed;
    e->needsReset = false;

    cc_array_new(&e->cells);
    cc_array_new(&e->walls);
    cc_array_new(&e->floatingWalls);
    cc_array_new(&e->drones);
    cc_array_new(&e->pickups);
    cc_slist_new(&e->projectiles);

    setupEnv(e);

    return e;
}

void clearEnv(env *e)
{
    memset(e->obs, 0x0, OBS_SIZE * NUM_DRONES * sizeof(float));
    memset(e->terminals, 0x0, NUM_DRONES * sizeof(bool));

    for (size_t i = 0; i < cc_array_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        destroyWeaponPickup(pickup);
    }

    for (size_t i = 0; i < cc_array_size(e->drones); i++)
    {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        destroyDrone(drone);
    }

    destroyAllProjectiles(e);

    for (size_t i = 0; i < cc_array_size(e->walls); i++)
    {
        wallEntity *wall = safe_array_get_at(e->walls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++)
    {
        wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->cells); i++)
    {
        mapCell *cell = safe_array_get_at(e->cells, i);
        fastFree(cell);
    }

    cc_array_remove_all(e->cells);
    cc_array_remove_all(e->walls);
    cc_array_remove_all(e->floatingWalls);
    cc_array_remove_all(e->drones);
    cc_array_remove_all(e->pickups);
    cc_slist_remove_all(e->projectiles);

    b2DestroyWorld(e->worldID);
}

void destroyEnv(env *e)
{
    clearEnv(e);

    cc_array_destroy(e->cells);
    cc_array_destroy(e->walls);
    cc_array_destroy(e->floatingWalls);
    cc_array_destroy(e->drones);
    cc_array_destroy(e->pickups);
    cc_slist_destroy(e->projectiles);
}

void resetEnv(env *e)
{
    clearEnv(e);
    setupEnv(e);
}

void computeReward(env *e, const droneEntity *drone)
{
    uint8_t enemyID = 1;
    if (drone->idx == 1)
    {
        enemyID = 0;
    }
    const droneEntity *enemyDrone = safe_array_get_at(e->drones, enemyID);

    if (enemyDrone->dead)
    {
        e->rewards[drone->idx] += KILL_REWARD;
        return;
    }
    if (drone->dead)
    {
        e->rewards[drone->idx] += DEATH_REWARD;
        return;
    }
    if (!drone->hitInfo.shotHit && !drone->hitInfo.explosionHit)
    {
        return;
    }

    // compute reward based off of how much the projectile(s) or explosion(s)
    // caused the enemy drone to change velocity
    const float prevEnemySpeed = b2Length(enemyDrone->lastVelocity);
    const float curEnemySpeed = b2Length(b2Body_GetLinearVelocity(enemyDrone->bodyID));
    e->rewards[drone->idx] += scaleValue(fabsf(curEnemySpeed - prevEnemySpeed), MAX_SPEED, true) * SHOT_HIT_REWARD_COEF;
}

void computeRewards(env *e)
{
    for (size_t i = 0; i < cc_array_size(e->drones); i++)
    {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        computeReward(e, drone);

        if (e->rewards[drone->idx] != 0.0f)
        {
            DEBUG_LOGF("reward[%d]: %f\n", drone->idx, e->rewards[drone->idx]);
        }
    }
}

void stepEnv(env *e)
{
    if (e->needsReset)
    {
        DEBUG_LOG("Resetting environment");
        resetEnv(e);
    }

    for (int i = 0; i < FRAMESKIP; i++)
    {
        // handle actions
        for (size_t i = 0; i < cc_array_size(e->drones); i++)
        {
            const uint8_t offset = i * ACTION_SIZE;
            const b2Vec2 move = b2Normalize((b2Vec2){.x = e->actions[offset + 0], .y = e->actions[offset + 1]});
            const b2Vec2 aim = b2Normalize((b2Vec2){.x = e->actions[offset + 2], .y = e->actions[offset + 3]});
            const bool shoot = e->actions[offset + 4] > 0.0f;

            droneEntity *drone = safe_array_get_at(e->drones, i);
            if (!b2VecEqual(move, b2Vec2_zero))
            {
                droneMove(drone, move);
            }
            if (shoot)
            {
                droneShoot(e, drone, aim);
            }
            if (!b2VecEqual(aim, b2Vec2_zero))
            {
                drone->lastAim = b2Normalize(aim);
            }

            e->rewards[i] = 0.0f;
            drone->lastVelocity = b2Body_GetLinearVelocity(drone->bodyID);
            memset(&drone->hitInfo, 0x0, sizeof(stepHitInfo));
        }

        // update entity info, step physics, and handle events
        b2World_Step(e->worldID, DELTA_TIME, BOX2D_SUBSTEPS);

        // handle sudden death
        e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
        if (e->stepsLeft == 0)
        {
            e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
            if (e->suddenDeathSteps == 0)
            {
                DEBUG_LOG("placing sudden death walls");
                handleSuddenDeath(e);
                e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
            }
        }

        bool terminate = false;
        for (size_t i = 0; i < cc_array_size(e->drones); i++)
        {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            droneStep(drone, DELTA_TIME);
            if (drone->dead)
            {
                memset(e->terminals, 1, NUM_DRONES * sizeof(bool));
                terminate = true;
            }
        }

        projectilesStep(e);
        weaponPickupsStep(e, DELTA_TIME);

        handleContactEvents(e);
        handleSensorEvents(e);

        computeRewards(e);

        if (terminate)
        {
            e->needsReset = true;
            break;
        }
    }

    computeObs(e);
}

bool envTerminated(env *e)
{
    for (size_t i = 0; i < cc_array_size(e->drones); i++)
    {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead)
        {
            return true;
        }
    }
    return false;
}

#endif
