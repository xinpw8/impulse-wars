#ifndef RENDER_H
#define RENDER_H

#include "helpers.h"
#include "game.h"

void renderWall(const wallEntity *wall)
{
    Color color = {0};
    switch (wall->type)
    {
    case STANDARD_WALL:
        color = BLUE;
        break;
    case BOUNCY_WALL:
        color = YELLOW;
        break;
    case DEATH_WALL:
        color = RED;
        break;
    default:
        ERRORF("unknown wall type %d", wall->type);
    }

    b2Vec2 groundPos = b2Body_GetPosition(wall->bodyID);
    Vector2 rectPosition = b2VecToRayVec(groundPos);
    Rectangle rec = {
        .x = rectPosition.x - wall->extent.x * scale,
        .y = rectPosition.y - wall->extent.y * scale,
        .width = wall->extent.x * scale * 2.0f,
        .height = wall->extent.y * scale * 2.0f,
    };
    DrawRectanglePro(rec, (Vector2){.x = 0.0f, .y = 0.0f}, 0.0f, color);
}

void renderDrone(const droneEntity *drone)
{
    b2Vec2 pos = b2Body_GetPosition(drone->bodyID);
    DrawCircleV(b2VecToRayVec(pos), DRONE_RADIUS * scale, GREEN);
}

void renderProjectiles(CC_SList *projectiles)
{
    for (SNode *cur = projectiles->head; cur != NULL; cur = cur->next)
    {
        projectileEntity *projectile = (projectileEntity *)cur->data;
        b2Vec2 projectilePos = b2Body_GetPosition(projectile->bodyID);
        DrawCircleV(b2VecToRayVec(projectilePos), scale * weaponRadius(projectile->type), WHITE);
    }
}

#endif