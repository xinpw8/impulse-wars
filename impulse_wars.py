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
    def __init__(self, num_envs: int, render_mode: str = None, buf=None):
        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=1.0, shape=(getObsSize(),), dtype=np.float32
        )
        self.single_action_space = gymnasium.spaces.Box(
            low=-1.0, high=1.0, shape=(getActionsSize(),), dtype=np.float32
        )
        self.render_mode = render_mode
        self.num_agents = NUM_DRONES

        super().__init__(buf)
        print(self.observations.shape)
        self.env = CyImpulseWars(self.observations, self.actions, self.rewards, self.terminals, num_envs)


if __name__ == "__main__":
    env = ImpulseWars(1)
