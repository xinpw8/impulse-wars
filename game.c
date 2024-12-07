#include "env.h"
#include "render.h"

const float lStickDeadzoneX = 0.1f;
const float lStickDeadzoneY = 0.1f;
const float rStickDeadzoneX = 0.1f;
const float rStickDeadzoneY = 0.1f;

droneInputs getPlayerInputs(const droneEntity *drone, const int gamepadIdx)
{
    droneInputs inputs = {0};
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
        inputs.move = (b2Vec2){.x = lStickX, .y = lStickY};
        inputs.aim = b2Normalize((b2Vec2){.x = rStickX, .y = rStickY});

        inputs.shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        if (!inputs.shoot)
        {
            inputs.shoot = IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        }
        return inputs;
    }
    if (gamepadIdx != 0)
    {
        return inputs;
    }

    if (IsKeyDown(KEY_W))
    {
        inputs.move.y += -1.0f;
    }
    if (IsKeyDown(KEY_S))
    {
        inputs.move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A))
    {
        inputs.move.x += -1.0f;
    }
    if (IsKeyDown(KEY_D))
    {
        inputs.move.x += 1.0f;
    }
    inputs.move = b2Normalize(inputs.move);

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    inputs.aim = b2Sub(rayVecToB2Vec(mousePos), dronePos);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        inputs.shoot = true;
    }

    return inputs;
}

void handlePlayerDroneInputs(env *e, droneEntity *drone, const droneInputs inputs)
{
    if (!b2VecEqual(inputs.move, b2Vec2_zero))
    {
        droneMove(drone, inputs.move);
    }
    if (inputs.shoot)
    {
        droneShoot(e, drone, inputs.aim);
    }
    if (!b2VecEqual(inputs.aim, b2Vec2_zero))
    {
        drone->lastAim = b2Normalize(inputs.aim);
    }
}

int main(void)
{
    srand(time(NULL));

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "test");

    SetTargetFPS(FRAME_RATE);

    env *e = createEnv();
    setupEnv(e);

    CC_Deque *inputs;
    CC_DequeConf inputsConf;
    cc_deque_conf_init(&inputsConf);
    inputsConf.capacity = 2;
    cc_deque_new_conf(&inputsConf, &inputs);
    for (int i = 0; i < 2; i++)
    {
        droneInputs *input = malloc(sizeof(droneInputs));
        cc_deque_add(inputs, input);
    }

    bool shouldExit = false;
    while (!shouldExit)
    {
        while (true)
        {
            if (WindowShouldClose())
            {
                shouldExit = true;
                break;
            }

            for (size_t i = 0; i < cc_deque_size(e->drones); i++)
            {
                droneEntity *drone;
                cc_deque_get_at(e->drones, i, (void **)&drone);
                droneInputs *input;
                cc_deque_get_at(inputs, i, (void **)&input);
                *input = getPlayerInputs(drone, i);

                handlePlayerDroneInputs(e, drone, *input);
            }

            stepEnv(e, DELTA_TIME);
            if (envTerminated(e))
            {
                break;
            }

            renderEnv(e, inputs);
        }

        resetEnv(e);
    }

    CloseWindow();

    return 0;
}
