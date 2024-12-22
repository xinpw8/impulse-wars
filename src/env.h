#pragma once
#ifndef IMPULSE_WARS_ENV_H
#define IMPULSE_WARS_ENV_H

#include "game.h"
#include "map.h"
#include "settings.h"
#include "types.h"

// autopdx can't parse raylib's headers for some reason, but that's ok
// because the Cython code doesn't need to directly use raylib anyway
// include the full render header if we're compiling C code, otherwise
// just declare the necessary functions the Cython code needs
#ifndef AUTOPXD
#include "render.h"
#else
rayClient *createRayClient();
void destroyRayClient(rayClient *client);
#endif

logBuffer *createLogBuffer(uint16_t capacity)
{
    logBuffer *logs = (logBuffer *)fastCalloc(1, sizeof(logBuffer));
    logs->logs = (logEntry *)fastCalloc(capacity, sizeof(logEntry));
    logs->size = 0;
    logs->capacity = capacity;
    return logs;
}

void destroyLogBuffer(logBuffer *buffer)
{
    fastFree(buffer->logs);
    fastFree(buffer);
}

void addLogEntry(logBuffer *logs, logEntry *log)
{
    if (logs->size == logs->capacity)
    {
        return;
    }
    logs->logs[logs->size] = *log;
    logs->size += 1;
}

logEntry aggregateAndClearLogBuffer(uint8_t numDrones, logBuffer *logs)
{
    logEntry log = {0};
    if (logs->size == 0)
    {
        return log;
    }

    DEBUG_LOGF("aggregating logs, size: %d", logs->size);

    const float logSize = logs->size;
    for (uint16_t i = 0; i < logs->size; i++)
    {
        log.length += logs->logs[i].length / logSize;
        log.winner += logs->logs[i].winner / logSize;

        for (uint8_t j = 0; j < numDrones; j++)
        {
            log.reward[j] += logs->logs[i].reward[j] / logSize;

            for (uint8_t k = 0; k < NUM_WEAPONS; k++)
            {
                log.stats[j].distanceTraveled += logs->logs[i].stats[j].distanceTraveled / logSize;
                log.stats[j].shotsFired[k] += logs->logs[i].stats[j].shotsFired[k] / logSize;
                log.stats[j].shotsHit[k] += logs->logs[i].stats[j].shotsHit[k] / logSize;
                log.stats[j].shotsTaken[k] += logs->logs[i].stats[j].shotsTaken[k] / logSize;
                log.stats[j].ownShotsTaken[k] += logs->logs[i].stats[j].ownShotsTaken[k] / logSize;
                log.stats[j].weaponsPickedUp[k] += logs->logs[i].stats[j].weaponsPickedUp[k] / logSize;
                log.stats[j].shotDistances[k] += logs->logs[i].stats[j].shotDistances[k] / logSize;
            }
        }
    }

    logs->size = 0;
    return log;
}

// TODO: can posToCellIdx be replaced with this?
static inline uint16_t entityPosToCellIdx(const env *e, const b2Vec2 pos)
{
    float cellX = pos.x + (((float)e->columns * WALL_THICKNESS) / 2.0f);
    float cellY = pos.y + (((float)e->rows * WALL_THICKNESS) / 2.0f);
    uint16_t cellCol = cellX / WALL_THICKNESS;
    uint16_t cellRow = cellY / WALL_THICKNESS;
    return cellCol + (cellRow * e->columns);
}

void computeObs(env *e)
{
    uint16_t offset = 0;

    // add scalar observations
    e->obs[offset++] = scaleValue(e->stepsLeft, ROUND_STEPS, true);

    // compute drone observations
    for (uint8_t i = 0; i < e->numDrones; i++)
    {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        const b2Vec2 pos = getCachedPos(drone->bodyID, &drone->pos);

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
        e->obs[offset++] = scaleValue(drone->lastVelocity.y, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(drone->lastAim.x, 1.0f, false);
        e->obs[offset++] = scaleValue(drone->lastAim.y, 1.0f, false);
        e->obs[offset++] = scaledAmmo;
        e->obs[offset++] = scaleValue(drone->weaponCooldown, drone->weaponInfo->coolDown, true);
        e->obs[offset++] = scaleValue(drone->charge, weaponCharge(drone->weaponInfo->type), true);

        ASSERT(i * DRONE_OBS_SIZE <= e->numDrones * DRONE_OBS_SIZE);
        ASSERT(offset <= e->obsInfo.droneObsOffset);
    }
    ASSERT(offset == e->obsInfo.droneObsOffset);

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
        ASSERT(offset <= e->obsInfo.projectileObsOffset);
        projIdx++;
    }
    // zero out any remaining projectile observations
    const uint16_t projectileObsSet = abs(offset - e->obsInfo.droneObsOffset);
    const uint16_t projectileObsUnset = (NUM_PROJECTILE_OBS * PROJECTILE_OBS_SIZE) - projectileObsSet;
    if (projectileObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, projectileObsUnset * sizeof(float));
        offset += projectileObsUnset;
    }
    ASSERT(offset == e->obsInfo.projectileObsOffset);

    // compute floating wall observations
    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++)
    {
        wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        const b2Vec2 pos = getCachedPos(wall->bodyID, &wall->pos);
        const b2Vec2 vel = b2Body_GetLinearVelocity(wall->bodyID);
        const float angle = b2Rot_GetAngle(b2Body_GetRotation(wall->bodyID)) * RAD2DEG;

        // will be processed in an embedding layer separately
        // 1 doesn't need to be added here as the first valid wall type is 1 already
        e->obs[offset++] = wall->type;
        e->obs[offset++] = scaleValue(pos.x, MAX_X_POS, false);
        e->obs[offset++] = scaleValue(pos.y, MAX_Y_POS, false);
        e->obs[offset++] = scaleValue(vel.x, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(vel.y, MAX_SPEED, false);
        e->obs[offset++] = scaleValue(angle, 180.0f, false);

        ASSERT(i * FLOATING_WALL_OBS_SIZE <= NUM_FLOATING_WALL_OBS * FLOATING_WALL_OBS_SIZE);
        ASSERT(offset <= e->obsInfo.floatingWallObsOffset);
    }
    // zero out any remaining floating wall observations
    const uint16_t floatingWallObsSet = abs(offset - e->obsInfo.projectileObsOffset);
    const uint16_t floatingWallObsUnset = (NUM_FLOATING_WALL_OBS * FLOATING_WALL_OBS_SIZE) - floatingWallObsSet;
    if (floatingWallObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, floatingWallObsUnset * sizeof(float));
        offset += floatingWallObsUnset;
    }
    ASSERT(offset == e->obsInfo.floatingWallObsOffset);

    // compute map cell observations
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
                ASSERTF(cellType <= OBS_HIGH, "cellType %f", cellType);
            }
        }
        // will be processed in an embedding layer separately
        e->obs[offset++] = cellType;

        ASSERT(i < MAX_MAP_COLUMNS * MAX_MAP_ROWS);
        ASSERT(offset <= e->obsInfo.obsSize);
    }
    // TODO: should only need to do this once after loading a map
    // zero out any remaining map cell observations
    const uint16_t mapCellObsSet = abs(offset - e->obsInfo.floatingWallObsOffset);
    const uint16_t mapCellObsUnset = (MAX_MAP_COLUMNS * MAX_MAP_ROWS) - mapCellObsSet;
    if (mapCellObsUnset != 0)
    {
        memset(e->obs + offset, 0x0, mapCellObsUnset * sizeof(float));
        offset += mapCellObsUnset;
    }
    ASSERT(offset == e->obsInfo.obsSize);

    // add drones and floating walls to map cells
    // add floating walls first so that we can ensure drones will
    // always be present in the map cell observation
    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++)
    {
        const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        const uint16_t cellIdx = entityPosToCellIdx(e, wall->pos.pos);
        const float cellType = (float)(wall->type + 1);
        ASSERTF(cellType <= OBS_HIGH, "cellType %f", cellType);
        e->obs[e->obsInfo.floatingWallObsOffset + cellIdx] = cellType;
    }
    for (uint8_t i = 0; i < e->numDrones; i++)
    {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        const uint16_t cellIdx = entityPosToCellIdx(e, drone->pos.pos);
        const float cellType = (float)(DRONE_ENTITY + i);
        ASSERTF(cellType <= OBS_HIGH, "cellType %f", cellType);
        e->obs[e->obsInfo.floatingWallObsOffset + cellIdx] = cellType;
    }

    // copy observations for other agents
    for (uint8_t agentIdx = 1; agentIdx < e->numAgents; agentIdx++)
    {
        const uint16_t obsOffset = e->obsInfo.obsSize * agentIdx;
        // move the active agent's drone observation to the front
        memcpy(
            e->obs + obsOffset + e->obsInfo.scalarObsOffset,
            e->obs + e->obsInfo.scalarObsOffset + (agentIdx * DRONE_OBS_SIZE),
            DRONE_OBS_SIZE * sizeof(float));

        // copy the rest of the observations over unmodified
        memcpy(
            e->obs + obsOffset + e->obsInfo.droneObsOffset,
            e->obs + e->obsInfo.droneObsOffset,
            (e->obsInfo.obsSize - e->obsInfo.droneObsOffset) * sizeof(float));

        uint8_t newIdx = 1;
        uint16_t droneObsOffset = obsOffset + e->obsInfo.scalarObsOffset + DRONE_OBS_SIZE;
        for (uint8_t curDroneIdx = 0; curDroneIdx < e->numDrones; curDroneIdx++)
        {
            // copy the rest of the drone observations over in order,
            // skipping the active agent's drone which is already in the front
            if (curDroneIdx != agentIdx)
            {
                memcpy(
                    e->obs + droneObsOffset,
                    e->obs + e->obsInfo.scalarObsOffset + (curDroneIdx * DRONE_OBS_SIZE),
                    DRONE_OBS_SIZE * sizeof(float));
                droneObsOffset += DRONE_OBS_SIZE;
            }

            const droneEntity *drone = safe_array_get_at(e->drones, curDroneIdx);
            const uint16_t cellIdx = entityPosToCellIdx(e, drone->pos.pos);
            // label the agent's drone as drone 0 so observations are consistent
            // between agents
            uint8_t activeIdx = 0;
            if (curDroneIdx != agentIdx)
            {
                activeIdx = newIdx++;
            }
            const float cellType = (float)(DRONE_ENTITY + activeIdx);
            ASSERTF(cellType <= OBS_HIGH, "cellType %f", cellType);
            e->obs[obsOffset + e->obsInfo.floatingWallObsOffset + cellIdx] = cellType;
        }
    }
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
    const int mapIdx = 1; // randInt(&e->randState, 0, NUM_MAPS - 1);
    createMap(e, mapIdx);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_array_size(e->walls); i++)
    {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        bounds.min.x = fminf(wall->pos.pos.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->pos.pos.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->pos.pos.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->pos.pos.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    DEBUG_LOG("creating drones");
    for (int i = 0; i < e->numDrones; i++)
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

env *initEnv(env *e, uint8_t numDrones, uint8_t numAgents, float *obs, float *actions, float *rewards, uint8_t *terminals, logBuffer *logs, uint64_t seed)
{
    e->numDrones = numDrones;
    e->numAgents = numAgents;
    e->obsInfo = calculateObservationInfo(numDrones);

    e->obs = obs;
    e->actions = actions;
    e->rewards = rewards;
    e->terminals = terminals;

    e->randState = seed;
    e->needsReset = false;

    e->logs = logs;

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
    memset(e->obs, 0x0, e->obsInfo.obsSize * e->numAgents * sizeof(float));
    // rewards get cleared in stepEnv every step
    memset(e->terminals, 0x0, e->numAgents * sizeof(uint8_t));

    memset(e->episodeReward, 0x0, e->numAgents * sizeof(float));
    e->episodeLength = 0;
    memset(e->stats, 0x0, sizeof(e->stats));

    for (size_t i = 0; i < cc_array_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        destroyWeaponPickup(pickup);
    }

    for (uint8_t i = 0; i < e->numDrones; i++)
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

void computeShotHitReward(env *e, const droneEntity *drone, const uint8_t enemyIdx)
{
    // compute reward based off of how much the projectile(s) or explosion(s)
    // caused the enemy drone to change velocity
    const droneEntity *enemyDrone = safe_array_get_at(e->drones, enemyIdx);
    const float prevEnemySpeed = b2Length(enemyDrone->lastVelocity);
    const float curEnemySpeed = b2Length(b2Body_GetLinearVelocity(enemyDrone->bodyID));
    e->rewards[drone->idx] += scaleValue(fabsf(curEnemySpeed - prevEnemySpeed), MAX_SPEED, true) * SHOT_HIT_REWARD_COEF;
}

void computeReward(env *e, const droneEntity *drone)
{
    if (drone->dead)
    {
        e->rewards[drone->idx] += DEATH_REWARD;
    }
    // TODO: compute kill reward
    for (uint8_t i = 0; i < e->numDrones; i++)
    {
        if (i == drone->idx)
        {
            continue;
        }
        if (drone->hitInfo.shotHit[i])
        {
            computeShotHitReward(e, drone, i);
        }
        if (drone->hitInfo.explosionHit[i])
        {
            computeShotHitReward(e, drone, i);
        }
    }
}

void computeRewards(env *e, const bool roundOver, const uint8_t winner)
{
    if (roundOver)
    {
        e->rewards[winner] += WIN_REWARD;
    }

    for (int i = 0; i < e->numAgents; i++)
    {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        computeReward(e, drone);
        e->episodeReward[i] += e->rewards[i];

        if (e->rewards[i] != 0.0f)
        {
            DEBUG_LOGF("reward[%d]: %f", i, e->rewards[i]);
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

    // reset reward buffer
    memset(e->rewards, 0x0, e->numAgents * sizeof(float));

    for (int i = 0; i < FRAMESKIP; i++)
    {
        e->episodeLength++;

        // handle actions
        for (uint8_t i = 0; i < e->numDrones; i++)
        {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->lastVelocity = b2Body_GetLinearVelocity(drone->bodyID);
            memset(&drone->hitInfo, 0x0, sizeof(stepHitInfo));

            if (i >= e->numAgents)
            {
                break;
            }

            const uint8_t offset = i * ACTION_SIZE;
            const b2Vec2 rawMove = (b2Vec2){.x = e->actions[offset + 0], .y = e->actions[offset + 1]};
            ASSERT_VEC_NORMALIZED(rawMove);
            const b2Vec2 move = b2Normalize(rawMove);
            const b2Vec2 rawAim = (b2Vec2){.x = e->actions[offset + 2], .y = e->actions[offset + 3]};
            ASSERT_VEC_NORMALIZED(rawAim);
            const b2Vec2 aim = b2Normalize(rawAim);
            const bool shoot = e->actions[offset + 4] > 0.0f;

            if (!b2VecEqual(move, b2Vec2_zero))
            {
                droneMove(drone, move);
            }
            drone->lastMove = move;
            if (shoot)
            {
                droneShoot(e, drone, aim);
            }
            if (!b2VecEqual(aim, b2Vec2_zero))
            {
                drone->lastAim = b2Normalize(aim);
            }
        }

        // update entity info, step physics, and handle events
        b2World_Step(e->worldID, DELTA_TIME, BOX2D_SUBSTEPS);

        // mark old positions as invalid now that physics has been stepped
        // projectiles will have their positions correctly updated in projectilesStep
        for (uint8_t i = 0; i < e->numDrones; i++)
        {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->pos.valid = false;
        }
        for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++)
        {
            wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
            wall->pos.valid = false;
        }

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

        projectilesStep(e);

        handleContactEvents(e);
        handleSensorEvents(e);

        uint8_t lastAlive = 0;
        uint8_t deadDrones = 0;
        for (uint8_t i = 0; i < e->numDrones; i++)
        {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            droneStep(e, drone, DELTA_TIME);
            if (drone->dead)
            {
                deadDrones++;
                e->terminals[i] = 1;
            }
            else
            {
                lastAlive = i;
            }
        }

        weaponPickupsStep(e, DELTA_TIME);

        const bool roundOver = deadDrones >= e->numDrones - 1;
        computeRewards(e, roundOver, lastAlive);

        if (e->client != NULL)
        {
            renderEnv(e);
        }

        if (roundOver)
        {
            // add existing projectile distances to stats
            for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next)
            {
                const projectileEntity *projectile = (projectileEntity *)cur->data;
                e->stats[projectile->droneIdx].shotDistances[projectile->weaponInfo->type] += projectile->distance;
            }

            logEntry log = {0};
            memcpy(log.reward, e->episodeReward, sizeof(e->episodeReward));
            log.length = e->episodeLength;
            memcpy(log.stats, e->stats, sizeof(e->stats));
            addLogEntry(e->logs, &log);

            for (uint8_t i = 0; i < e->numAgents; i++)
            {
                DEBUG_LOGF("drone %d total reward: %f", i, log.reward[i]);
            }

            e->needsReset = true;
            break;
        }
    }

    computeObs(e);
}

bool envTerminated(env *e)
{
    for (uint8_t i = 0; i < e->numDrones; i++)
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
