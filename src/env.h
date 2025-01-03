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

static inline b2Vec2 b2Rotate(b2Vec2 v, float angle) {
    float cosAngle = cosf(angle);
    float sinAngle = sinf(angle);
    return (b2Vec2){
        .x = v.x * cosAngle - v.y * sinAngle,
        .y = v.x * sinAngle + v.y * cosAngle
    };
}

logBuffer *createLogBuffer(uint16_t capacity) {
    logBuffer *logs = (logBuffer *)fastCalloc(1, sizeof(logBuffer));
    logs->logs = (logEntry *)fastCalloc(capacity, sizeof(logEntry));
    logs->size = 0;
    logs->capacity = capacity;
    return logs;
}

void destroyLogBuffer(logBuffer *buffer) {
    fastFree(buffer->logs);
    fastFree(buffer);
}

void addLogEntry(logBuffer *logs, logEntry *log) {
    if (logs->size == logs->capacity) {
        return;
    }
    logs->logs[logs->size] = *log;
    logs->size += 1;
}

logEntry aggregateAndClearLogBuffer(uint8_t numDrones, logBuffer *logs) {
    logEntry log = {0};
    if (logs->size == 0) {
        return log;
    }

    DEBUG_LOGF("aggregating logs, size: %d", logs->size);

    const float logSize = logs->size;
    for (uint16_t i = 0; i < logs->size; i++) {
        log.length += logs->logs[i].length / logSize;

        for (uint8_t j = 0; j < numDrones; j++) {
            log.stats[j].reward += logs->logs[i].stats[j].reward / logSize;
            log.stats[j].wins += logs->logs[i].stats[j].wins / logSize;

            for (uint8_t k = 0; k < NUM_WEAPONS; k++) {
                log.stats[j].distanceTraveled += logs->logs[i].stats[j].distanceTraveled / logSize;
                log.stats[j].absDistanceTraveled += logs->logs[i].stats[j].absDistanceTraveled / logSize;
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

void computeObs(env *e) {
    memset(e->obs, 0x0, OBS_SIZE * e->numAgents * sizeof(uint8_t));

    for (uint8_t agent = 0; agent < e->numAgents; agent++) {
        uint16_t offset = OBS_SIZE * agent;
        const uint16_t obsStart = offset;

        // compute map wall observations
        // TODO: needs to be padded for smaller maps then max size
        for (size_t i = 0; i < cc_array_size(e->cells); i++) {
            const mapCell *cell = safe_array_get_at(e->cells, i);

            if (cell->ent != NULL) {
                uint8_t wallType = 0;
                if (entityTypeIsWall(cell->ent->type)) {
                    wallType = cell->ent->type + 1;
                }
                e->obs[offset++] = wallType;

                uint8_t pickupWeaponType = 0;
                if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
                    weaponPickupEntity *pickup = (weaponPickupEntity *)cell->ent->entity;
                    pickupWeaponType = pickup->weapon + 1;
                }
                e->obs[offset++] = pickupWeaponType;
                offset += MAP_CELL_OBS_SIZE - 2;
            } else {
                offset += MAP_CELL_OBS_SIZE;
            }

            ASSERT(i <= MAX_MAP_COLUMNS * MAX_MAP_ROWS);
            ASSERT(offset <= (OBS_SIZE * agent) + MAP_OBS_SIZE);
        }

        // compute projectile observations
        for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next) {
            const projectileEntity *projectile = (projectileEntity *)cur->data;
            const int16_t cellIdx = entityPosToCellIdx(e, projectile->lastPos);
            if (cellIdx == -1) {
                continue;
            }
            // don't add the projectile to the obs if it somehow
            // overlaps with a static wall
            const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
            if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
                continue;
            }

            const uint8_t projWeapon = projectile->weaponInfo->type + 1;
            ASSERT(projWeapon <= NUM_WEAPONS + 1);
            const uint16_t offset = obsStart + (cellIdx * MAP_CELL_OBS_SIZE) + PROJECTILE_OBS_OFFSET;
            ASSERTF(offset <= OBS_SIZE * (agent + 1), "offset: %d, max offset: %d, last pos: %f %f", offset, OBS_SIZE * (agent + 1), projectile->lastPos.x, projectile->lastPos.y);
            e->obs[offset] = projWeapon;
        }
        // compute floating wall observations
        for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
            const wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
            const int16_t cellIdx = entityPosToCellIdx(e, wall->pos.pos);
            if (cellIdx == -1) {
                continue;
            }
            // don't add the floating wall to the obs if it somehow
            // overlaps with a static wall
            const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
            if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
                continue;
            }

            const uint8_t wallType = wall->type + 1;
            ASSERT(wallType <= NUM_WALL_TYPES + 1);
            const uint16_t offset = obsStart + (cellIdx * MAP_CELL_OBS_SIZE) + FLOATING_WALL_OBS_OFFSET;
            ASSERT(offset <= OBS_SIZE * (agent + 1));
            e->obs[offset] = wallType;
        }
        // compute drone observations
        for (uint8_t i = 0; i < e->numDrones; i++) {
            const droneEntity *drone = safe_array_get_at(e->drones, i);
            const int16_t cellIdx = entityPosToCellIdx(e, drone->pos.pos);
            if (cellIdx == -1) {
                continue;
            }
            // don't add the drone to the obs if it somehow
            // overlaps with a static wall
            const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
            if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
                continue;
            }

            const uint8_t droneWeapon = drone->weaponInfo->type + 1;
            ASSERT(droneWeapon <= NUM_WEAPONS + 1);
            const uint16_t offset = obsStart + (cellIdx * MAP_CELL_OBS_SIZE) + DRONE_OBS_OFFSET;
            ASSERT(offset <= OBS_SIZE * (agent + 1));
            e->obs[offset] = droneWeapon;
        }

        // compute active drone observations
        offset = obsStart + MAP_OBS_SIZE;
        droneEntity *activeDrone = safe_array_get_at(e->drones, agent);
        const b2Vec2 pos = getCachedPos(activeDrone->bodyID, &activeDrone->pos);
        const b2Vec2 vel = b2Body_GetLinearVelocity(activeDrone->bodyID);

        int8_t maxAmmo = weaponAmmo(e->defaultWeapon->type, activeDrone->weaponInfo->type);
        uint8_t scaledAmmo = 0;
        if (activeDrone->ammo != INFINITE) {
            scaledAmmo = scaleValue(activeDrone->ammo, maxAmmo, true);
        }

        e->obs[offset++] = scaleValue(pos.x, MAX_X_POS, false) * 255;
        e->obs[offset++] = scaleValue(pos.y, MAX_Y_POS, false) * 255;
        e->obs[offset++] = scaleValue(vel.x, MAX_SPEED, false) * 255;
        e->obs[offset++] = scaleValue(vel.y, MAX_SPEED, false) * 255;
        e->obs[offset++] = scaleValue(activeDrone->lastAim.x, 1.0f, false) * 255;
        e->obs[offset++] = scaleValue(activeDrone->lastAim.y, 1.0f, false) * 255;
        e->obs[offset++] = scaledAmmo * 255;
        e->obs[offset++] = scaleValue(activeDrone->weaponCooldown, activeDrone->weaponInfo->coolDown, true) * 255;
        e->obs[offset++] = scaleValue(activeDrone->charge, weaponCharge(activeDrone->weaponInfo->type), true) * 255;
        oneHotEncode(e->obs, offset, activeDrone->weaponInfo->type, NUM_WEAPONS);
    }
}

void setupEnv(env *e) {
    e->needsReset = false;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){.x = 0.0f, .y = 0.0f};
    e->worldID = b2CreateWorld(&worldDef);

    e->stepsLeft = ROUND_STEPS;
    e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
    e->suddenDeathWallCounter = 0;

    DEBUG_LOG("creating map");
    const int mapIdx = 0; // randInt(&e->randState, 0, NUM_MAPS - 1);
    createMap(e, mapIdx);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        const wallEntity *wall = safe_array_get_at(e->walls, i);
        bounds.min.x = fminf(wall->pos.pos.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->pos.pos.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->pos.pos.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->pos.pos.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }
    e->bounds = bounds;

    DEBUG_LOG("creating drones");
    for (int i = 0; i < e->numDrones; i++) {
        createDrone(e, i);
    }

    DEBUG_LOG("placing floating walls");
    placeRandFloatingWalls(e, mapIdx);

    DEBUG_LOG("creating weapon pickups");
    for (int i = 0; i < maps[mapIdx]->weaponPickups; i++) {
        createWeaponPickup(e);
    }

    computeObs(e);
}

env *initEnv(env *e, uint8_t numDrones, uint8_t numAgents, uint8_t *obs, int *actions, float *rewards, uint8_t *terminals, logBuffer *logs, uint64_t seed) {
    e->numDrones = numDrones;
    e->numAgents = numAgents;

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

void clearEnv(env *e) {
    // rewards get cleared in stepEnv every step
    memset(e->terminals, 0x0, e->numAgents * sizeof(uint8_t));

    e->episodeLength = 0;
    memset(e->stats, 0x0, sizeof(e->stats));

    for (size_t i = 0; i < cc_array_size(e->pickups); i++) {
        weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
        destroyWeaponPickup(pickup);
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        destroyDrone(drone);
    }

    destroyAllProjectiles(e);

    for (size_t i = 0; i < cc_array_size(e->walls); i++) {
        wallEntity *wall = safe_array_get_at(e->walls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
        wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
        destroyWall(wall);
    }

    for (size_t i = 0; i < cc_array_size(e->cells); i++) {
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

void destroyEnv(env *e) {
    clearEnv(e);

    cc_array_destroy(e->cells);
    cc_array_destroy(e->walls);
    cc_array_destroy(e->floatingWalls);
    cc_array_destroy(e->drones);
    cc_array_destroy(e->pickups);
    cc_slist_destroy(e->projectiles);
}

void resetEnv(env *e) {
    clearEnv(e);
    setupEnv(e);
}

float computeShotHitReward(env *e, const uint8_t enemyIdx) {
    // compute reward based off of how much the projectile(s) or explosion(s)
    // caused the enemy drone to change velocity
    const droneEntity *enemyDrone = safe_array_get_at(e->drones, enemyIdx);
    const float prevEnemySpeed = b2Length(enemyDrone->lastVelocity);
    const float curEnemySpeed = b2Length(b2Body_GetLinearVelocity(enemyDrone->bodyID));
    return scaleValue(fabsf(curEnemySpeed - prevEnemySpeed), MAX_SPEED, true) * SHOT_HIT_REWARD_COEF;
}

float computeReward(env *e, const droneEntity *drone) {
    float reward = 0.0f;
    if (drone->dead) {
        reward += DEATH_REWARD;
    }
    // TODO: compute kill reward
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (i == drone->idx) {
            continue;
        }
        if (drone->hitInfo.shotHit[i]) {
            reward += computeShotHitReward(e, i);
        }
        if (drone->hitInfo.explosionHit[i]) {
            reward += computeShotHitReward(e, i);
        }
    }

    return reward;
}

void computeRewards(env *e, const bool roundOver, const uint8_t winner) {
    if (roundOver) {
        if (winner < e->numAgents) {
            e->rewards[winner] += WIN_REWARD;
        }
        e->stats[winner].reward += WIN_REWARD;
    }

    for (int i = 0; i < e->numDrones; i++) {
        const droneEntity *drone = safe_array_get_at(e->drones, i);
        const float reward = computeReward(e, drone);
        if (i < e->numAgents) {
            e->rewards[i] += reward;
        }
        e->stats[i].reward += reward;

        if (reward != 0.0f) {
            DEBUG_LOGF("reward[%d]: %f", i, reward);
        }
    }
}

void stepEnv(env *e) {
    if (e->needsReset) {
        DEBUG_LOG("Resetting environment");
        resetEnv(e);
    }

    // Reset reward buffer
    memset(e->rewards, 0x0, e->numAgents * sizeof(float));

    for (int frame = 0; frame < FRAMESKIP; frame++) {
        e->episodeLength++;

        // Handle actions
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->lastVelocity = b2Body_GetLinearVelocity(drone->bodyID);
            memset(&drone->hitInfo, 0x0, sizeof(stepHitInfo));

            if (i >= e->numAgents) {
                break;
            }

            const uint8_t offset = i * 4; // Each agent has 4 action components
            uint8_t aim_action = e->actions[offset + 0];
            uint8_t booster_action = e->actions[offset + 1];
            uint8_t fire_action = e->actions[offset + 2];
            uint8_t rotation_speed_action = e->actions[offset + 3];

            // Handle aiming
            float angle_step = 0.0f;
            switch (rotation_speed_action) {
                case 1: angle_step = 5.0f; break;  // Slow rotation
                case 2: angle_step = 15.0f; break; // Medium rotation
                case 3: angle_step = 30.0f; break; // Fast rotation
                default: break; // No-op
            }

            if (aim_action < 16) { // 16 valid aiming directions
                float angle = aim_action * (360.0f / 16.0f); // Map action to angle
                b2Vec2 aim = {cosf(DEG_TO_RAD(angle)), sinf(DEG_TO_RAD(angle))};
                // b2Vec2 normalized_aim = b2Normalize(aim);
                drone->lastAim = b2Rotate(drone->lastAim, angle_step); // Apply rotation step
            }

            // Handle booster impulse
            b2Vec2 boost = b2Vec2_zero;
            switch (booster_action) {
                case 1: boost = (b2Vec2){.x = -1.0f, .y = 0.0f}; break; // Left
                case 2: boost = (b2Vec2){.x = 1.0f, .y = 0.0f}; break;  // Right
                case 3: boost = (b2Vec2){.x = 0.0f, .y = -1.0f}; break; // Up
                case 4: boost = (b2Vec2){.x = 0.0f, .y = 1.0f}; break;  // Down
                default: break; // No-op
            }
            if (!b2VecEqual(boost, b2Vec2_zero)) {
                droneMove(drone, boost);
            }
            drone->lastMove = boost;

            // Handle firing weapon
            if (fire_action == 1) { // Fire
                droneShoot(e, drone, drone->lastAim);
            }
        }

        // Step physics and handle events
        b2World_Step(e->worldID, DELTA_TIME, BOX2D_SUBSTEPS);

        // Mark old positions as invalid
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            drone->pos.valid = false;
        }
        for (size_t i = 0; i < cc_array_size(e->floatingWalls); i++) {
            wallEntity *wall = safe_array_get_at(e->floatingWalls, i);
            wall->pos.valid = false;
        }

        // Handle sudden death logic
        e->stepsLeft = fmaxf(e->stepsLeft - 1, 0.0f);
        if (e->stepsLeft == 0) {
            e->suddenDeathSteps = fmaxf(e->suddenDeathSteps - 1, 0.0f);
            if (e->suddenDeathSteps == 0) {
                DEBUG_LOG("Placing sudden death walls");
                handleSuddenDeath(e);
                e->suddenDeathSteps = SUDDEN_DEATH_STEPS;
            }
        }

        // Update projectiles, sensors, and other events
        projectilesStep(e);
        handleContactEvents(e);
        handleSensorEvents(e);

        // Step drones and check for round end conditions
        uint8_t dronesAlive = 0;
        uint8_t lastAlive = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            droneEntity *drone = safe_array_get_at(e->drones, i);
            droneStep(e, drone, DELTA_TIME);
            if (!drone->dead) {
                dronesAlive++;
                lastAlive = i;
            }
        }

        weaponPickupsStep(e, DELTA_TIME);

        // Check if the round is over
        if (dronesAlive <= 1) {
            memset(e->terminals, 1, e->numAgents * sizeof(uint8_t));

            e->stats[lastAlive].wins = 1.0f;

            for (uint8_t i = 0; i < e->numDrones; i++) {
                droneEntity *drone = safe_array_get_at(e->drones, i);
                e->stats[i].absDistanceTraveled = b2Distance(drone->initalPos, drone->pos.pos);
            }

            logEntry log = {0};
            log.length = e->episodeLength;
            memcpy(log.stats, e->stats, sizeof(e->stats));
            addLogEntry(e->logs, &log);

            e->needsReset = true;
            break;
        }
    }

    // Compute observations for the next step
    computeObs(e);
}

bool envTerminated(env *e) {
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            return true;
        }
    }
    return false;
}

#endif
