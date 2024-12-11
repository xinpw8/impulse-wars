#pragma once

#include <errno.h>
#include <string.h>

#include "box2d/box2d.h"
#include "cc_deque.h"

#include "env.h"
#include "settings.h"

typedef struct mapEntry
{
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t floatingStandardWalls;
    const uint8_t floatingBouncyWalls;
    const uint8_t floatingDeathWalls;
    const enum weaponType defaultWeapon;
} mapEntry;

// clang-format off

const char prototypeArenaLayout[] = {
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','D','D','D','D','D','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','W','W','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D',
    'D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D','D',
};

const mapEntry prototypeArenaMap = {
    .layout = prototypeArenaLayout,
    .columns = 20,
    .rows = 20,
    .floatingStandardWalls = 6,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 6,
    .defaultWeapon = STANDARD_WEAPON,
};

const char snipersLayout[] = {
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','B','B','B','B','O','B','B','B','B','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','D','D','D','B','O','B','D','D','D','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','D','D','B','O','B','D','D','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','D','D','O','O','O','O','O','O','O','O','O','O','O','O','O','O','O','D','D','B',
    'B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B','B',
};

const mapEntry snipersMap = {
    .layout = snipersLayout,
    .columns = 21,
    .rows = 19,
    .floatingStandardWalls = 0,
    .floatingBouncyWalls = 0,
    .floatingDeathWalls = 0,
    .defaultWeapon = SNIPER_WEAPON,
};

// clang-format on

void createMap(env *e, const mapEntry *map)
{
    e->columns = map->columns;
    e->rows = map->rows;
    e->defaultWeapon = &weaponInfos[map->defaultWeapon];

    for (int row = 0; row < map->rows; row++)
    {
        for (int col = 0; col < map->columns; col++)
        {
            char cellType = map->layout[col + (row * map->columns)];
            enum entityType wallType;
            float x = (col - (map->columns / 2.0f) + 0.5) * WALL_THICKNESS;
            float y = ((map->rows / 2.0f) - (map->rows - row) + 0.5f) * WALL_THICKNESS;

            b2Vec2 pos = {.x = x, .y = y};
            mapCell *cell = (mapCell *)fastMalloc(sizeof(mapCell));
            cell->pos = pos;
            cc_deque_add(e->cells, cell);

            switch (cellType)
            {
            case 'O':
                cell->ent = NULL;
                continue;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'D':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cellType);
            }

            DEBUG_LOGF("creating wall at: (%f %f) cell index: %d", x, y, posToCellIdx(e, pos));

            entity *ent = createWall(e, x, y, WALL_THICKNESS, WALL_THICKNESS, wallType, false);
            cell->ent = ent;
        }
    }
}

void placeFloatingWall(env *e, const enum entityType wallType)
{
    b2Vec2 pos;
    if (!findOpenPos(e, FLOATING_WALL_SHAPE, &pos))
    {
        ERROR("failed to find open position for floating wall");
    }
    entity *ent = createWall(e, pos.x, pos.y, FLOATING_WALL_THICKNESS, FLOATING_WALL_THICKNESS, wallType, true);
    mapCell *cell;
    cc_deque_get_at(e->cells, posToCellIdx(e, pos), (void **)&cell);
    cell->ent = ent;
}

void placeFloatingWalls(env *e, const mapEntry *map)
{
    for (int i = 0; i < map->floatingStandardWalls; i++)
    {
        placeFloatingWall(e, STANDARD_WALL_ENTITY);
    }
    for (int i = 0; i < map->floatingBouncyWalls; i++)
    {
        placeFloatingWall(e, BOUNCY_WALL_ENTITY);
    }
    for (int i = 0; i < map->floatingDeathWalls; i++)
    {
        placeFloatingWall(e, DEATH_WALL_ENTITY);
    }
}
