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

playerInputs getPlayerInputs(const droneEntity *drone, const int gamepadIdx)
{
    playerInputs inputs = {0};
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

    SetTargetFPS(FRAME_RATE);

    b2WorldDef worldDef;
    b2WorldId worldID;

    CC_Deque *entities;
    CC_Deque *walls;
    CC_Deque *emptyCells;
    CC_Deque *pickups;
    CC_SList *projectiles;

    bool shouldExit = false;
    while (!shouldExit)
    {
        worldDef = b2DefaultWorldDef();
        worldDef.gravity = createb2Vec(0.0f, 0.0f);
        worldID = b2CreateWorld(&worldDef);

        cc_deque_new(&entities);
        cc_deque_new(&walls);
        cc_deque_new(&emptyCells);
        cc_deque_new(&pickups);
        cc_slist_new(&projectiles);

        createMap("prototype_arena.txt", worldID, entities, walls, emptyCells);

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

        for (int i = 0; i < 2; i++)
        {
            weaponPickupEntity *machPickup = createWeaponPickup(worldID, entities, emptyCells, MACHINEGUN_WEAPON);
            cc_deque_add(pickups, machPickup);
            weaponPickupEntity *snipPickup = createWeaponPickup(worldID, entities, emptyCells, SNIPER_WEAPON);
            cc_deque_add(pickups, snipPickup);
            weaponPickupEntity *shotPickup = createWeaponPickup(worldID, entities, emptyCells, SHOTGUN_WEAPON);
            cc_deque_add(pickups, shotPickup);
            weaponPickupEntity *implPickup = createWeaponPickup(worldID, entities, emptyCells, IMPLODER_WEAPON);
            cc_deque_add(pickups, implPickup);
        }

        while (true)
        {
            if (WindowShouldClose())
            {
                shouldExit = true;
                break;
            }

            playerInputs inputs1 = getPlayerInputs(playerDrone, 0);
            handlePlayerDroneInputs(worldID, projectiles, playerDrone, inputs1);
            playerInputs inputs2 = getPlayerInputs(aiDrone, 1);
            handlePlayerDroneInputs(worldID, projectiles, aiDrone, inputs2);

            droneStep(playerDrone, DELTA_TIME);
            droneStep(aiDrone, DELTA_TIME);
            projectilesStep(worldID, projectiles);

            for (size_t i = 0; i < cc_deque_size(pickups); i++)
            {
                weaponPickupEntity *pickup;
                cc_deque_get_at(pickups, i, (void **)&pickup);
                weaponPickupStep(entities, emptyCells, pickup, DELTA_TIME);
            }

            b2World_Step(worldID, DELTA_TIME, 8);

            handleContactEvents(worldID, projectiles);
            handleSensorEvents(worldID);

            if (playerDrone->dead || aiDrone->dead)
            {
                break;
            }

            BeginDrawing();

            ClearBackground(BLACK);
            DrawFPS(10, 10);

            // for (size_t i = 0; i < cc_deque_size(emptyCells); i++)
            // {
            //     b2Vec2 *emptyCell;
            //     cc_deque_get_at(emptyCells, i, (void **)&emptyCell);
            //     renderEmptyCell(*emptyCell);
            // }

            renderDroneGuides(worldID, playerDrone, inputs1.move, inputs1.aim, 0);
            renderDroneGuides(worldID, aiDrone, inputs2.move, inputs2.aim, 1);
            renderDrone(playerDrone, 0);
            renderDrone(aiDrone, 1);

            for (size_t i = 0; i < cc_deque_size(walls); i++)
            {
                wallEntity *wall;
                cc_deque_get_at(walls, i, (void **)&wall);
                renderWall(wall);
            }

            for (size_t i = 0; i < cc_deque_size(pickups); i++)
            {
                weaponPickupEntity *pickup;
                cc_deque_get_at(pickups, i, (void **)&pickup);
                renderWeaponPickup(pickup);
            }

            renderProjectiles(projectiles);

            renderDroneLabels(playerDrone);
            renderDroneLabels(aiDrone);

            DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.min.x, .y = bounds.max.y}), 0.3f * scale, GREEN);
            DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.max.x, .y = bounds.max.y}), 0.3f * scale, GREEN);
            DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.min.x, .y = bounds.min.y}), 0.3f * scale, GREEN);
            DrawCircleV(b2VecToRayVec((b2Vec2){.x = bounds.max.x, .y = bounds.min.y}), 0.3f * scale, GREEN);

            EndDrawing();
        }

        for (size_t i = 0; i < cc_deque_size(pickups); i++)
        {
            weaponPickupEntity *pickup;
            cc_deque_get_at(pickups, i, (void **)&pickup);
            destroyWeaponPickup(pickup);
        }

        destroyDrone(playerDrone);
        destroyDrone(aiDrone);
        destroyAllProjectiles(worldID, projectiles);

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
        cc_deque_destroy(entities);
        cc_deque_destroy(walls);
        cc_deque_destroy(emptyCells);
        cc_deque_destroy(pickups);

        b2DestroyWorld(worldID);
    }

    CloseWindow();

    return 0;
}
