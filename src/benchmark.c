#include "env.h"

void perfTest(const float testTime)
{
    const uint8_t NUM_DRONES = 2;
    const observationInfo obsInfo = calculateObservationInfo(NUM_DRONES);

    env *e = (env *)fastCalloc(1, sizeof(env));
    float *obs = (float *)fastCalloc(NUM_DRONES * obsInfo.obsSize, sizeof(float));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * ACTION_SIZE, sizeof(float));
    unsigned char *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(bool));
    logBuffer *logs = createLogBuffer(1);

    initEnv(e, 2, 2, obs, actions, rewards, terminals, logs, 0);

    const time_t start = time(NULL);
    int steps = 0;
    while (time(NULL) - start < testTime)
    {
        uint8_t actionOffset = 0;
        for (uint8_t i = 0; i < e->numDrones; i++)
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
    printf("SPS: %f\n", (float)(NUM_DRONES * FRAMESKIP * steps) / (float)(end - start));
    printf("Steps: %d\n", steps * FRAMESKIP);

    destroyEnv(e);

    fastFree(obs);
    fastFree(actions);
    fastFree(rewards);
    fastFree(terminals);
    destroyLogBuffer(logs);
    fastFree(e);
}

int main(void)
{
    perfTest(10.0f);
    return 0;
}
