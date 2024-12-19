import gymnasium
import numpy as np

import pufferlib

from cy_impulse_wars import (
    obsHigh,
    numDrones,
    obsSize,
    actionsSize,
    CyImpulseWars,
)


def transformRawLog(rawLog):
    log = {"length": rawLog["length"]}
    for i, reward in enumerate(rawLog["reward"]):
        log[f"drone_{i}_reward"] = reward

    for i, stats in enumerate(rawLog["stats"]):
        log[f"drone_{i}_distance_traveled"] = stats["distanceTraveled"]
        log[f"drone_{i}_shots_fired"] = sum(stats["shotsFired"])
        log[f"drone_{i}_shots_hit"] = sum(stats["shotsHit"])
        log[f"drone_{i}_shots_taken"] = sum(stats["shotsTaken"])
        log[f"drone_{i}_own_shots_taken"] = sum(stats["ownShotsTaken"])
        log[f"drone_{i}_weapons_picked_up"] = sum(stats["weaponsPickedUp"])
        log[f"drone_{i}_shots_distance"] = sum(stats["shotDistances"])

    return log


class ImpulseWars(pufferlib.PufferEnv):
    def __init__(self, num_envs: int, seed: int = 0, report_interval=8, render_mode: str = None, buf=None):
        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=obsHigh(), shape=(obsSize(),), dtype=np.float32
        )
        self.single_action_space = gymnasium.spaces.Box(
            low=-1.0, high=1.0, shape=(actionsSize(),), dtype=np.float32
        )

        self.report_interval = report_interval
        self.render_mode = render_mode
        self.num_agents = numDrones() * num_envs
        self.tick = 0

        super().__init__(buf)
        self.c_envs = CyImpulseWars(
            self.observations, self.actions, self.rewards, self.terminals, num_envs, seed
        )

    def reset(self, seed=None):
        self.c_envs.reset()
        self.tick = 0
        return self.observations, []

    def step(self, actions):
        self.actions[:] = actions
        self.c_envs.step()

        infos = []
        self.tick += 1
        if self.tick % self.report_interval == 0:
            rawLog = self.c_envs.log()
            if rawLog["length"] > 0:
                infos.append(transformRawLog(rawLog))

        return self.observations, self.rewards, self.terminals, self.truncations, infos

    def render(self):
        pass

    def close(self):
        self.c_envs.close()


def testPerf(timeout, actionCache, numEnvs):
    env = ImpulseWars(numEnvs)

    import time

    np.random.seed(int(time.time()))
    actions = np.random.uniform(
        env.single_action_space.low[0],
        env.single_action_space.high[0],
        (actionCache, env.num_agents, actionsSize()),
    )

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
    testPerf(timeout=10, actionCache=10240, numEnvs=1)
