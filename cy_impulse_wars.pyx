from libc.stdint cimport uint8_t, int8_t, uint16_t, uint64_t
from libc.stdlib cimport calloc, free
import numpy as np
import pufferlib

cdef extern from "include/cc_array.h":
    void* safe_array_get_at(CC_Array* array, size_t index)

cdef extern from "settings.h":
    cdef int ACTION_SIZE

from impulse_wars cimport (
    MAX_DRONES,
    OBS_SIZE,
    NUM_WALL_TYPES,
    NUM_WEAPONS,
    MAP_OBS_SIZE,
    SCALAR_OBS_SIZE,
    DRONE_OBS_SIZE,
    MAP_CELL_OBS_SIZE,
    MAX_MAP_COLUMNS,
    MAX_MAP_ROWS,
    env,
    initEnv,
    rayClient,
    createRayClient,
    destroyRayClient,
    resetEnv,
    stepEnv,
    destroyEnv,
    LOG_BUFFER_SIZE,
    logBuffer,
    logEntry,
    createLogBuffer,
    destroyLogBuffer,
    aggregateAndClearLogBuffer,
)

cdef extern from "box2d/box2d.h":
    ctypedef struct b2Vec2:
        float x
        float y

    ctypedef struct b2BodyId:
        uint64_t id

    ctypedef struct b2ShapeId:
        uint64_t id

    ctypedef struct b2WorldId:
        uint64_t id

    ctypedef struct b2ExplosionDef:
        b2Vec2 pos
        float radius
        float impulse

cdef extern from "include/cc_array.h":
    ctypedef struct CC_Array:
        void *data
        size_t size
        size_t capacity

cdef extern from "include/cc_slist.h":
    ctypedef struct CC_SList:
        void *head

cdef extern from "settings.h":
    ctypedef float DELTA_TIME

cdef extern from "types.h":
    # Constants
    cdef const int _MAX_DRONES
    cdef const int _NUM_WEAPONS

    # Enums
    cdef enum entityType:
        STANDARD_WALL_ENTITY
        BOUNCY_WALL_ENTITY
        DEATH_WALL_ENTITY
        WEAPON_PICKUP_ENTITY
        PROJECTILE_ENTITY
        DRONE_ENTITY

    cdef enum shapeCategory:
        WALL_SHAPE
        FLOATING_WALL_SHAPE
        PROJECTILE_SHAPE
        WEAPON_PICKUP_SHAPE
        DRONE_SHAPE

    cdef enum weaponType:
        STANDARD_WEAPON
        MACHINEGUN_WEAPON
        SNIPER_WEAPON
        SHOTGUN_WEAPON
        IMPLODER_WEAPON

    # Structs
    cdef struct entity:
        entityType type
        void *entity

    cdef struct mapEntry:
        const char *layout
        uint8_t columns
        uint8_t rows
        uint8_t floatingStandardWalls
        uint8_t floatingBouncyWalls
        uint8_t floatingDeathWalls
        uint16_t weaponPickups
        weaponType defaultWeapon

    cdef struct mapCell:
        entity *ent
        b2Vec2 pos

    cdef struct mapBounds:
        b2Vec2 min
        b2Vec2 max

    cdef struct cachedPos:
        b2Vec2 pos
        bint valid

    cdef struct wallEntity:
        b2BodyId bodyID
        b2ShapeId shapeID
        cachedPos pos
        b2Vec2 extent
        bint isFloating
        entityType type

    cdef struct weaponInformation:
        weaponType type
        bint isPhysicsBullet
        uint8_t numProjectiles
        float recoilMagnitude
        float coolDown
        float maxDistance
        float radius
        float density
        float invMass
        uint8_t maxBounces

    cdef struct weaponPickupEntity:
        b2BodyId bodyID
        b2ShapeId shapeID
        weaponType weapon
        float respawnWait
        uint8_t floatingWallsTouching
        uint16_t mapCellIdx

    cdef struct projectileEntity:
        uint8_t droneIdx
        b2BodyId bodyID
        b2ShapeId shapeID
        weaponInformation *weaponInfo
        cachedPos pos
        b2Vec2 lastPos
        float distance
        uint8_t bounces

    cdef struct stepHitInfo:
        bint shotHit[_MAX_DRONES]
        bint explosionHit[_MAX_DRONES]

    cdef struct droneStats:
        float reward
        float distanceTraveled
        float absDistanceTraveled
        float shotsFired[_NUM_WEAPONS]
        float shotsHit[_NUM_WEAPONS]
        float shotsTaken[_NUM_WEAPONS]
        float ownShotsTaken[_NUM_WEAPONS]
        float weaponsPickedUp[_NUM_WEAPONS]
        float shotDistances[_NUM_WEAPONS]
        float wins

    cdef struct droneEntity:
        b2BodyId bodyID
        b2ShapeId shapeID
        weaponInformation *weaponInfo
        int8_t ammo
        float weaponCooldown
        uint16_t heat
        uint16_t charge
        bint shotThisStep

        uint8_t idx
        b2Vec2 initalPos
        cachedPos pos
        b2Vec2 lastPos
        b2Vec2 lastMove
        b2Vec2 lastAim
        b2Vec2 lastVelocity
        stepHitInfo hitInfo
        bint dead
        int killedBy
        int lives

        # New fields for multidiscrete action space
        uint8_t aim_action
        uint8_t booster_action
        uint8_t fire_action
        uint8_t rotation_speed_action

    cdef struct logEntry:
        float length
        droneStats stats[_MAX_DRONES]

    cdef struct logBuffer:
        logEntry *logs
        uint16_t size
        uint16_t capacity

    cdef struct rayClient:
        float scale
        uint16_t width
        uint16_t height
        uint16_t halfWidth
        uint16_t halfHeight

    cdef struct env:
        uint8_t numDrones
        uint8_t numAgents

        uint8_t *obs
        float *rewards
        int *actions
        uint8_t *terminals

        uint64_t randState
        bint needsReset

        uint16_t episodeLength
        logBuffer *logs
        droneStats stats[_MAX_DRONES]

        b2WorldId worldID
        uint8_t columns
        uint8_t rows
        mapBounds bounds
        weaponInformation *defaultWeapon
        CC_Array *cells
        CC_Array *walls
        CC_Array *floatingWalls
        CC_Array *drones
        CC_Array *pickups
        CC_SList *projectiles

        uint16_t stepsLeft
        uint16_t suddenDeathSteps
        uint8_t suddenDeathWallCounter

        rayClient *client

        uint8_t explosionSteps
        b2ExplosionDef explosion

# Wrapper functions for constants
def maxDrones() -> int:
    return MAX_DRONES


def obsConstants(numDrones: int) -> pufferlib.Namespace:
    return pufferlib.Namespace(
        obsSize=OBS_SIZE,
        mapObsSize=MAP_OBS_SIZE,
        scalarObsSize=SCALAR_OBS_SIZE,
        droneObsSize=DRONE_OBS_SIZE,
        wallTypes=NUM_WALL_TYPES + 1,
        weaponTypes=NUM_WEAPONS + 1,
        mapCellObsSize=MAP_CELL_OBS_SIZE,
        maxMapColumns=MAX_MAP_COLUMNS,
        maxMapRows=MAX_MAP_ROWS,
    )


cdef class CyImpulseWars:
    cdef:
        uint16_t numEnvs
        uint8_t numDrones
        bint render
        env* envs
        logBuffer *logs
        rayClient* rayClient
        int[:, :, :] actions  # Define actions as a 3D integer array

    def __init__(self, uint16_t numEnvs, uint8_t numDrones, uint8_t numAgents, uint8_t[:, :] observations, int[:, :, :] discrete_actions, float[:] rewards, uint8_t[:] terminals, uint64_t seed, bint render):
        self.numEnvs = numEnvs
        self.numDrones = numDrones
        self.render = render
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.logs = createLogBuffer(LOG_BUFFER_SIZE)

        # Initialize the actions array
        self.actions = np.zeros((numEnvs, numDrones, 4), dtype=np.int32).view(dtype=int[:, :, :])

        cdef int inc = numAgents
        cdef int i
        for i in range(self.numEnvs):
            initEnv(
                &self.envs[i],
                numDrones,
                numAgents,
                &observations[i * inc, 0],
                &self.actions[0, 0, 0],  # Pass the base pointer to actions
                &rewards[i * inc],
                &terminals[i * inc],
                self.logs,
                seed + i,
            )
        self.discrete_actions = discrete_actions


    cdef _initRaylib(self):
        self.rayClient = createRayClient()
        cdef int i
        for i in range(self.numEnvs):
            self.envs[i].client = self.rayClient

    def reset(self):
        if self.render and self.rayClient == NULL:
            self._initRaylib()

        cdef int i
        for i in range(self.numEnvs):
            resetEnv(&self.envs[i])

    def step(self):
        cdef int i, j
        cdef env* env_instance
        cdef CC_Array* drone_array
        cdef droneEntity* drone
        cdef int aim_action, booster_action, fire_action, rotation_speed_action

        for i in range(self.numEnvs):
            env_instance = &self.envs[i]
            drone_array = env_instance.drones  # Get CC_Array pointer

            for j in range(self.numDrones):
                # Access drone from CC_Array
                drone = <droneEntity*>safe_array_get_at(drone_array, j)
                
                # Extract multidiscrete actions
                aim_action = self.discrete_actions[i, j, 0]
                booster_action = self.discrete_actions[i, j, 1]
                fire_action = self.discrete_actions[i, j, 2]
                rotation_speed_action = self.discrete_actions[i, j, 3]

                # Assign actions to the drone
                drone.aim_action = aim_action
                drone.booster_action = booster_action
                drone.fire_action = fire_action
                drone.rotation_speed_action = rotation_speed_action

            # Step the environment
            stepEnv(env_instance)


    def log(self):
        cdef logEntry log = aggregateAndClearLogBuffer(self.numDrones, self.logs)
        return log

    def close(self):
        cdef int i
        for i in range(self.numEnvs):
            destroyEnv(&self.envs[i])

        destroyLogBuffer(self.logs)
        free(self.envs)

        if self.rayClient != NULL:
            destroyRayClient(self.rayClient)
