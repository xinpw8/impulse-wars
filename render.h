#pragma once

#include "raylib.h"

#include "helpers.h"
#include "env.h"

const float scale = 12.0f;

const int width = 1500;
const int height = 1000;

const float halfWidth = (float)width / 2.0f;
const float halfHeight = (float)height / 2.0f;

const Color barolo = {.r = 165, .b = 8, .g = 37, .a = 255};

const float halfDroneRadius = DRONE_RADIUS / 2.0f;
const float aimGuideHeight = 0.3f * DRONE_RADIUS;

static inline float b2XToRayX(const float x)
{
    return halfWidth + (x * scale);
}

static inline float b2YToRayY(const float y)
{
    return (halfHeight + (y * scale)) + (2 * scale);
}

static inline Vector2 b2VecToRayVec(const b2Vec2 v)
{
    return (Vector2){.x = b2XToRayX(v.x), .y = b2YToRayY(v.y)};
}

static inline b2Vec2 rayVecToB2Vec(const Vector2 v)
{
    return (b2Vec2){.x = (v.x - halfWidth) / scale, .y = (v.y - halfHeight) / scale};
}

void renderUI(const env *e)
{
    if (e->stepsLeft == 0)
    {
        DrawText("SUDDEN DEATH", (width / 2) - (8 * scale), scale, 2 * scale, WHITE);
        return;
    }

    const int bufferSize = 3;
    char timerStr[bufferSize];
    if (e->stepsLeft >= 10 * FRAME_RATE)
    {
        snprintf(timerStr, bufferSize, "%d", (uint16_t)(e->stepsLeft / FRAME_RATE));
    }
    else
    {
        snprintf(timerStr, bufferSize, "0%d", (uint16_t)(e->stepsLeft / FRAME_RATE));
    }
    DrawText(timerStr, (width / 2) - scale, scale, 2 * scale, WHITE);
}

void renderEmptyCell(const b2Vec2 emptyCell, const size_t idx)
{
    Rectangle rec = {
        .x = b2XToRayX(emptyCell.x - (WALL_THICKNESS / 2.0f)),
        .y = b2YToRayY(emptyCell.y - (WALL_THICKNESS / 2.0f)),
        .width = WALL_THICKNESS * scale,
        .height = WALL_THICKNESS * scale,
    };
    DrawRectangleLinesEx(rec, scale / 20.0f, RAYWHITE);

    const int bufferSize = 4;
    char idxStr[bufferSize];
    snprintf(idxStr, bufferSize, "%zu", idx);
    DrawText(idxStr, rec.x, rec.y, 1.5f * scale, WHITE);
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
        name = "MCGN";
        break;
    case SNIPER_WEAPON:
        name = "SNPR";
        break;
    case SHOTGUN_WEAPON:
        name = "SHGN";
        break;
    case IMPLODER_WEAPON:
        name = "IMPL";
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

void renderDroneGuides(const env *e, const droneEntity *drone, const b2Vec2 move, b2Vec2 aim, const int droneIdx)
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
    const b2QueryFilter filter = {.categoryBits = PROJECTILE_SHAPE, .maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_SHAPE};
    const b2RayResult rayRes = b2World_CastRayClosest(e->worldID, pos, translation, filter);
    const entity *ent = (entity *)b2Shape_GetUserData(rayRes.shapeId);

    b2DistanceCache cache = {0};
    bool shapeIsCircle = false;
    b2DistanceProxy proxyA = {0};
    if (ent->type == DRONE_ENTITY)
    {
        proxyA.radius = DRONE_RADIUS;
        shapeIsCircle = true;
    }
    else
    {
        proxyA.count = 1;
        proxyA.points[0] = (b2Vec2){.x = 0.0f, .y = 0.0f};
    }
    const b2DistanceProxy proxyB = makeDistanceProxy(ent->type, &shapeIsCircle);
    const b2DistanceInput input = {
        .proxyA = proxyA,
        .proxyB = proxyB,
        .transformA = {.p = pos, .q = b2Rot_identity},
        .transformB = {.p = rayRes.point, .q = b2Rot_identity},
        .useRadii = shapeIsCircle,
    };
    const b2DistanceOutput output = b2ShapeDistance(&cache, &input, NULL, 0);

    float aimGuideWidth = 0.0f;
    switch (drone->weaponInfo->type)
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
    case SHOTGUN_WEAPON:
        aimGuideWidth = 4.0f;
        break;
    case IMPLODER_WEAPON:
        aimGuideWidth = 5.0f;
        break;
    default:
        ERRORF("unknown weapon when getting aim guide width %d", drone->weaponInfo->type);
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
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    const Vector2 raylibPos = b2VecToRayVec(pos);
    DrawCircleV(raylibPos, DRONE_RADIUS * scale, BLACK);
    DrawCircleLinesV(raylibPos, DRONE_RADIUS * scale, getDroneColor(droneIdx));
}

void renderDroneLabels(const droneEntity *drone)
{
    const b2Vec2 pos = b2Body_GetPosition(drone->bodyID);

    const int bufferSize = 5;
    char ammoStr[bufferSize];
    snprintf(ammoStr, bufferSize, "%d", drone->ammo);
    float posX = pos.x - 0.25;
    if (drone->ammo >= 10 || drone->ammo == INFINITE)
    {
        posX -= 0.25f;
    }
    DrawText(ammoStr, b2XToRayX(posX), b2YToRayY(pos.y + 1.5f), scale, WHITE);

    const float maxCharge = weaponCharge(drone->weaponInfo->type);
    if (maxCharge == 0.0f)
    {
        return;
    }

    const float chargeMeterWidth = 2.0f;
    const float chargeMeterHeight = 1.0f;
    Rectangle outlineRec = {
        .x = b2XToRayX(pos.x - (chargeMeterWidth / 2.0f)),
        .y = b2YToRayY(pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = chargeMeterWidth * scale,
        .height = chargeMeterHeight * scale,
    };
    DrawRectangleLinesEx(outlineRec, scale / 20.0f, RAYWHITE);

    const float fillRecWidth = (drone->charge / maxCharge) * chargeMeterWidth;
    Rectangle fillRec = {
        .x = b2XToRayX(pos.x - 1.0f),
        .y = b2YToRayY(pos.y - (chargeMeterHeight / 2.0f) + 3.0f),
        .width = fillRecWidth * scale,
        .height = chargeMeterHeight * scale,
    };
    const Vector2 origin = {.x = 0.0f, .y = 0.0f};
    DrawRectanglePro(fillRec, origin, 0.0f, RAYWHITE);
}

void renderProjectiles(env *e)
{
    if (e->explosionSteps != 0)
    {
        DrawCircleV(b2VecToRayVec(e->explosion.position), e->explosion.falloff * 2.0f * scale, GRAY);
        DrawCircleV(b2VecToRayVec(e->explosion.position), e->explosion.radius * scale, RAYWHITE);
        e->explosionSteps = fmaxf(e->explosionSteps - 1, 0);
    }

    for (SNode *cur = e->projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        b2Vec2 pos = b2Body_GetPosition(projectile->bodyID);
        DrawCircleV(b2VecToRayVec(pos), scale * projectile->weaponInfo->radius, PURPLE);
    }
}

void renderEnv(env *e, const CC_Deque *inputs)
{
    BeginDrawing();

    ClearBackground(BLACK);
    DrawFPS(scale, scale);

    renderUI(e);

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        droneInputs *input;
        cc_deque_get_at(inputs, i, (void **)&input);

        renderDroneGuides(e, drone, input->move, input->aim, i);
    }
    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        renderDrone(drone, i);
    }

    for (size_t i = 0; i < cc_deque_size(e->walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->walls, i, (void **)&wall);
        renderWall(wall);
    }

    for (size_t i = 0; i < cc_deque_size(e->floatingWalls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(e->floatingWalls, i, (void **)&wall);
        renderWall(wall);
    }

    for (size_t i = 0; i < cc_deque_size(e->pickups); i++)
    {
        weaponPickupEntity *pickup;
        cc_deque_get_at(e->pickups, i, (void **)&pickup);
        renderWeaponPickup(pickup);
    }

    renderProjectiles(e);

    for (size_t i = 0; i < cc_deque_size(e->drones); i++)
    {
        droneEntity *drone;
        cc_deque_get_at(e->drones, i, (void **)&drone);
        renderDroneLabels(drone);
    }

    // for (size_t i = 0; i < cc_deque_size(e->cells); i++)
    // {
    //     mapCell *cell;
    //     cc_deque_get_at(e->cells, i, (void **)&cell);
    //     if (cell->ent == NULL)
    //     {
    //         renderEmptyCell(cell->pos, i);
    //     }
    // }

    EndDrawing();
}
