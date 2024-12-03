#ifndef RENDER_H
#define RENDER_H

#include "helpers.h"
#include "game.h"

const float scale = 20.0f; // 10.0f;

const int width = 1500;
const int height = 1000;

const float halfWidth = (float)width / 2.0f;
const float halfHeight = (float)height / 2.0f;

const Color barolo = {.r = 165, .b = 8, .g = 37, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideWidthExtent = 2.0f * DRONE_RADIUS;
const float aimGuideHeightExtent = 0.1f * DRONE_RADIUS;

float b2XToRayX(const float x)
{
    return halfWidth + x * scale;
}

float b2YToRayY(const float y)
{
    return (halfHeight + y * scale) + (4 * scale);
}

Vector2 b2VecToRayVec(const b2Vec2 v)
{
    return (Vector2){.x = b2XToRayX(v.x), .y = b2YToRayY(v.y)};
}

b2Vec2 rayVecToB2Vec(const Vector2 v)
{
    return (b2Vec2){.x = (v.x - halfWidth) / scale, .y = (v.y - halfHeight) / scale};
}

void renderWall(const wallEntity *wall)
{
    Color color = {0};
    switch (wall->type)
    {
    case STANDARD_WALL_ENTITY:
        color = BLUE;
        break;
    case BOUNCY_WALL_ENTITY:
        color = YELLOW;
        break;
    case DEATH_WALL_ENTITY:
        color = RED;
        break;
    default:
        ERRORF("unknown wall type %d", wall->type);
    }

    b2Vec2 wallPos = b2Body_GetPosition(wall->bodyID);
    Vector2 pos = b2VecToRayVec(wallPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = wall->extent.x * scale * 2.0f,
        .height = wall->extent.y * scale * 2.0f,
    };

    Vector2 origin = {.x = wall->extent.x * scale, .y = wall->extent.y * scale};
    float angle = 0.0f;
    if (wall->isFloating)
    {
        angle = b2Rot_GetAngle(b2Body_GetRotation(wall->bodyID));
        angle *= RAD2DEG;
    }

    DrawRectanglePro(rec, origin, angle, color);
}

void renderWeaponPickup(const weaponPickupEntity *pickup)
{
    if (pickup->respawnWait != 0.0f || pickup->floatingWallsTouching != 0)
    {
        return;
    }

    b2Vec2 pickupPos = b2Body_GetPosition(pickup->bodyID);
    Vector2 pos = b2VecToRayVec(pickupPos);
    Rectangle rec = {
        .x = pos.x,
        .y = pos.y,
        .width = PICKUP_THICKNESS * scale,
        .height = PICKUP_THICKNESS * scale,
    };
    Vector2 origin = {.x = (PICKUP_THICKNESS / 2.0f) * scale, .y = (PICKUP_THICKNESS / 2.0f) * scale};
    DrawRectanglePro(rec, origin, 0.0f, LIME);

    char *name = "";
    switch (pickup->weapon)
    {
    case MACHINEGUN_WEAPON:
        name = "MACH";
        break;
    default:
        ERRORF("unknown weapon pickup type %d", pickup->weapon);
    }

    DrawText(name, rec.x - origin.x + 1, rec.y - origin.y + 10, 10, BLACK);
}

void renderDrone(const droneEntity *drone, b2Vec2 move, b2Vec2 aim)
{
    if (b2VecEqual(aim, b2Vec2_zero))
    {
        aim = drone->lastAim;
    }

    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    float rayX = b2XToRayX(pos.x);
    float rayY = b2YToRayY(pos.y);

    if (!b2VecEqual(move, b2Vec2_zero))
    {
        float moveMagnitude = b2Length(move);
        float moveRot = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(-move.y, -move.x)));
        Rectangle moveGuide = {
            .x = rayX,
            .y = rayY,
            .width = ((halfDroneRadius * moveMagnitude) + halfDroneRadius) * scale * 2.0f,
            .height = halfDroneRadius * scale * 2.0f,
        };
        DrawRectanglePro(moveGuide, (Vector2){.x = 0.0f, .y = 0.5f * scale}, moveRot, RED);
    }

    float aimRot = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(aim.y, aim.x)));
    Rectangle aimGuide = {
        .x = rayX,
        .y = rayY,
        .width = aimGuideWidthExtent * scale * 3.0f,
        .height = aimGuideHeightExtent * scale * 3.0f,
    };
    DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = aimGuideHeightExtent * scale}, aimRot, RED);

    DrawCircleV(b2VecToRayVec(pos), DRONE_RADIUS * scale, barolo);
}

void renderProjectiles(CC_SList *projectiles)
{
    for (SNode *cur = projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        b2Vec2 projectilePos = b2Body_GetPosition(projectile->bodyID);
        DrawCircleV(b2VecToRayVec(projectilePos), scale * weaponRadius(projectile->type), PURPLE);
    }
}

#endif