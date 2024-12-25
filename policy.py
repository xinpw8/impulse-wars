from typing import Tuple

from gymnasium import spaces
import numpy as np
import torch as th
from torch import nn
from torch.distributions.normal import Normal

import pufferlib
from pufferlib.models import LSTMWrapper
from pufferlib.pytorch import layer_init

from cy_impulse_wars import obsConstants


cnnChannels = 32
encoderOutputSize = 256
lstmOutputSize = 256


class Recurrent(LSTMWrapper):
    def __init__(self, env: pufferlib.PufferEnv, policy: nn.Module):
        super().__init__(env, policy, input_size=encoderOutputSize, hidden_size=lstmOutputSize)


class Policy(nn.Module):
    def __init__(self, env: pufferlib.PufferEnv, numDrones: int, device: str = "cuda"):
        super().__init__()

        self.is_continuous = True

        self.numDrones = numDrones
        self.obsInfo = obsConstants(numDrones)

        self.factors = np.array(
            [
                self.obsInfo.wallTypes,  # wall types
                self.obsInfo.weaponTypes,  # weapon pickup types
                self.obsInfo.weaponTypes,  # projectile types
                self.obsInfo.wallTypes,  # floating wall types
                self.obsInfo.weaponTypes,  # drone weapon types
            ]
        )
        self.offsets = th.tensor([0] + list(np.cumsum(self.factors)[:-1])).view(1, -1, 1, 1)
        self.cumFactors = np.cumsum(self.factors)
        self.multihotDim = self.factors.sum()

        self.mapCNN = nn.Sequential(
            layer_init(nn.Conv2d(self.multihotDim, cnnChannels, kernel_size=5, stride=2)),
            nn.ReLU(),
            layer_init(nn.Conv2d(cnnChannels, cnnChannels, kernel_size=3, stride=2)),
            nn.ReLU(),
            nn.Flatten(),
        )
        cnnOutputSize = self._computeCNNShape()

        featuresSize = cnnOutputSize + self.obsInfo.scalarObsSize

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
        mapObs = obs[:, : self.obsInfo.mapObsSize].view(
            batchSize, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows, self.obsInfo.mapCellObsSize
        )
        droneObs = obs[:, self.obsInfo.mapObsSize :].float() / 255.0

        mapBuf = th.zeros(
            batchSize,
            self.multihotDim,
            self.obsInfo.maxMapColumns,
            self.obsInfo.maxMapRows,
            device=obs.device,
            dtype=th.float32,
        )
        codes = mapObs.permute(0, 3, 1, 2) + self.offsets
        mapBuf.scatter_(1, codes, 1)
        mapObs = self.mapCNN(mapBuf)

        features = th.cat((mapObs, droneObs), dim=-1)

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
            high=1,
            shape=(self.multihotDim, self.obsInfo.maxMapColumns, self.obsInfo.maxMapRows),
            dtype=np.float32,
        )

        with th.no_grad():
            t = th.as_tensor(mapSpace.sample()[None])
            return self.mapCNN(t).shape[1]
