#ifndef SETTINGS_H
#define SETTINGS_H

#include "helpers.h"

#define INFINITE_AMMO -1

#define DRONE_RADIUS 1.0f
#define DRONE_DENSITY 1.25f
#define DRONE_MOVE_MAGNITUDE 20.0f
#define DRONE_DRAG_COEFFICIENT 6.0f
#define DRONE_DEFAULT_WEAPON MACHINEGUN_WEAPON

#define STANDARD_AMMO INFINITE_AMMO
#define STANDARD_RECOIL_MAGNITUDE 15.0f
#define STANDARD_FIRE_MAGNITUDE 25.0f
#define STANDARD_COOL_DOWN 0.5f
#define STANDARD_MAX_DISTANCE 75.0f
#define STANDARD_RADIUS 0.2
#define STANDARD_DENSITY 10.0f
#define STANDARD_INV_MASS INV_MASS(STANDARD_DENSITY, STANDARD_RADIUS)
#define STANDARD_BOUNCE 2

#define MACHINEGUN_AMMO 34
#define MACHINEGUN_RECOIL_MAGNITUDE 5.0f
#define MACHINEGUN_FIRE_MAGNITUDE 20.0f
#define MACHINEGUN_COOL_DOWN 0.05f
#define MACHINEGUN_MAX_DISTANCE 50.0f
#define MACHINEGUN_RADIUS 0.1f
#define MACHINEGUN_DENSITY 25.0f
#define MACHINEGUN_INV_MASS INV_MASS(MACHINEGUN_DENSITY, MACHINEGUN_RADIUS)
#define MACHINEGUN_BOUNCE 1

#define MACHINEGUN_AIM_RECOIL_MAX 5.0f

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
};

b2Vec2 weaponAimRecoil(enum weaponType type)
{
    float aimRecoilMax = 0.0f;
    switch (type)
    {
    case MACHINEGUN_WEAPON:
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