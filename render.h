#ifndef RENDER_H
#define RENDER_H

#include "helpers.h"
#include "game.h"

void renderWall(const wallEntity *wall)
{
    Color color = {0};
    switch (wall->type)
    {
    case FLOATING_STANDARD_WALL_ENTITY:
    case STANDARD_WALL_ENTITY:
        color = BLUE;
        break;
    case FLOATING_BOUNCY_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
        color = YELLOW;
        break;
    case FLOATING_DEATH_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        color = RED;
        break;
    default:
        ERRORF("unknown wall type %d", wall->type);
    }

    b2Vec2 wallPos = b2Body_GetPosition(wall->bodyID);
    Vector2 pos = b2VecToRayVec(wallPos);
    Rectangle rec = {
        .x = pos.x - wall->extent.x * scale,
        .y = pos.y - wall->extent.y * scale,
        .width = wall->extent.x * scale * 2.0f,
        .height = wall->extent.y * scale * 2.0f,
    };
    DrawRectanglePro(rec, (Vector2){.x = 0.0f, .y = 0.0f}, 0.0f, color);
}

void renderWeaponPickup(const weaponPickupEntity *pickup)
{
    b2Vec2 pos = b2Body_GetPosition(pickup->bodyID);
    DrawCircleV(b2VecToRayVec(pos), DRONE_RADIUS * scale, LIME);
}

void renderDrone(const droneEntity *drone)
{
    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    DrawCircleV(b2VecToRayVec(pos), DRONE_RADIUS * scale, MAROON);
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