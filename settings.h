#ifndef SETTINGS_H
#define SETTINGS_H

#include "helpers.h"

#define INFINITE_AMMO -1

// wall settings
#define BOUNCY_WALL_RESTITUTION 0.9
#define WALL_DENSITY 50.0f

// drone settings
#define DRONE_RADIUS 1.0f
#define DRONE_DENSITY 1.25f
#define DRONE_MOVE_MAGNITUDE 17.5f
#define DRONE_DRAG_COEFFICIENT 3.5f
#define DRONE_DEFAULT_WEAPON STANDARD_WEAPON

// weapon projectile settings
#define STANDARD_AMMO INFINITE_AMMO
#define STANDARD_RECOIL_MAGNITUDE 12.5f
#define STANDARD_FIRE_MAGNITUDE 25.0f
#define STANDARD_DRONE_MOVE_COEF 1.0f
#define STANDARD_COOL_DOWN 0.37f
#define STANDARD_MAX_DISTANCE 75.0f // needs tuning
#define STANDARD_RADIUS 0.2
#define STANDARD_DENSITY 8.0f
#define STANDARD_INV_MASS INV_MASS(STANDARD_DENSITY, STANDARD_RADIUS)
#define STANDARD_BOUNCE 2

#define MACHINEGUN_AMMO 35
#define MACHINEGUN_RECOIL_MAGNITUDE 3.0f
#define MACHINEGUN_FIRE_MAGNITUDE 12.0f
#define MACHINEGUN_DRONE_MOVE_COEF 0.5f
#define MACHINEGUN_COOL_DOWN 0.045f
#define MACHINEGUN_MAX_DISTANCE 50.0f
#define MACHINEGUN_RADIUS 0.15f
#define MACHINEGUN_DENSITY 5.0f
#define MACHINEGUN_INV_MASS INV_MASS(MACHINEGUN_DENSITY, MACHINEGUN_RADIUS)
#define MACHINEGUN_BOUNCE 1

#define MACHINEGUN_AIM_RECOIL_MAX 0.1f

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
};

// amount the aim can be randomly varied by
b2Vec2 weaponAimRecoil(enum weaponType type)
{
    float aimRecoilMax = 0.0f;
    switch (type)
    {
    case MACHINEGUN_WEAPON:
        return b2Vec2_zero;
        aimRecoilMax = MACHINEGUN_AIM_RECOIL_MAX;
        break;
    default:
        return b2Vec2_zero;
    }

    return (b2Vec2){
        .x = randFloat(-aimRecoilMax, aimRecoilMax),
        .y = randFloat(-aimRecoilMax, aimRecoilMax),
    };
}

// max ammo of weapon
int8_t weaponAmmo(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_AMMO;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_AMMO;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// amount of recoil to apply to drone
float weaponRecoil(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_RECOIL_MAGNITUDE;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_RECOIL_MAGNITUDE;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// amount of force to apply to projectile
float weaponFire(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_FIRE_MAGNITUDE;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_FIRE_MAGNITUDE;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// amount of parent drone lateral movement for be applied to projectile
float weaponDroneMoveCoef(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_DRONE_MOVE_COEF;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_DRONE_MOVE_COEF;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// time in seconds of cooldown between shots
float weaponCoolDown(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_COOL_DOWN;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_COOL_DOWN;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// max distance of projectile before it is destroyed
float weaponMaxDistance(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_MAX_DISTANCE;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_MAX_DISTANCE;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// radius of projectile shape, affects size and mass
float weaponRadius(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_RADIUS;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_RADIUS;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// density of projectile, affects mass
float weaponDensity(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_DENSITY;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_DENSITY;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// projectile inverse mass, used to keep projectile velocity constant
// when bouncing off other bodies
float weaponInvMass(enum weaponType type)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return STANDARD_INV_MASS;
    case MACHINEGUN_WEAPON:
        return MACHINEGUN_INV_MASS;
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// amount of times projectile can bounce before being destroyed
uint8_t weaponBounce(enum weaponType type)
{
    uint8_t bounce = 0;
    switch (type)
    {
    case STANDARD_WEAPON:
        bounce = STANDARD_BOUNCE;
        break;
    case MACHINEGUN_WEAPON:
        bounce = MACHINEGUN_BOUNCE;
        break;
    default:
        ERRORF("unknown weapon type %d", type);
    }
    return bounce + 1;
}

#endif