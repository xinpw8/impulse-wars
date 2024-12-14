#include "env.h"
#include "render.h"

const float lStickDeadzoneX = 0.1f;
const float lStickDeadzoneY = 0.1f;
const float rStickDeadzoneX = 0.1f;
const float rStickDeadzoneY = 0.1f;

void getPlayerInputs(const env *e, const droneEntity *drone, const int gamepadIdx)
{
    uint8_t offset = gamepadIdx * ACTION_SIZE;
    if (IsGamepadAvailable(gamepadIdx))
    {
        float lStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_X);
        float lStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_Y);
        float rStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_Y);

        if (lStickX > -lStickDeadzoneX && lStickX < lStickDeadzoneX)
        {
            lStickX = 0.0f;
        }
        if (lStickY > -lStickDeadzoneY && lStickY < lStickDeadzoneY)
        {
            lStickY = 0.0f;
        }
        if (rStickX > -rStickDeadzoneX && rStickX < rStickDeadzoneX)
        {
            rStickX = 0.0f;
        }
        if (rStickY > -rStickDeadzoneY && rStickY < rStickDeadzoneY)
        {
            rStickY = 0.0f;
        }
        const b2Vec2 aim = b2Normalize((b2Vec2){.x = rStickX, .y = rStickY});

        bool shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        if (!shoot)
        {
            shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        }

        e->actions[offset++] = lStickX;
        e->actions[offset++] = lStickY;
        e->actions[offset++] = aim.x;
        e->actions[offset++] = aim.y;
        e->actions[offset++] = shoot;

        return;
    }

    b2Vec2 move = b2Vec2_zero;
    if (IsKeyDown(KEY_W))
    {
        move.y += -1.0f;
    }
    if (IsKeyDown(KEY_S))
    {
        move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A))
    {
        move.x += -1.0f;
    }
    if (IsKeyDown(KEY_D))
    {
        move.x += 1.0f;
    }
    move = b2Normalize(move);

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    const b2Vec2 aim = b2Normalize(b2Sub(rayVecToB2Vec(mousePos), dronePos));

    bool shoot = false;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        shoot = true;
    }

    e->actions[offset++] = move.x;
    e->actions[offset++] = move.y;
    e->actions[offset++] = aim.x;
    e->actions[offset++] = aim.y;
    e->actions[offset++] = shoot;
}

int main(void)
{
    srand(time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "test");

    SetTargetFPS(FRAME_RATE);

    env *e = (env *)fastCalloc(1, sizeof(env));
    float *obs = (float *)fastCalloc(NUM_DRONES * OBS_SIZE, sizeof(float));
    float *rewards = (float *)fastCalloc(NUM_DRONES, sizeof(float));
    float *actions = (float *)fastCalloc(NUM_DRONES * ACTION_SIZE, sizeof(float));
    unsigned char *terminals = (unsigned char *)fastCalloc(NUM_DRONES, sizeof(bool));

    initEnv(e, obs, rewards, actions, terminals);

    while (true)
    {
        if (WindowShouldClose())
        {
            destroyEnv(e);
            fastFree(obs);
            fastFree(rewards);
            fastFree(actions);
            fastFree(terminals);
            fastFree(e);
            CloseWindow();
            return 0;
        }

        for (size_t i = 0; i < cc_deque_size(e->drones); i++)
        {
            droneEntity *drone;
            cc_deque_get_at(e->drones, i, (void **)&drone);
            getPlayerInputs(e, drone, i);
        }

        stepEnv(e);
        renderEnv(e);
        if (envTerminated(e))
        {
            resetEnv(e);
        }
    }
}
