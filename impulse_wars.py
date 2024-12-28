from typing import Dict

import gymnasium
import numpy as np

import pufferlib

from cy_impulse_wars import (
    maxDrones,
    obsConstants,
    CyImpulseWars,
)


def transformRawLog(numDrones: int, rawLog: Dict[str, float]):
    log = {"length": rawLog["length"]}

    count = 0
    for i, stats in enumerate(rawLog["stats"]):
        log[f"drone_{i}_reward"] = stats["reward"]
        log[f"drone_{i}_wins"] = stats["wins"]
        log[f"drone_{i}_distance_traveled"] = stats["distanceTraveled"]
        log[f"drone_{i}_abs_distance_traveled"] = stats["absDistanceTraveled"]
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
        report_interval=16,
        buf=None,
    ):
        if num_drones > maxDrones() or num_drones <= 0:
            raise ValueError(f"num_drones must greater than 0 and less than or equal to {maxDrones()}")
        if num_agents > num_drones or num_agents <= 0:
            raise ValueError(f"num_agents must greater than 0 and less than or equal to num_drones")

        self.numDrones = num_drones
        self.obsInfo = obsConstants(num_drones)

        # Define the multidiscrete action space
        self.single_action_space = gymnasium.spaces.MultiDiscrete([
            17,  # Aiming: 16 directions (cardinal, diagonal, intermediates) + noop
            5,   # Booster impulse: 4 cardinal directions + noop
            2,   # Fire weapon: 0 (noop), 1 (fire)
            4    # Rotation speed: 0 (noop), 1 (slow), 2 (medium), 3 (fast)
        ])

        self.single_observation_space = gymnasium.spaces.Box(
            low=0.0, high=255, shape=(self.obsInfo.obsSize,), dtype=np.uint8
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
    actions = np.random.randint(
        0,
        env.single_action_space.nvec,
        size=(actionCache, env.num_agents, 4),  # Updated for multidiscrete
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
