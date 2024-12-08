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
            width = strlen(line) - 1;
        }
        height++;
    }
    width /= 2;
    rewind(file);

    int row = 0;
    while (fgets(line, sizeof(line), file))
    {
        size_t len = strlen(line);
        if (line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
            len--;
        }

        for (int col = 0; col < len; col++)
        {
            char cell = line[col];
            enum entityType wallType;
            bool floating = false;
            float x = ((col / 2) - (width / 2.0f)) * WALL_THICKNESS;
            float y = ((height / 2.0f) - (height - row) - 1) * WALL_THICKNESS;

            float thickness = WALL_THICKNESS;
            switch (cell)
            {
            case ' ':
                continue;
            case 'O':
                b2Vec2 *pos = malloc(sizeof(b2Vec2));
                pos->x = x;
                pos->y = y;
                cc_deque_add(e->emptyCells, pos);
                continue;
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
                ERRORF("unknown map layout cell %c", cell);
            }

            DEBUG_LOGF("creating wall at: (%f %f)", x, y);

            createWall(e, x, y, thickness, thickness, wallType, floating);
        }
        row++;
    }

    fclose(file);

    e->columns = width;
    e->rows = height;

    DEBUG_LOGF("empty cells: %zu", cc_deque_size(e->emptyCells));
}
