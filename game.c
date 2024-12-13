#include "env.h"
#include "render.h"

const float lStickDeadzoneX = 0.1f;
const float lStickDeadzoneY = 0.1f;
const float rStickDeadzoneX = 0.1f;
const float rStickDeadzoneY = 0.1f;

void getPlayerInputs(const env *e, const droneEntity *drone, const int gamepadIdx)
{
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

        e->actions[0] = lStickX;
        e->actions[1] = lStickY;
        e->actions[2] = aim.x;
        e->actions[3] = aim.y;
        e->actions[4] = shoot;

        return;
    }
    if (gamepadIdx != 0)
    {
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

    e->actions[0] = move.x;
    e->actions[1] = move.y;
    e->actions[2] = aim.x;
    e->actions[3] = aim.y;
    e->actions[4] = shoot;
}

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
    // perfTest(3000000);
    // return 0;

    srand(time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "test");

    SetTargetFPS(FRAME_RATE);

    env *e = createEnv();
    setupEnv(e);

    while (true)
    {
        while (true)
        {
            if (WindowShouldClose())
            {
                destroyEnv(e);
                CloseWindow();
                return 0;
            }

            for (size_t i = 0; i < cc_deque_size(e->drones); i++)
            {
                droneEntity *drone;
                cc_deque_get_at(e->drones, i, (void **)&drone);
                getPlayerInputs(e, drone, i);
            }

            stepEnv(e, DELTA_TIME);
            if (envTerminated(e))
            {
                break;
            }

            renderEnv(e);
        }

        resetEnv(e);
    }
}
