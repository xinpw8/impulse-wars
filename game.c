#include "game.h"
#include "render.h"

const float lStickDeadzoneX = 0.1f;
const float lStickDeadzoneY = 0.1f;
const float rStickDeadzoneX = 0.1f;
const float rStickDeadzoneY = 0.1f;

typedef struct gamepadInputs
{
    b2Vec2 lStick;
    b2Vec2 rStick;
    bool rTrigger;
} gamepadInputs;

gamepadInputs getGamepadInputs()
{
    gamepadInputs inputs = {0};

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
    inputs.lStick = (b2Vec2){.x = lStickX, .y = lStickY};
    inputs.rStick = b2Normalize((b2Vec2){.x = rStickX, .y = rStickY});

    inputs.rTrigger = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    if (!inputs.rTrigger)
    {
        inputs.rTrigger = IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    }

    return inputs;
}

void handlePlayerDroneInputs(const b2WorldId worldID, struct hashmap_s *shapeMap, CC_SList *projectiles, droneEntity *drone)
{
    gamepadInputs inputs = getGamepadInputs();
    if (b2AbsFloat(inputs.lStick.x) > 0 || b2AbsFloat(inputs.lStick.y) > 0)
    {
        droneMove(drone, inputs.lStick);
    }
    else
    {
        if (IsKeyDown(KEY_W))
        {
            droneMove(drone, (b2Vec2){0.0f, -1.0f});
        }
        if (IsKeyDown(KEY_S))
        {
            droneMove(drone, (b2Vec2){0.0f, 1.0f});
        }
        if (IsKeyDown(KEY_A))
        {
            droneMove(drone, (b2Vec2){-1.0f, 0.0f});
        }
        if (IsKeyDown(KEY_D))
        {
            droneMove(drone, (b2Vec2){1.0f, 0.0f});
        }
    }

    if (inputs.rTrigger)
    {
        droneShoot(worldID, shapeMap, projectiles, drone, inputs.rStick);
    }
    else if (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
        b2Vec2 dronePos = b2Body_GetPosition(drone->bodyID);
        b2Vec2 mouseAim = b2Sub(rayVecToB2Vec(mousePos), dronePos);
        droneShoot(worldID, shapeMap, projectiles, drone, mouseAim);
    }
}

// void perfTest(float testTime)
// {
//     int width = 1920;
//     int height = 1080;

//     b2WorldDef worldDef = b2DefaultWorldDef();
//     worldDef.gravity = createb2Vec(0.0f, 0.0f);
//     b2WorldId worldID = b2CreateWorld(&worldDef);

//     const unsigned initalSize = 64;
//     struct hashmap_s shapeMap;
//     if (0 != hashmap_create(initalSize, &shapeMap))
//     {
//         ERROR("error creating hashmap");
//     }

//     createWall(worldID, &shapeMap, ((float)width / 2.0f) / scale, 1000.0f / scale, 1600.0f / scale, 100.0f / scale, STANDARD_WALL);
//     createWall(worldID, &shapeMap, 110.0f / scale, ((float)height / 2.0f) / scale, 100.0f / scale, 1020.0f / scale, BOUNCY_WALL);
//     createWall(worldID, &shapeMap, ((float)width - 110.0f) / scale, ((float)height / 2.0f) / scale, 100.0f / scale, 1020.0f / scale, BOUNCY_WALL);
//     createWall(worldID, &shapeMap, ((float)width / 2.0f) / scale, 80.0f / scale, 1600.0f / scale, 100.0f / scale, STANDARD_WALL);

//     droneEntity *playerDrone = createDrone(worldID, &shapeMap, ((float)width / 2.0f) / scale, ((float)height / 2.0f) / scale);
//     droneEntity *aiDrone = createDrone(worldID, &shapeMap, ((float)width / 2.0f + 1.0f) / scale, ((float)height / 2.0f + 200.0f) / scale);

//     int i = 0;
//     time_t start = time(NULL);
//     time_t cur = start;
//     while (cur - start < testTime)
//     {
//         b2Vec2 playerMove = {.x = randFloat(-1.0f, 1.0f), .y = randFloat(-1.0f, 1.0f)};
//         droneMove(playerDrone, playerMove);
//         if (randInt(0, 1))
//         {
//             b2Vec2 aim = {.x = randFloat(-1.0f, 1.0f), .y = randFloat(-1.0f, 1.0f)};
//             droneShoot(worldID, &shapeMap, playerDrone, aim);
//         }

//         b2Vec2 aiMove = {.x = randFloat(-1.0f, 1.0f), .y = randFloat(-1.0f, 1.0f)};
//         droneMove(aiDrone, aiMove);
//         if (randInt(0, 1))
//         {
//             b2Vec2 aim = {.x = randFloat(-1.0f, 1.0f), .y = randFloat(-1.0f, 1.0f)};
//             droneShoot(worldID, &shapeMap, aiDrone, aim);
//         }

//         float frameTime = (float)cur - (float)start;
//         droneStep(&shapeMap, playerDrone, frameTime);
//         droneStep(&shapeMap, aiDrone, frameTime);

//         b2World_Step(worldID, frameTime, 4);

//         handleContactEvents(worldID, &shapeMap);

//         cur = time(NULL);
//         i++;
//     }

//     time_t end = time(NULL);
//     printf("SPS: %f\n", (float)i / (float)(end - start));
// }

int main(void)
{
    srand(time(NULL));

    int width = 1920;
    int height = 1080;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, "test");

    SetTargetFPS(60);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = createb2Vec(0.0f, 0.0f);
    b2WorldId worldID = b2CreateWorld(&worldDef);

    // TODO: set custom hash function to use shapeID id
    // TODO: set this based on number of drones
    const unsigned initalSize = 64;
    struct hashmap_s shapeMap;
    if (0 != hashmap_create(initalSize, &shapeMap))
    {
        ERROR("error creating hashmap");
    }

    CC_SList *projectiles;
    cc_slist_new(&projectiles);

    wallEntity *wall = createWall(worldID, &shapeMap, ((float)width / 2.0f) / scale, 1000.0f / scale, 1600.0f / scale, 100.0f / scale, STANDARD_WALL);
    wallEntity *wall2 = createWall(worldID, &shapeMap, 110.0f / scale, ((float)height / 2.0f) / scale, 100.0f / scale, 1020.0f / scale, BOUNCY_WALL);
    wallEntity *wall3 = createWall(worldID, &shapeMap, ((float)width - 110.0f) / scale, ((float)height / 2.0f) / scale, 100.0f / scale, 1020.0f / scale, BOUNCY_WALL);
    wallEntity *wall4 = createWall(worldID, &shapeMap, ((float)width / 2.0f) / scale, 80.0f / scale, 1600.0f / scale, 100.0f / scale, STANDARD_WALL);

    droneEntity *playerDrone = createDrone(worldID, &shapeMap, ((float)width / 2.0f) / scale, ((float)height / 2.0f) / scale);
    droneEntity *aiDrone = createDrone(worldID, &shapeMap, ((float)width / 2.0f + 1.0f) / scale, ((float)height / 2.0f + 200.0f) / scale);

    while (!WindowShouldClose())
    {
        handlePlayerDroneInputs(worldID, &shapeMap, projectiles, playerDrone);

        float frameTime = 1.0f / 60.0f;
        droneStep(&shapeMap, playerDrone, frameTime);
        droneStep(&shapeMap, aiDrone, frameTime);
        projectilesStep(&shapeMap, projectiles);

        b2World_Step(worldID, frameTime, 4);

        handleContactEvents(worldID, &shapeMap, projectiles);

        BeginDrawing();

        ClearBackground(DARKGRAY);
        DrawFPS(10, 10);

        renderDrone(playerDrone);
        renderDrone(aiDrone);
        renderProjectiles(projectiles);

        renderWall(wall);
        renderWall(wall2);
        renderWall(wall3);
        renderWall(wall4);

        EndDrawing();
    }

    destroyDrone(&shapeMap, playerDrone);
    destroyDrone(&shapeMap, aiDrone);
    destroyAllProjectiles(&shapeMap, projectiles);

    cc_slist_destroy(projectiles);
    hashmap_destroy(&shapeMap);

    b2DestroyWorld(worldID);
    CloseWindow();

    return 0;
}
