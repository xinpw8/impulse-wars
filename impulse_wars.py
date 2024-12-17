import gymnasium
import numpy as np

import pufferlib

from cy_impulse_wars import (
    getObsSize,
    getActionsSize,
    CyImpulseWars,
)


NUM_DRONES = 2


class ImpulseWars(pufferlib.PufferEnv):
    def __init__(self, num_envs: int, seed: int = 0, render_mode: str = None, buf=None):
        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=1.0, shape=(getObsSize(),), dtype=np.float32
        )
        self.single_action_space = gymnasium.spaces.Box(
            low=-1.0, high=1.0, shape=(getActionsSize(),), dtype=np.float32
        )
        self.render_mode = render_mode
        self.num_agents = NUM_DRONES * num_envs

        super().__init__(buf)
        self.c_envs = CyImpulseWars(
            self.observations, self.actions, self.rewards, self.terminals, num_envs, seed
        )

    def reset(self, seed=None):
        self.c_envs.reset()
        return self.observations, []

    def step(self, actions):
        self.actions[:] = actions
        self.c_envs.step()

        return self.observations, self.rewards, self.terminals, self.truncations, []

    def render(self):
        pass

    def close(self):
        self.c_envs.close()


def testPerf(timeout=10, actionCache=1024, numEnvs=1024):
    env = ImpulseWars(numEnvs)

    actions = np.random.randint(
        env.single_action_space.low[0],
        env.single_action_space.high[1],
        (actionCache, env.num_agents, getActionsSize()),
    )

    import time

    tick = 0
    start = time.time()
    while time.time() - start < timeout:
        action = actions[tick % actionCache]
        env.step(action)
        tick += 1

    sps = numEnvs * tick / (time.time() - start)
    print(f"SPS: {sps:,}")

    env.close()


if __name__ == "__main__":
    testPerf(timeout=10, actionCache=1024, numEnvs=10)
