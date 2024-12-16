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


def getObsSize() -> int:
    return OBS_SIZE

def getActionsSize() -> int:
    return ACTION_SIZE


cdef class CyImpulseWars:
    cdef:
        env* envs
        int numEnvs

    def __init__(self, float[:, :] observations, float[:] actions, float[:] rewards, unsigned char[:] terminals, int numEnvs):
        self.envs = <env*>calloc(numEnvs, sizeof(env))
        self.numEnvs = numEnvs

        cdef int i
        for i in range(numEnvs):
            initEnv(
                &self.envs[i],
                &observations[i, 0],
                &actions[i],
                &rewards[i],
                &terminals[i],
            )

    def reset(self):
        cdef int i
        for i in range(self.numEnvs):
            resetEnv(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.num_envs):
            stepEnv(&self.envs[i])

    def close(self):
        cdef int i
        for i in range(self.num_envs):
            destroyEnv(&self.envs[i])

        free(self.envs)
