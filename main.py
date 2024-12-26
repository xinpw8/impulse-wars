from pdb import set_trace as T
import argparse
import os

import numpy as np
import pufferlib
import pufferlib.vector
import pufferlib.cleanrl
import torch as th
import wandb

from rich_argparse import RichHelpFormatter
from rich.console import Console
from rich.traceback import install

install(show_locals=False)

import clean_pufferl

from policy import Policy, Recurrent
from impulse_wars import ImpulseWars


def make_policy(env, config):
    """Make the policy for the environment"""
    policy = Policy(env, config.num_drones)
    policy = Recurrent(env, policy)
    return pufferlib.cleanrl.RecurrentPolicy(policy)


def train(args):
    if args.track and args.mode != "sweep":
        args.wandb = init_wandb(args, args.wandb_name, id=args.train.exp_id)
        args.train.__dict__.update(dict(args.wandb.config.train))
    elif not args.track and args.train.exp_id is None:
        args.train.exp_id = wandb.util.generate_id()

    vecenv = pufferlib.vector.make(
        ImpulseWars,
        num_envs=args.vec.num_envs,
        env_args=(args.train.num_envs,),
        env_kwargs=dict(
            num_drones=args.train.num_drones, num_agents=args.train.num_agents, seed=args.train.seed
        ),
        num_workers=args.vec.num_workers,
        batch_size=args.vec.env_batch_size,
        zero_copy=args.vec.zero_copy,
        backend=pufferlib.vector.Multiprocessing,
    )
    policy = make_policy(vecenv.driver_env, args.train).to(args.train.device)

    data = clean_pufferl.create(args.train, vecenv, policy, wandb=args.wandb)

    try:
        stats = {}

        while data.global_step < args.train.total_timesteps:
            newStats, _ = clean_pufferl.evaluate(data)
            if newStats:
                stats = newStats
            clean_pufferl.train(data)
    except KeyboardInterrupt as e:
        clean_pufferl.close(data)
        raise e
    except Exception as e:
        Console().print_exception()
        clean_pufferl.close(data)
        raise e

    clean_pufferl.close(data)

    return stats


def init_wandb(args, name, id=None, resume=True):
    wandb.init(
        id=id or wandb.util.generate_id(),
        project=args.wandb_project,
        entity=args.wandb_entity,
        group=args.wandb_group,
        config={
            "train": dict(args.train),
            "vec": dict(args.vec),
        },
        name=name,
        save_code=True,
        resume=resume,
    )
    return wandb


def eval_policy(env: pufferlib.PufferEnv, policy, device, data=None, bestEval: float = None, printInfo=False):
    steps = 0
    totalReward = 0.0

    state = None
    ob, _ = env.reset()
    while True:
        with th.no_grad():
            ob = th.as_tensor(ob).to(device)
            if hasattr(policy, "lstm"):
                actions, _, _, _, state = policy(ob, state)
            else:
                actions, _, _, _ = policy(ob)

            action = actions.cpu().numpy().reshape(env.action_space.shape)
            action = np.clip(action, env.single_action_space.low[0], env.single_action_space.high[0])

        ob, reward, done, trunc, info = env.step(action)
        totalReward += reward
        steps += 1

        if done.any() or trunc.any():
            break

    print(f"Steps: {steps}, Reward: {totalReward}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=f":blowfish: PufferLib [bright_cyan]{pufferlib.__version__}[/]"
        " demo options. Shows valid args for your env and policy",
        formatter_class=RichHelpFormatter,
        add_help=False,
    )
    parser.add_argument(
        "--mode",
        type=str,
        default="train",
        choices="train eval playtest autotune sweep".split(),
    )
    parser.add_argument("--sweep-child", action="store_true")
    parser.add_argument("--eval-model-path", type=str, default=None, help="Path to model to evaluate")
    parser.add_argument("--render", action="store_false", help="Enable rendering")
    parser.add_argument("--cell-id", type=int, default=0)
    parser.add_argument("--wandb-entity", type=str, default="xinpw8", help="WandB entity")
    parser.add_argument("--wandb-project", type=str, default="", help="WandB project")
    parser.add_argument("--wandb-group", type=str, default="", help="WandB group")
    parser.add_argument("--wandb-name", type=str, default="", help="WandB run name")
    parser.add_argument("--wandb-sweep", type=str, default="", help="Wandb sweep ID")
    parser.add_argument("--track", action="store_true", help="Track on WandB")

    parser.add_argument("--train.data-dir", type=str, default="checkpoints")
    parser.add_argument("--train.exp-id", type=str, default=None)
    parser.add_argument("--train.seed", type=int, default=-1)
    parser.add_argument("--train.torch-deterministic", action="store_true")
    parser.add_argument("--train.cpu-offload", action="store_false")
    parser.add_argument("--train.device", type=str, default="cuda" if th.cuda.is_available() else "cpu")
    parser.add_argument("--train.total-timesteps", type=int, default=250_000_000)
    parser.add_argument("--train.checkpoint-interval", type=int, default=25)
    parser.add_argument("--train.eval-interval", type=int, default=1_000_000)
    parser.add_argument("--train.compile", action="store_true")
    parser.add_argument("--train.compile-mode", type=str, default="reduce-overhead")

    parser.add_argument("--train.num-envs", type=int, default=256)
    parser.add_argument("--train.batch-size", type=int, default=262_144)
    parser.add_argument("--train.bptt-horizon", type=int, default=32)
    parser.add_argument("--train.clip-coef", type=float, default=0.2)
    parser.add_argument("--train.clip-vloss", action="store_false")
    parser.add_argument("--train.ent-coef", type=float, default=0.0005)
    parser.add_argument("--train.gae-lambda", type=float, default=0.90)
    parser.add_argument("--train.gamma", type=float, default=0.99)
    parser.add_argument("--train.learning-rate", type=float, default=0.0003)
    parser.add_argument("--train.anneal-lr", action="store_true")
    parser.add_argument("--train.max-grad-norm", type=float, default=0.5)
    parser.add_argument("--train.minibatch-size", type=int, default=32_768)
    parser.add_argument("--train.norm-adv", action="store_false")
    parser.add_argument("--train.update-epochs", type=int, default=1)
    parser.add_argument("--train.vf-clip-coef", type=float, default=0.1)
    parser.add_argument("--train.vf-coef", type=float, default=0.5321276235227259)
    parser.add_argument("--train.target-kl", type=float, default=0.2)

    parser.add_argument("--train.num-drones", type=int, default=2)
    parser.add_argument("--train.num-agents", type=int, default=2)

    parser.add_argument("--vec.num-envs", type=int, default=288)
    parser.add_argument("--vec.num-workers", type=int, default=24)
    parser.add_argument("--vec.env-batch-size", type=int, default=36)
    parser.add_argument("--vec.zero-copy", action="store_true")
    parsed = parser.parse_args()

    args = {}
    for k, v in vars(parsed).items():
        if "." in k:
            group, name = k.split(".")
            if group not in args:
                args[group] = {}

            args[group][name] = v
        else:
            args[k] = v

    args["train"] = pufferlib.namespace(**args["train"])
    args["vec"] = pufferlib.namespace(**args["vec"])
    args = pufferlib.namespace(**args)

    args.train.env = "impulse_wars"

    if args.train.seed == -1:
        args.train.seed = np.random.randint(2**32 - 1, dtype="int64").item()

    if args.mode == "train":
        try:
            args.wandb = None
            train(args)
            if args.track:
                wandb.finish()
        except KeyboardInterrupt:
            os._exit(0)
        except Exception:
            Console().print_exception()
            os._exit(0)
    elif args.mode == "eval":
        vecenv = pufferlib.vector.make(
            ImpulseWars,
            num_envs=1,
            env_args=(1,),
            env_kwargs=dict(
                num_drones=args.train.num_drones,
                num_agents=args.train.num_agents,
                render=True,
                seed=args.train.seed,
            ),
            num_workers=1,
            batch_size=1,
            backend=pufferlib.PufferEnv,
        )

        if args.eval_model_path is None:
            policy = make_policy(vecenv, args.train).to(args.train.device)
        else:
            policy = th.load(args.eval_model_path, map_location=args.train.device)

        eval_policy(vecenv, policy, args.train.device)
    elif args.mode == "sweep":
        from sweep import sweep

        sweep(args, train)
