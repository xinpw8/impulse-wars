#ifndef SETTINGS_H
#define SETTINGS_H

#include "helpers.h"

#define INFINITE_AMMO -1

// wall settings
#define WALL_THICKNESS 4.0f
#define FLOATING_WALL_THICKNESS 3.0f
#define BOUNCY_WALL_RESTITUTION 1.0f
#define WALL_DENSITY 5.0f

// weapon pickup settings
#define PICKUP_THICKNESS 3.0f
#define PICKUP_RESPAWN_WAIT 1.0f

// drone settings
#define DRONE_RADIUS 1.0f
#define DRONE_DENSITY 1.25f
#define DRONE_MOVE_MAGNITUDE 25.0f
#define DRONE_LINEAR_DAMPING 1.0f
#define DRONE_MOVE_AIM_DIVISOR 10.0f
#define DRONE_DEFAULT_WEAPON STANDARD_WEAPON

// weapon projectile settings
#define STANDARD_AMMO INFINITE_AMMO
#define STANDARD_RECOIL_MAGNITUDE 12.5f
#define STANDARD_FIRE_MAGNITUDE 15.5f
#define STANDARD_COOL_DOWN 0.37f
#define STANDARD_MAX_DISTANCE 80.0f
#define STANDARD_RADIUS 0.2
#define STANDARD_DENSITY 3.0f
#define STANDARD_INV_MASS INV_MASS(STANDARD_DENSITY, STANDARD_RADIUS)
#define STANDARD_BOUNCE 2

#define MACHINEGUN_AMMO 35
#define MACHINEGUN_RECOIL_MAGNITUDE 5.0f
#define MACHINEGUN_FIRE_MAGNITUDE 10.0f
#define MACHINEGUN_COOL_DOWN 0.07f
#define MACHINEGUN_MAX_DISTANCE 120.0f
#define MACHINEGUN_RADIUS 0.15f
#define MACHINEGUN_DENSITY 2.0f
#define MACHINEGUN_INV_MASS INV_MASS(MACHINEGUN_DENSITY, MACHINEGUN_RADIUS)
#define MACHINEGUN_BOUNCE 1

#define MACHINEGUN_AIM_RECOIL_MAX 0.1f

enum weaponType
{
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
};

// amount the aim can be randomly varied by
b2Vec2 weaponAimRecoil(const enum weaponType type)
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
int8_t weaponAmmo(const enum weaponType type)
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
float weaponRecoil(const enum weaponType type)
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
float weaponFire(const enum weaponType type)
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

// time in seconds of cooldown between shots
float weaponCoolDown(const enum weaponType type)
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
float weaponMaxDistance(const enum weaponType type)
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
float weaponRadius(const enum weaponType type)
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
float weaponDensity(const enum weaponType type)
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

b2Vec2 weaponAdjustAim(const enum weaponType type, const uint16_t heat, const b2Vec2 normAim)
{
    switch (type)
    {
    case STANDARD_WEAPON:
        return normAim;
    case MACHINEGUN_WEAPON:
        const float swayCoef = logBasef((heat / 5.0f) + 1, 180);
        const float maxSway = 0.15f;
        const float swayX = randFloat(maxSway * -swayCoef, maxSway * swayCoef);
        const float swayY = randFloat(maxSway * -swayCoef, maxSway * swayCoef);
        DEBUG_LOGF("heat=%d sway=(%f %f)", heat, swayX, swayY);
        b2Vec2 aim = {.x = normAim.x + swayX, .y = normAim.y + swayY};
        return b2Normalize(aim);
    default:
        ERRORF("unknown weapon type %d", type);
    }
}

// projectile inverse mass, used to keep projectile velocity constant
// when bouncing off other bodies
float weaponInvMass(const enum weaponType type)
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
uint8_t weaponBounce(const enum weaponType type)
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