#include "env.h"

void perfTest(const float testTime)
{
    env *e = (env *)fastCalloc(1, sizeof(env));
    float *obs = (float *)fastCalloc(NUM_DRONES * OBS_SIZE, sizeof(float));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * ACTION_SIZE, sizeof(float));
    unsigned char *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(bool));

    initEnv(e, obs, actions, rewards, terminals, 0);

    const time_t start = time(NULL);
    int steps = 0;
    while (time(NULL) - start < testTime)
    {
        uint8_t actionOffset = 0;
        for (size_t i = 0; i < cc_array_size(e->drones); i++)
        {
            e->actions[actionOffset + 0] = randFloat(&e->randState, -1.0f, 1.0f);
            e->actions[actionOffset + 1] = randFloat(&e->randState, -1.0f, 1.0f);
            e->actions[actionOffset + 2] = randFloat(&e->randState, -1.0f, 1.0f);
            e->actions[actionOffset + 3] = randFloat(&e->randState, -1.0f, 1.0f);
            e->actions[actionOffset + 4] = randFloat(&e->randState, -1.0f, 1.0f);

            actionOffset += ACTION_SIZE;
        }

        stepEnv(e);
        steps++;
    }

    const time_t end = time(NULL);
    printf("SPS: %f\n", (2.0f * FRAMESKIP * steps) / (end - start));
    printf("Steps: %d\n", steps * FRAMESKIP);

    destroyEnv(e);

    fastFree(obs);
    fastFree(actions);
    fastFree(rewards);
    fastFree(terminals);
    fastFree(e);
}

int main(void)
{
    srand(0);
    perfTest(10.0f);
    return 0;
}
