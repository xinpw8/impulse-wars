#include "env.h"

void perfTest(const int steps)
{
    srand(0);

    env *e = (env *)fastCalloc(1, sizeof(env));
    float *obs = (float *)fastCalloc(NUM_DRONES * OBS_SIZE, sizeof(float));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * ACTION_SIZE, sizeof(float));
    unsigned char *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(bool));

    initEnv(e, obs, rewards, actions, terminals);

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

        stepEnv(e);
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
