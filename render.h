#ifndef RENDER_H
#define RENDER_H

#include "helpers.h"
#include "game.h"

const float scale = 14.0f;

const int width = 1500;
const int height = 1200;

const float halfWidth = (float)width / 2.0f;
const float halfHeight = (float)height / 2.0f;

const Color barolo = {.r = 165, .b = 8, .g = 37, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;

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

void renderEmptyCell(const b2Vec2 emptyCell)
{
    Rectangle rec = {
        .x = b2XToRayX(emptyCell.x - (WALL_THICKNESS / 2.0f)),
        .y = b2YToRayY(emptyCell.y - (WALL_THICKNESS / 2.0f)),
        .width = WALL_THICKNESS * scale,
        .height = WALL_THICKNESS * scale,
    };
    DrawRectangleLinesEx(rec, scale / 20.0f, RAYWHITE);
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
    case SNIPER_WEAPON:
        name = "SNIP";
        break;
    default:
        ERRORF("unknown weapon pickup type %d", pickup->weapon);
    }

    DrawText(name, rec.x - origin.x + (0.1f * scale), rec.y - origin.y + scale, scale, BLACK);
}

Color getDroneColor(const int droneIdx)
{
    switch (droneIdx)
    {
    case 0:
        return barolo;
    case 1:
        return GREEN;
    case 2:
        return BLUE;
    case 3:
        return YELLOW;
    default:
        ERRORF("unsupported number of drones %d", droneIdx + 1);
    }
}

void renderDroneGuides(const b2WorldId worldID, const droneEntity *drone, const b2Vec2 move, b2Vec2 aim, const int droneIdx)
{
    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const float rayX = b2XToRayX(pos.x);
    const float rayY = b2YToRayY(pos.y);
    const Color droneColor = getDroneColor(droneIdx);

    // render thruster move guide
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
        DrawRectanglePro(moveGuide, (Vector2){.x = 0.0f, .y = 0.5f * scale}, moveRot, droneColor);
    }

    // find length of laser aiming guide by where it touches the nearest shape
    if (b2VecEqual(aim, b2Vec2_zero))
    {
        aim = drone->lastAim;
    }

    const b2Vec2 rayEnd = b2MulAdd(pos, 150.0f, aim);
    const b2Vec2 translation = b2Sub(rayEnd, pos);
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | DRONE_SHAPE};
    const b2RayResult rayRes = b2World_CastRayClosest(worldID, pos, translation, filter);
    const entity *e = (entity *)b2Shape_GetUserData(rayRes.shapeId);

    b2DistanceCache cache = {0};
    bool shapeIsCircle = false;
    b2DistanceProxy proxyA = {0};
    if (e->type == DRONE_ENTITY)
    {
        proxyA.radius = DRONE_RADIUS;
        shapeIsCircle = true;
    }
    else
    {
        proxyA.count = 1;
        proxyA.points[0] = (b2Vec2){.x = 0.0f, .y = 0.0f};
    }
    const b2DistanceProxy proxyB = makeDistanceProxy(e->type, &shapeIsCircle);
    const b2DistanceInput input = {
        .proxyA = proxyA,
        .proxyB = proxyB,
        .transformA = {.p = pos, .q = zeroB2Rot},
        .transformB = {.p = rayRes.point, .q = zeroB2Rot},
        .useRadii = shapeIsCircle,
    };
    const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

    float aimGuideWidth = 0.0f;
    switch (drone->weapon)
    {
    case STANDARD_WEAPON:
        aimGuideWidth = 5.0f;
        break;
    case MACHINEGUN_WEAPON:
        aimGuideWidth = 7.0f;
        break;
    case SNIPER_WEAPON:
        aimGuideWidth = 100.0f;
        break;
    default:
        ERRORF("unknown weapon when getting aim guide width %d", drone->weapon);
    }
    aimGuideWidth = fminf(aimGuideWidth, output.distance) + (DRONE_RADIUS * 2.0f);

    // render laser aim guide
    Rectangle aimGuide = {
        .x = rayX,
        .y = rayY,
        .width = aimGuideWidth * scale,
        .height = aimGuideHeight * scale,
    };
    const float aimAngle = RAD2DEG * b2Rot_GetAngle(b2MakeRot(b2Atan2(aim.y, aim.x)));
    DrawRectanglePro(aimGuide, (Vector2){.x = 0.0f, .y = (aimGuideHeight / 2.0f) * scale}, aimAngle, droneColor);
}

void renderDrone(const droneEntity *drone, const int droneIdx)
{
    const Vector2 raylibPos = b2VecToRayVec(b2Body_GetPosition(drone->bodyID));
    DrawCircleV(raylibPos, DRONE_RADIUS * scale, BLACK);
    DrawCircleLinesV(raylibPos, DRONE_RADIUS * scale, getDroneColor(droneIdx));
}

void renderDroneLabels(const droneEntity *drone)
{
    const Vector2 pos = b2VecToRayVec(b2Body_GetPosition(drone->bodyID));

    char ammoStr[5];
    sprintf(ammoStr, "%d", drone->ammo);
    DrawText(ammoStr, pos.x - (0.5f * scale), pos.y + (1.5f * scale), scale, WHITE);
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