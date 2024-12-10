#pragma once

#include <errno.h>
#include <string.h>

#include "box2d/box2d.h"
#include "cc_deque.h"

#include "env.h"
#include "settings.h"

void createMap(env *e, const char *layoutPath)
{
    FILE *file = fopen(layoutPath, "r");
    if (!file)
    {
        ERRORF("failed to open map file %s: %s", layoutPath, strerror(errno));
    }

    int width = 0;
    int height = 0;
    char line[50];
    while (fgets(line, sizeof(line), file))
    {
        if (width == 0)
        {
            width = strlen(line);
        }
        height++;
    }
    width /= 2;
    rewind(file);

    e->columns = width;
    e->rows = height;

    int row = 0;
    while (fgets(line, sizeof(line), file))
    {
        size_t len = strlen(line);
        if (line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
            len -= 2;
        }

        for (int col = 0; col <= len; col++)
        {
            char cellType = line[col];
            enum entityType wallType;
            bool empty = false;
            bool floating = false;
            float x = ((col / 2) - (width / 2.0f) + 0.5) * WALL_THICKNESS;
            float y = ((height / 2.0f) - (height - row) + 0.5f) * WALL_THICKNESS;

            float thickness = WALL_THICKNESS;
            switch (cellType)
            {
            case ' ':
                continue;
            case 'O':
                empty = true;
                break;
            case 'w':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'b':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'd':
                thickness = FLOATING_WALL_THICKNESS;
                floating = true;
            case 'D':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cellType);
            }

            b2Vec2 pos = {.x = x, .y = y};
            if (empty || floating)
            {
                if (empty)
                {
                    mapCell *cell = (mapCell *)calloc(1, sizeof(mapCell));
                    cell->ent = NULL;
                    cell->pos = pos;
                    cc_deque_add(e->cells, cell);
                    continue;
                }
            }

            // DEBUG_LOGF("creating wall at: (%f %f) cell index: %d", x, y, posToCellIdx(e, pos));

            entity *ent = createWall(e, x, y, thickness, thickness, wallType, floating);
            mapCell *cell = (mapCell *)calloc(1, sizeof(mapCell));
            cell->ent = ent;
            cc_deque_add(e->cells, cell);
        }
        row++;
    }

    fclose(file);
}
