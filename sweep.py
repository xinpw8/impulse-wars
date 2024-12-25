from math import log2
import logging
import os
import subprocess
import sys
import time

from rich.console import Console

from carbs import CARBS, CARBSParams, LinearSpace, LogitSpace, LogSpace, Param, WandbLoggingParams
import wandb
from wandb_carbs import WandbCarbs, create_sweep


logging.basicConfig(level=logging.INFO)


class CustomWandbCarbs(WandbCarbs):
    def __init__(self, carbs: CARBS, wandb_run=None):
        super().__init__(carbs, wandb_run)

    def _transform_suggestion(self, suggestion):
        print(f"Suggestion pre-transformation: {suggestion}")

        suggestion["bptt_horizon"] = 2 ** suggestion["bptt_horizon"]

        return suggestion

    def _suggestion_from_run(self, run):
        suggestion = super()._suggestion_from_run(run)

        suggestion["bptt_horizon"] = int(log2(suggestion["bptt_horizon"]))

        return suggestion


def sweep(args, train):
    params = [
        Param(
            name="total_timesteps",
            space=LinearSpace(
                min=50_000_000, scale=100_000_000, rounding_factor=10_000_000, is_integer=True
            ),
            search_center=100_000_000,
        ),
        # hyperparams
        Param(
            name="bptt_horizon",
            space=LinearSpace(min=4, max=8, scale=3, is_integer=True),
            search_center=5,
        ),
        Param(name="ent_coef", space=LogSpace(scale=1.0), search_center=0.0005),
        Param(name="gae_lambda", space=LogitSpace(min=0.0, max=1.0), search_center=0.90),
        Param(name="gamma", space=LogitSpace(min=0.0, max=1.0), search_center=0.95),
        Param(name="learning_rate", space=LogSpace(scale=1.0), search_center=0.0001),
        Param(name="max_grad_norm", space=LinearSpace(min=0.0, scale=2.0), search_center=1.0),
        Param(name="vf_coef", space=LogitSpace(min=0.0, max=1.0), search_center=0.5),
    ]

    sweepID = args.wandb_sweep
    if not sweepID:
        sweepID = create_sweep(
            sweep_name=args.wandb_name,
            wandb_entity=args.wandb_entity,
            wandb_project=args.wandb_project,
            carb_params=params,
        )

    if args.sweep_child:
        try:
            trainWithSuggestion(args, params, train)
        except Exception:
            Console().print_exception()
        os._exit(0)

    def launchTrainingProcess():
        childArgs = ["python", "main.py", "--mode=sweep", "--sweep-child", f"--wandb-sweep={sweepID}"]
        if args.train.compile:
            childArgs.append("--train.compile")
        print(f"running child training process with args: {childArgs}")
        ret = subprocess.run(args=childArgs, stdin=sys.stdin, stdout=sys.stdout, stderr=sys.stderr)
        ret.check_returncode()

    wandb.agent(
        sweep_id=sweepID, entity=args.wandb_entity, project=args.wandb_project, function=launchTrainingProcess
    )


def trainWithSuggestion(args, params, train):
    args.track = False

    try:
        config = CARBSParams(
            seed=int(time.time()),
            better_direction_sign=1,
            max_suggestion_cost=1800,  # 30m
            num_random_samples=2 * len(params),
            initial_search_radius=0.5,
            num_candidates_for_suggestion_per_dim=100,
            wandb_params=WandbLoggingParams(root_dir="wandb"),
        )
        carbs = CARBS(config=config, params=params)

        wandbCarbs = CustomWandbCarbs(carbs=carbs)
        resampling = not carbs._is_random_sampling() and len(carbs.success_observations) > (
            (carbs.resample_count + 1) * carbs.config.resample_frequency
        )
        print(
            f"loaded CARBS from wandb: success={len(carbs.success_observations)} failures={len(carbs.failure_observations)} outstanding={len(carbs.outstanding_suggestions)} resample_count={carbs.resample_count} resampling={resampling} random_sampling={carbs._is_random_sampling()} "
        )
        args.wandb = wandb

        suggestion = wandbCarbs.suggest()
        del suggestion["suggestion_uuid"]
        print(f"Suggestion: {suggestion}")

        args.train.__dict__.update(dict(suggestion))
        print(f"Training args: {args.train}")

        startTime = time.time()
        stats = train(args)
    except Exception:
        Console().print_exception()
        wandbCarbs.record_failure()
        wandb.finish()
        return

    totalTime = time.time() - startTime
    obj = stats["drone_0_shots_hit"] + stats["drone_1_shots_hit"]
    wandbCarbs.record_observation(objective=obj, cost=totalTime)
    wandb.finish()
