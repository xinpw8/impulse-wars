from libc.stdint cimport uint64_t
from libc.stdlib cimport calloc, free

from impulse_wars cimport (
    OBS_SIZE,
    ACTION_SIZE,
    env,
    initEnv,
    resetEnv,
    stepEnv,
    destroyEnv,
)


cdef int NUM_DRONES = 2

def getObsSize() -> int:
    return OBS_SIZE

def getActionsSize() -> int:
    return ACTION_SIZE


cdef class CyImpulseWars:
    cdef:
        env* envs
        int numEnvs

    def __init__(self, float[:, :] observations, float[:, :] actions, float[:] rewards, unsigned char[:] terminals, unsigned int numEnvs, uint64_t seed):
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.numEnvs = numEnvs

        cdef int inc = NUM_DRONES
        cdef int i
        for i in range(self.numEnvs):
            initEnv(
                &self.envs[i],
                &observations[i * inc, 0],
                &actions[i * inc, 0],
                &rewards[i * inc],
                &terminals[i * inc],
                seed + i,
            )

    def reset(self):
        cdef int i
        for i in range(self.numEnvs):
            resetEnv(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.numEnvs):
            stepEnv(&self.envs[i])

    def close(self):
        cdef int i
        for i in range(self.numEnvs):
            destroyEnv(&self.envs[i])

        free(self.envs)
