from typing import Dict

import gymnasium
import numpy as np

import pufferlib

from cy_impulse_wars import (
    maxDrones,
    obsHigh,
    obsConstants,
    actionsSize,
    CyImpulseWars,
)


def transformRawLog(numDrones: int, rawLog: Dict[str, float]):
    log = {"length": rawLog["length"]}

    count = 0
    for i, reward in enumerate(rawLog["reward"]):
        log[f"drone_{i}_reward"] = reward
        count += 1
        if count >= numDrones:
            break
        

    count = 0
    for i, stats in enumerate(rawLog["stats"]):
        log[f"drone_{i}_distance_traveled"] = stats["distanceTraveled"]
        log[f"drone_{i}_shots_fired"] = sum(stats["shotsFired"])
        log[f"drone_{i}_shots_hit"] = sum(stats["shotsHit"])
        log[f"drone_{i}_shots_taken"] = sum(stats["shotsTaken"])
        log[f"drone_{i}_own_shots_taken"] = sum(stats["ownShotsTaken"])
        log[f"drone_{i}_weapons_picked_up"] = sum(stats["weaponsPickedUp"])
        log[f"drone_{i}_shots_distance"] = sum(stats["shotDistances"])
        count += 1
        if count >= numDrones:
            break

    return log


class ImpulseWars(pufferlib.PufferEnv):
    def __init__(
        self,
        num_envs: int = 1,
        num_drones: int = 2,
        num_agents: int = 2,
        seed: int = 0,
        render: bool = False,
        report_interval=8,
        buf=None,
    ):
        if num_drones > maxDrones() or num_drones <= 0:
            raise ValueError(f"num_drones must greater than 0 and less than or equal to {maxDrones()}")
        if num_agents > num_drones or num_agents <= 0:
            raise ValueError(f"num_agents must greater than 0 and less than or equal to num_drones")

        self.numDrones = num_drones
        self.obsInfo = obsConstants(num_drones)

        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=obsHigh(), shape=(self.obsInfo.obsSize,), dtype=np.float32
        )
        self.single_action_space = gymnasium.spaces.Box(
            low=-1.0, high=1.0, shape=(actionsSize(),), dtype=np.float32
        )

        self.report_interval = report_interval
        self.render_mode = "human" if render else None
        self.num_agents = num_agents * num_envs
        self.tick = 0

        super().__init__(buf)
        self.c_envs = CyImpulseWars(
            num_envs,
            num_drones,
            num_agents,
            self.observations,
            self.actions,
            self.rewards,
            self.terminals,
            seed,
            render,
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
                infos.append(transformRawLog(self.numDrones, rawLog))

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
    testPerf(timeout=5, actionCache=1024, numEnvs=1)
