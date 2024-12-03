#include <string.h>

#include "box2d/box2d.h"
#include "cc_deque.h"

#include "game.h"

void createMap(const char *layoutPath, const b2WorldId worldID, CC_Deque **walls, CC_Deque **emptyCells)
{
    FILE *file = fopen(layoutPath, "r");
    if (!file)
    {
        perror("Failed to open map file");
        return;
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

    cc_deque_new(walls);
    cc_deque_new(emptyCells);

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
            const float x = ((col / 2) - (width / 2.0f)) * WALL_THICKNESS;
            const float y = ((height / 2.0f) - (height - row) - 1) * WALL_THICKNESS;

            switch (cell)
            {
            case ' ':
                continue;
            case 'O':
                b2Vec2 *pos = malloc(sizeof(b2Vec2));
                pos->x = x;
                pos->y = y;
                cc_deque_add(*emptyCells, pos);
                continue;
            case 'w':
                floating = true;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'b':
                floating = true;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            case 'd':
                floating = true;
            case 'D':
                wallType = DEATH_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cell);
            }

            DEBUG_LOGF("creating wall at: (%f %f)", x, y);

            wallEntity *wall = createWall(worldID, x, y, WALL_THICKNESS, WALL_THICKNESS, wallType, floating);
            cc_deque_add(*walls, wall);
        }
        row++;
    }

    fclose(file);
}
