from typing import Tuple

from gymnasium import spaces
import numpy as np
import torch as th
from torch import nn
from torch.distributions.normal import Normal

import pufferlib
from pufferlib.models import LSTMWrapper
from pufferlib.pytorch import layer_init

from cy_impulse_wars import (
    obsConstants,
    numDrones,
    obsSize,
)


weaponTypesEmbeddingDims = 4
floatingWallTypesEmbeddingDims = 2
mapCellsEmbeddingDims = 4
encoderOutputSize = 256
lstmOutputSize = 256


class Recurrent(LSTMWrapper):
    def __init__(self, env: pufferlib.PufferEnv, policy: nn.Module):
        super().__init__(env, policy, input_size=encoderOutputSize, hidden_size=lstmOutputSize)


class Policy(nn.Module):
    def __init__(self, env: pufferlib.PufferEnv):
        super().__init__()

        self.is_continuous = True

        self.obsInfo = obsConstants()
        self.numDrones = numDrones()
        self.obsSize = obsSize()
        self.droneOffset = self.obsInfo.scalarObsSize + (self.numDrones * self.obsInfo.droneObsSize)
        self.projectileObsTotalSize = self.obsInfo.numProjectileObs * self.obsInfo.projectileObsSize
        self.projectileOffset = self.droneOffset + self.projectileObsTotalSize
        self.floatingWallObsTotalSize = self.obsInfo.numFloatingWallObs * self.obsInfo.floatingWallObsSize
        self.floatingWallOffset = self.projectileOffset + self.floatingWallObsTotalSize

        self.weaponTypeEmbedding = nn.Embedding(self.obsInfo.weaponTypes + 1, weaponTypesEmbeddingDims)
        self.floatingWallTypeEmbedding = nn.Embedding(self.obsInfo.wallTypes, floatingWallTypesEmbeddingDims)
        self.mapCellEmbedding = nn.Embedding(self.obsInfo.mapCellTypes + 1, mapCellsEmbeddingDims)

        self.mapCNN = nn.Sequential(
            layer_init(nn.Conv2d(mapCellsEmbeddingDims, 32, kernel_size=5, stride=2)),
            nn.ReLU(),
            layer_init(nn.Conv2d(32, 32, kernel_size=3, stride=2)),
            nn.ReLU(),
            nn.Flatten(),
        )
        cnnOutputSize = self._computeCNNShape()
        featuresSize = (
            self.obsInfo.scalarObsSize
            + (self.numDrones * (weaponTypesEmbeddingDims + self.obsInfo.droneObsSize - 1))
            + (
                self.obsInfo.numProjectileObs
                * (weaponTypesEmbeddingDims + self.obsInfo.projectileObsSize - 1)
            )
            + (
                self.obsInfo.numFloatingWallObs
                * (floatingWallTypesEmbeddingDims + self.obsInfo.floatingWallObsSize - 1)
            )
            + cnnOutputSize
        )

        self.encoder = nn.Sequential(
            layer_init(nn.Linear(featuresSize, encoderOutputSize)),
            nn.Tanh(),
        )

        self.actorMean = layer_init(nn.Linear(lstmOutputSize, env.single_action_space.shape[0]), std=0.01)
        self.actorLogStd = nn.Parameter(th.zeros(1, env.single_action_space.shape[0]))

        self.critic = layer_init(nn.Linear(lstmOutputSize, 1), std=1.0)

    def forward(self, obs: th.Tensor) -> Tuple[th.Tensor, th.Tensor]:
        hidden = self.encode_observations(obs)
        actions, value = self.decode_actions(hidden)
        return actions, value

    def encode_observations(self, obs: th.Tensor) -> th.Tensor:
        batchSize = obs.shape[0]
        offset = 0

        # process scalar observations
        scalarObs = obs[:, offset : self.obsInfo.scalarObsSize]
        offset += self.obsInfo.scalarObsSize

        # process drone observations
        droneWeapons = th.zeros(
            (batchSize, self.numDrones, weaponTypesEmbeddingDims), dtype=th.float32, device=obs.device
        )
        droneObs = th.zeros(
            (batchSize, self.numDrones, self.obsInfo.droneObsSize - 1), dtype=th.float32, device=obs.device
        )
        for i in range(self.numDrones):
            weaponType = obs[:, offset].to(th.int)
            offset += 1
            droneWeapons[:, i] = self.weaponTypeEmbedding(weaponType)
            droneObs[:, i] = obs[:, offset : offset + self.obsInfo.droneObsSize - 1]
            offset += self.obsInfo.droneObsSize - 1

        # process projectile observations
        projectileWeapons = th.zeros(
            (batchSize, self.obsInfo.numProjectileObs, weaponTypesEmbeddingDims),
            dtype=th.float32,
            device=obs.device,
        )
        projectileObs = th.zeros(
            (batchSize, self.obsInfo.numProjectileObs, self.obsInfo.projectileObsSize - 1),
            dtype=th.float32,
            device=obs.device,
        )
        for i in range(self.obsInfo.numProjectileObs):
            weaponType = obs[:, offset].to(th.int)
            offset += 1
            projectileWeapons[:, i] = self.weaponTypeEmbedding(weaponType)
            projectileObs[:, i] = obs[:, offset : offset + self.obsInfo.projectileObsSize - 1]
            offset += self.obsInfo.projectileObsSize - 1

        # process floating wall observations
        floatingWallTypes = th.zeros(
            (batchSize, self.obsInfo.numFloatingWallObs, floatingWallTypesEmbeddingDims),
            dtype=th.float32,
            device=obs.device,
        )
        floatingWallObs = th.zeros(
            (batchSize, self.obsInfo.numFloatingWallObs, self.obsInfo.floatingWallObsSize - 1),
            dtype=th.float32,
            device=obs.device,
        )
        for i in range(self.obsInfo.numFloatingWallObs):
            wallType = obs[:, offset].to(th.int)
            offset += 1
            floatingWallTypes[:, i] = self.floatingWallTypeEmbedding(wallType)
            floatingWallObs[:, i] = obs[:, offset : offset + self.obsInfo.floatingWallObsSize - 1]
            offset += self.obsInfo.floatingWallObsSize - 1

        # process map cell observations
        mapCellTypes = obs[:, offset : offset + (self.obsSize - offset)].to(th.int)
        mapCellTypes = mapCellTypes.view(batchSize, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows)
        mapCellTypes = self.mapCellEmbedding(mapCellTypes)
        mapCellTypes = mapCellTypes.permute(0, 3, 1, 2)
        mapCells = self.mapCNN(mapCellTypes)

        # encode all observations
        drones = th.cat((droneWeapons, droneObs), dim=-1)
        drones = th.flatten(drones, start_dim=-2, end_dim=-1)
        projectiles = th.cat((projectileWeapons, projectileObs), dim=-1)
        projectiles = th.flatten(projectiles, start_dim=-2, end_dim=-1)
        floatingWalls = th.cat((floatingWallTypes, floatingWallObs), dim=-1)
        floatingWalls = th.flatten(floatingWalls, start_dim=-2, end_dim=-1)
        features = th.cat((drones, projectiles, floatingWalls, mapCells, scalarObs), dim=-1)

        return self.encoder(features), None

    def decode_actions(self, hidden: th.Tensor, lookup=None):
        actionMean = self.actorMean(hidden)
        actionLogStd = self.actorLogStd.expand_as(actionMean)
        actionStd = th.exp(actionLogStd)
        action = Normal(actionMean, actionStd)

        value = self.critic(hidden)

        return action, value

    def _computeCNNShape(self) -> int:
        mapSpace = spaces.Box(
            low=0,
            high=self.obsInfo.mapCellTypes,
            shape=(self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows),
            dtype=np.int,
        )

        with th.no_grad():
            t = th.as_tensor(mapSpace.sample()[None])
            e = self.mapCellEmbedding(t).to(th.float32)
            e = e.permute(0, 3, 1, 2)
            return self.mapCNN(e).shape[1]
