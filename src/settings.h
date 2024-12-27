#pragma once
#ifndef IMPULSE_WARS_SETTINGS_H
#define IMPULSE_WARS_SETTINGS_H

#include "helpers.h"
#include "types.h"

#define INFINITE -1

// general settings
#define FRAME_RATE 60.0f
#define DELTA_TIME 1.0f / FRAME_RATE

#define BOX2D_SUBSTEPS 4

#define FRAMESKIP 4

#define _MAX_MAP_COLUMNS 21
#define _MAX_MAP_ROWS 21
#define MAX_CELLS _MAX_MAP_COLUMNS *_MAX_MAP_ROWS + 1

#define MIN_SPAWN_DISTANCE 6.0f

#define ROUND_STEPS 91.0f * FRAME_RATE
#define SUDDEN_DEATH_STEPS 5.0f * FRAME_RATE

const uint8_t MAX_DRONES = _MAX_DRONES;

#define EXPLOSION_STEPS 5

const uint16_t LOG_BUFFER_SIZE = 1024;

// env constants
#define DEFAULT_LIVES 5

// reward settings
#define WIN_REWARD 1.0f
#define KILL_REWARD 0.5f
#define DEATH_REWARD 0.0f
#define SHOT_HIT_REWARD_COEF 1.0f
#define PICKUP_REWARD 0.1f

// observation constants
const uint8_t MAX_MAP_COLUMNS = _MAX_MAP_COLUMNS;
const uint8_t MAX_MAP_ROWS = _MAX_MAP_ROWS;
const uint8_t PROJECTILE_OBS_OFFSET = 2;
const uint8_t FLOATING_WALL_OBS_OFFSET = PROJECTILE_OBS_OFFSET + 1;
const uint8_t DRONE_OBS_OFFSET = FLOATING_WALL_OBS_OFFSET + 1;
const uint8_t MAP_CELL_OBS_SIZE = DRONE_OBS_OFFSET + 1;
const uint16_t MAP_OBS_SIZE = MAP_CELL_OBS_SIZE * MAX_MAP_COLUMNS * MAX_MAP_ROWS;
const uint8_t DRONE_OBS_SIZE = 9;
const uint8_t SCALAR_OBS_SIZE = DRONE_OBS_SIZE + NUM_WEAPONS + 1;
const uint16_t OBS_SIZE = MAP_OBS_SIZE + SCALAR_OBS_SIZE;

#define MAX_X_POS 40.0f
#define MAX_Y_POS 40.0f
#define MAX_SPEED 250.0f

const uint8_t ACTION_SIZE = 5;

// wall settings
#define WALL_THICKNESS 4.0f
#define FLOATING_WALL_THICKNESS 3.0f
#define FLOATING_WALL_DAMPING 0.5f
#define BOUNCY_WALL_RESTITUTION 1.0f
#define WALL_DENSITY 4.0f

// weapon pickup settings
#define PICKUP_THICKNESS 3.0f
#define PICKUP_RESPAWN_WAIT 1.0f

// drone settings
#define DRONE_WALL_SPAWN_DISTANCE 7.5f
#define DRONE_DRONE_SPAWN_DISTANCE 20.0f
#define DRONE_RADIUS 1.0f
#define DRONE_DENSITY 1.25f
#define DRONE_MOVE_MAGNITUDE 25.0f
#define DRONE_LINEAR_DAMPING 1.0f
#define DRONE_MOVE_AIM_DIVISOR 10.0f

// weapon projectile settings
#define STANDARD_AMMO INFINITE
#define STANDARD_PROJECTILES 1
#define STANDARD_RECOIL_MAGNITUDE 12.5f
#define STANDARD_FIRE_MAGNITUDE 15.5f
#define STANDARD_CHARGE 0.0f
#define STANDARD_COOL_DOWN 0.37f
#define STANDARD_MAX_DISTANCE 80.0f
#define STANDARD_RADIUS 0.2
#define STANDARD_DENSITY 3.0f
#define STANDARD_INV_MASS INV_MASS(STANDARD_DENSITY, STANDARD_RADIUS)
#define STANDARD_BOUNCE 2

#define MACHINEGUN_AMMO 35
#define MACHINEGUN_PROJECTILES 1
#define MACHINEGUN_RECOIL_MAGNITUDE 8.0f
#define MACHINEGUN_FIRE_MAGNITUDE 20.0f
#define MACHINEGUN_CHARGE 0.0f
#define MACHINEGUN_COOL_DOWN 0.07f
#define MACHINEGUN_MAX_DISTANCE 225.0f
#define MACHINEGUN_RADIUS 0.15f
#define MACHINEGUN_DENSITY 3.0f
#define MACHINEGUN_INV_MASS INV_MASS(MACHINEGUN_DENSITY, MACHINEGUN_RADIUS)
#define MACHINEGUN_BOUNCE 1

#define SNIPER_AMMO 3
#define SNIPER_PROJECTILES 1
#define SNIPER_RECOIL_MAGNITUDE 60.0f
#define SNIPER_FIRE_MAGNITUDE 200.0f
#define SNIPER_CHARGE 1.0f
#define SNIPER_COOL_DOWN 1.5f
#define SNIPER_MAX_DISTANCE INFINITE
#define SNIPER_RADIUS 0.5f
#define SNIPER_DENSITY 1.5f
#define SNIPER_INV_MASS INV_MASS(SNIPER_DENSITY, SNIPER_RADIUS)
#define SNIPER_BOUNCE 0

#define SHOTGUN_AMMO 8
#define SHOTGUN_PROJECTILES 8
#define SHOTGUN_RECOIL_MAGNITUDE 75.0f
#define SHOTGUN_FIRE_MAGNITUDE 20.0f
#define SHOTGUN_CHARGE 0.0f
#define SHOTGUN_COOL_DOWN 1.0f
#define SHOTGUN_MAX_DISTANCE 100.0f
#define SHOTGUN_RADIUS 0.15f
#define SHOTGUN_DENSITY 3.0f
#define SHOTGUN_INV_MASS INV_MASS(SHOTGUN_DENSITY, SHOTGUN_RADIUS)
#define SHOTGUN_BOUNCE 1

#define IMPLODER_AMMO 1
#define IMPLODER_PROJECTILES 1
#define IMPLODER_RECOIL_MAGNITUDE 35.0f
#define IMPLODER_FIRE_MAGNITUDE 25.0f
#define IMPLODER_CHARGE 2.0f
#define IMPLODER_COOL_DOWN 0.0f
#define IMPLODER_MAX_DISTANCE INFINITE
#define IMPLODER_RADIUS 0.8f
#define IMPLODER_DENSITY 1.0f
#define IMPLODER_INV_MASS INV_MASS(IMPLODER_DENSITY, IMPLODER_RADIUS)
#define IMPLODER_BOUNCE 0

const weaponInformation standard = {
    .type = STANDARD_WEAPON,
    .isPhysicsBullet = true,
    .numProjectiles = STANDARD_PROJECTILES,
    .recoilMagnitude = STANDARD_RECOIL_MAGNITUDE,
    .coolDown = STANDARD_COOL_DOWN,
    .maxDistance = STANDARD_MAX_DISTANCE,
    .radius = STANDARD_RADIUS,
    .density = STANDARD_DENSITY,
    .invMass = STANDARD_INV_MASS,
    .maxBounces = STANDARD_BOUNCE + 1,
};

const weaponInformation machineGun = {
    .type = MACHINEGUN_WEAPON,
    .isPhysicsBullet = true,
    .numProjectiles = MACHINEGUN_PROJECTILES,
    .recoilMagnitude = MACHINEGUN_RECOIL_MAGNITUDE,
    .coolDown = MACHINEGUN_COOL_DOWN,
    .maxDistance = MACHINEGUN_MAX_DISTANCE,
    .radius = MACHINEGUN_RADIUS,
    .density = MACHINEGUN_DENSITY,
    .invMass = MACHINEGUN_INV_MASS,
    .maxBounces = MACHINEGUN_BOUNCE + 1,
};

const weaponInformation sniper = {
    .type = SNIPER_WEAPON,
    .isPhysicsBullet = true,
    .numProjectiles = SNIPER_PROJECTILES,
    .recoilMagnitude = SNIPER_RECOIL_MAGNITUDE,
    .coolDown = SNIPER_COOL_DOWN,
    .maxDistance = SNIPER_MAX_DISTANCE,
    .radius = SNIPER_RADIUS,
    .density = SNIPER_DENSITY,
    .invMass = SNIPER_INV_MASS,
    .maxBounces = SNIPER_BOUNCE + 1,
};

const weaponInformation shotgun = {
    .type = SHOTGUN_WEAPON,
    .isPhysicsBullet = true,
    .numProjectiles = SHOTGUN_PROJECTILES,
    .recoilMagnitude = SHOTGUN_RECOIL_MAGNITUDE,
    .coolDown = SHOTGUN_COOL_DOWN,
    .maxDistance = SHOTGUN_MAX_DISTANCE,
    .radius = SHOTGUN_RADIUS,
    .density = SHOTGUN_DENSITY,
    .invMass = SHOTGUN_INV_MASS,
    .maxBounces = SHOTGUN_BOUNCE + 1,
};

const weaponInformation imploder = {
    .type = IMPLODER_WEAPON,
    .isPhysicsBullet = false,
    .numProjectiles = IMPLODER_PROJECTILES,
    .recoilMagnitude = IMPLODER_RECOIL_MAGNITUDE,
    .coolDown = IMPLODER_COOL_DOWN,
    .maxDistance = IMPLODER_MAX_DISTANCE,
    .radius = IMPLODER_RADIUS,
    .density = IMPLODER_DENSITY,
    .invMass = IMPLODER_INV_MASS,
    .maxBounces = IMPLODER_BOUNCE + 1,
};

#ifndef AUTOPXD
weaponInformation *weaponInfos[] = {
    (weaponInformation *)&standard,
    (weaponInformation *)&machineGun,
    (weaponInformation *)&sniper,
    (weaponInformation *)&shotgun,
    (weaponInformation *)&imploder,
};
#endif

// max ammo of weapon
int8_t weaponAmmo(const enum weaponType defaultWep, const enum weaponType type) {
    if (type == defaultWep) {
        return INFINITE;
    }
    switch (type) {
    case STANDARD_WEAPON:
        return STANDARD_AMMO;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_AMMO;
    case SNIPER_WEAPON:
        return SNIPER_AMMO;
    case SHOTGUN_WEAPON:
        return SHOTGUN_AMMO;
    case IMPLODER_WEAPON:
        return IMPLODER_AMMO;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// amount of force to apply to projectile
float weaponFire(uint64_t *seed, const enum weaponType type) {
    switch (type) {
    case STANDARD_WEAPON:
        return STANDARD_FIRE_MAGNITUDE;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_FIRE_MAGNITUDE;
    case SNIPER_WEAPON:
        return SNIPER_FIRE_MAGNITUDE;
    case SHOTGUN_WEAPON: {
        const int maxOffset = 3;
        const int fireOffset = randInt(seed, -maxOffset, maxOffset);
        return SHOTGUN_FIRE_MAGNITUDE + fireOffset;
    }
    case IMPLODER_WEAPON:
        return IMPLODER_FIRE_MAGNITUDE;
    default:
        ERRORF("unknown weapon type %d", type);
        return 0;
    }
}

// how many steps the weapon needs to be charged for before it can be fired
uint16_t weaponCharge(const enum weaponType type) {
    float charge = 0.0f;
    switch (type) {
    case STANDARD_WEAPON:
        charge = STANDARD_CHARGE;
        break;
    case MACHINEGUN_WEAPON:
        charge = MACHINEGUN_CHARGE;
        break;
    case SNIPER_WEAPON:
        charge = SNIPER_CHARGE;
        break;
    case SHOTGUN_WEAPON:
        charge = SHOTGUN_CHARGE;
        break;
    case IMPLODER_WEAPON:
        charge = IMPLODER_CHARGE;
        break;
    default:
        ERRORF("unknown weapon type %d", type);
    }

    return (uint16_t)(charge * FRAME_RATE);
}

b2Vec2 weaponAdjustAim(uint64_t *seed, const enum weaponType type, const uint16_t heat, const b2Vec2 normAim) {
    switch (type) {
    case STANDARD_WEAPON:
        return normAim;
    case MACHINEGUN_WEAPON: {
        const float swayCoef = logBasef((heat / 5.0f) + 1, 180);
        const float maxSway = 0.15f;
        const float swayX = randFloat(seed, maxSway * -swayCoef, maxSway * swayCoef);
        const float swayY = randFloat(seed, maxSway * -swayCoef, maxSway * swayCoef);
        b2Vec2 machinegunAim = {.x = normAim.x + swayX, .y = normAim.y + swayY};
        return b2Normalize(machinegunAim);
    }
    case SNIPER_WEAPON:
        return normAim;
    case SHOTGUN_WEAPON: {
        const float maxOffset = 0.15f;
        const float offsetX = randFloat(seed, -maxOffset, maxOffset);
        const float offsetY = randFloat(seed, -maxOffset, maxOffset);
        b2Vec2 shotgunAim = {.x = normAim.x + offsetX, .y = normAim.y + offsetY};
        return b2Normalize(shotgunAim);
    }
    case IMPLODER_WEAPON:
        return normAim;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// sets explosion parameters and returns true if an explosion should be created
// when a projectile is destroyed
bool weaponExplosion(const enum weaponType type, b2ExplosionDef *explosionDef) {
    switch (type) {
    case IMPLODER_WEAPON:
        explosionDef->radius = 10.0f;
        explosionDef->falloff = 5.0f;
        explosionDef->impulsePerLength = -150.0f;
        return true;
    default:
        return false;
    }
}

#endif
