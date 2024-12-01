#include <string.h>

#include "box2d/box2d.h"
#include "cc_deque.h"

#include "game.h"

CC_Deque *createMap(const char *layoutPath, const b2WorldId worldID)
{
    FILE *file = fopen(layoutPath, "r");
    if (!file)
    {
        perror("Failed to open map file");
        return NULL;
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
    rewind(file);

    CC_Deque *walls;
    cc_deque_new(&walls);

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
            switch (cell)
            {
            case 'O':
                continue;
            case 'W':
                wallType = STANDARD_WALL_ENTITY;
                break;
            case 'B':
                wallType = BOUNCY_WALL_ENTITY;
                break;
            default:
                ERRORF("unknown map layout cell %c", cell);
            }

            float x = (col - width / 2.0f) * WALL_THICKNESS;
            float y = (height / 2.0f - row - 1) * WALL_THICKNESS;
            DEBUG_LOGF("creating wall at: (%f %f)", x, y);

            wallEntity *wall = createWall(worldID, x, y, WALL_THICKNESS, WALL_THICKNESS, wallType, false);
            cc_deque_add(walls, wall);
        }
        row++;
    }

    fclose(file);

    return walls;
}
