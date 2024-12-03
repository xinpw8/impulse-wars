#include "game.h"
#include "map.h"
#include "render.h"

const float lStickDeadzoneX = 0.1f;
const float lStickDeadzoneY = 0.1f;
const float rStickDeadzoneX = 0.1f;
const float rStickDeadzoneY = 0.1f;

typedef struct playerInputs
{
    b2Vec2 move;
    b2Vec2 aim;
    bool shoot;
} playerInputs;

playerInputs getPlayerInputs(const droneEntity *drone)
{
    playerInputs inputs = {0};
    if (IsGamepadAvailable(0))
    {
        float lStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float lStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        float rStickX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

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

        inputs.shoot = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        if (!inputs.shoot)
        {
            inputs.shoot = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        }
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

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
    inputs.aim = b2Sub(rayVecToB2Vec(mousePos), dronePos);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        inputs.shoot = true;
        b2Vec2 b2mousePos = rayVecToB2Vec(mousePos);
        DEBUG_LOGF("mouse aim: %f %f", b2mousePos.x, b2mousePos.y);
    }

    return inputs;
}

void handlePlayerDroneInputs(const b2WorldId worldID, CC_SList *projectiles, droneEntity *drone, const playerInputs inputs)
{
    if (!b2VecEqual(inputs.move, b2Vec2_zero))
    {
        droneMove(drone, inputs.move);
    }
    if (inputs.shoot)
    {
        droneShoot(worldID, projectiles, drone, inputs.aim);
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

    SetTargetFPS(60);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = createb2Vec(0.0f, 0.0f);
    b2WorldId worldID = b2CreateWorld(&worldDef);

    CC_Deque *entities;
    cc_deque_new(&entities);

    CC_Deque *walls;
    CC_Deque *emptyCells;
    createMap("prototype_arena.txt", worldID, entities, &walls, &emptyCells);

    mapBounds bounds = {.min = {.x = FLT_MAX, .y = FLT_MAX}, .max = {.x = FLT_MIN, .y = FLT_MIN}};
    for (size_t i = 0; i < cc_deque_size(walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(walls, i, (void **)&wall);
        bounds.min.x = fminf(wall->position.x - wall->extent.x + WALL_THICKNESS, bounds.min.x);
        bounds.min.y = fminf(wall->position.y - wall->extent.y + WALL_THICKNESS, bounds.min.y);
        bounds.max.x = fmaxf(wall->position.x + wall->extent.x - WALL_THICKNESS, bounds.max.x);
        bounds.max.y = fmaxf(wall->position.y + wall->extent.y - WALL_THICKNESS, bounds.max.y);
    }

    droneEntity *playerDrone = createDrone(worldID, entities, emptyCells);
    droneEntity *aiDrone = createDrone(worldID, entities, emptyCells);

    weaponPickupEntity *pickup = createWeaponPickup(worldID, entities, emptyCells, MACHINEGUN_WEAPON);

    CC_SList *projectiles;
    cc_slist_new(&projectiles);

    while (!WindowShouldClose())
    {
        playerInputs inputs = getPlayerInputs(playerDrone);
        handlePlayerDroneInputs(worldID, projectiles, playerDrone, inputs);

        float frameTime = fmaxf(GetFrameTime(), FLT_MIN);
        droneStep(playerDrone, frameTime);
        droneStep(aiDrone, frameTime);
        projectilesStep(projectiles);
        weaponPickupStep(entities, emptyCells, pickup, frameTime);

        b2World_Step(worldID, 1.0f / 60.0f, 8);

        handleContactEvents(worldID, projectiles);
        handleSensorEvents(worldID);

        BeginDrawing();

        ClearBackground(BLACK);
        DrawFPS(10, 10);

        for (size_t i = 0; i < cc_deque_size(walls); i++)
        {
            wallEntity *wall;
            cc_deque_get_at(walls, i, (void **)&wall);
            renderWall(wall);
        }

        renderWeaponPickup(pickup);

        renderDrone(playerDrone, inputs.move, inputs.aim);
        renderDrone(aiDrone, b2Vec2_zero, b2Vec2_zero);
        renderProjectiles(projectiles);

        DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.min.x, .y = bounds.max.y}), 0.3f * scale, GREEN);
        DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.max.x, .y = bounds.max.y}), 0.3f * scale, GREEN);
        DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.min.x, .y = bounds.min.y}), 0.3f * scale, GREEN);
        DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.max.x, .y = bounds.min.y}), 0.3f * scale, GREEN);

        EndDrawing();
    }

    destroyWeaponPickup(pickup);
    destroyDrone(playerDrone);
    destroyDrone(aiDrone);
    destroyAllProjectiles(projectiles);

    for (size_t i = 0; i < cc_deque_size(walls); i++)
    {
        wallEntity *wall;
        cc_deque_get_at(walls, i, (void **)&wall);
        destroyWall(worldID, wall);
    }

    for (size_t i = 0; i < cc_deque_size(emptyCells); i++)
    {
        b2Vec2 *pos;
        cc_deque_get_at(emptyCells, i, (void **)&pos);
        free(pos);
    }

    cc_slist_destroy(projectiles);
    cc_deque_destroy(walls);
    cc_deque_destroy(emptyCells);

    b2DestroyWorld(worldID);
    CloseWindow();

    return 0;
}
