from libc.stdint cimport uint64_t
from libc.stdlib cimport calloc, free

import pufferlib

from impulse_wars cimport (
    MAX_DRONES,
    OBS_SIZE,
    ACTION_SIZE,
    OBS_HIGH,
    NUM_WEAPONS,
    NUM_WALL_TYPES,
    NUM_ENTITY_TYPES,
    SCALAR_OBS_SIZE,
    DRONE_OBS_SIZE,
    NUM_PROJECTILE_OBS,
    PROJECTILE_OBS_SIZE,
    NUM_FLOATING_WALL_OBS,
    FLOATING_WALL_OBS_SIZE,
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


# doesn't seem like you an directly import C or Cython constants 
# from Python so we have to create wrapper functions

def maxDrones() -> int:
    return MAX_DRONES

def obsSize() -> int:
    return OBS_SIZE

def actionsSize() -> int:
    return ACTION_SIZE

def obsHigh() -> float:
    return OBS_HIGH

def obsConstants() -> pufferlib.Namespace:
    weaponTypes = NUM_WEAPONS
    wallTypes = NUM_WALL_TYPES
    mapCellTypes = NUM_ENTITY_TYPES + NUM_WEAPONS
    droneObsSize = DRONE_OBS_SIZE
    numProjectileObs = NUM_PROJECTILE_OBS
    projectileObsSize = PROJECTILE_OBS_SIZE
    numFloatingWallObs = NUM_FLOATING_WALL_OBS
    floatingWallObsSize = FLOATING_WALL_OBS_SIZE
    maxMapColumns = MAX_MAP_COLUMNS
    maxMapRows = MAX_MAP_ROWS
    return pufferlib.Namespace(
        weaponTypes=NUM_WEAPONS,
        wallTypes=NUM_WALL_TYPES,
        mapCellTypes=OBS_HIGH,
        scalarObsSize=SCALAR_OBS_SIZE,
        droneObsSize=DRONE_OBS_SIZE,
        numProjectileObs=NUM_PROJECTILE_OBS,
        projectileObsSize=PROJECTILE_OBS_SIZE,
        numFloatingWallObs=NUM_FLOATING_WALL_OBS,
        floatingWallObsSize=FLOATING_WALL_OBS_SIZE,
        maxMapColumns=MAX_MAP_COLUMNS,
        maxMapRows=MAX_MAP_ROWS,
    )


cdef class CyImpulseWars:
    cdef:
        unsigned int numEnvs
        bint render
        env* envs
        logBuffer *logs
        rayClient* rayClient

    def __init__(self, unsigned int numEnvs, unsigned int numDrones, unsigned int numAgents, float[:, :] observations, float[:, :] actions, float[:] rewards, unsigned char[:] terminals, uint64_t seed, bint render):
        self.numEnvs = numEnvs
        self.render = render
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.logs = createLogBuffer(LOG_BUFFER_SIZE)

        cdef int inc = numAgents
        cdef int i
        for i in range(self.numEnvs):
            initEnv(
                &self.envs[i],
                numAgents,
                &observations[i * inc, 0],
                &actions[i * inc, 0],
                &rewards[i * inc],
                &terminals[i * inc],
                self.logs,
                seed + i,
                0,
            )

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
        cdef int i
        for i in range(self.numEnvs):
            stepEnv(&self.envs[i])

    def log(self):
        cdef logEntry log = aggregateAndClearLogBuffer(self.logs)
        return log

    def close(self):
        cdef int i
        for i in range(self.numEnvs):
            destroyEnv(&self.envs[i])

        destroyLogBuffer(self.logs)
        free(self.envs)

        if self.rayClient != NULL:
            destroyRayClient(self.rayClient)
