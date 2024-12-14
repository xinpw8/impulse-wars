#include "env.h"

void perfTest(const int steps)
{
    srand(0);

    env *e = createEnv();
    setupEnv(e);

    for (int i = 0; i < steps; i++)
    {
        uint8_t actionOffset = 0;
        for (size_t i = 0; i < cc_deque_size(e->drones); i++)
        {
            e->actions[actionOffset + 0] = randFloat(-1.0f, 1.0f);
            e->actions[actionOffset + 1] = randFloat(-1.0f, 1.0f);
            e->actions[actionOffset + 2] = randFloat(-1.0f, 1.0f);
            e->actions[actionOffset + 3] = randFloat(-1.0f, 1.0f);
            e->actions[actionOffset + 4] = randInt(0, 1);

            actionOffset += ACTION_SIZE;
        }

        stepEnv(e, DELTA_TIME);
        if (envTerminated(e))
        {
            resetEnv(e);
        }
    }

    destroyEnv(e);
}

int main(void)
{
    srand(0);
    perfTest(3000000);
    return 0;
}
