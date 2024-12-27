# Impulse Wars

A C reinforcement learning environment that is very closely based off of the game Retrograde Arena. 

## Building

Build the Python module with `make`. You can then run the `main.py` file to train a policy or evaluate one. 

Python 3.11 is what I'm developing with, I make no promises for other versions. `scikit-core-build` is used to build the Python module, but will be installed automatically if the correct make command is invoked. `autopxd2` is used to generate declarations in a PXD file for the Cython code, which will automatically be installed as well. There are a few parts of my C headers that `autopxd2` fails to parse, but they are guarded by defines. 

## Structure

### Python

- `main.py` contains the main training/evaluation loop
- `policy.py` contains the neural network policy
- `impulse_wars.py` defines the Python environment
- `cy_impulse_wars.pyx` is the Cython wrapper between Python and C

### C environment (`src` directory)

- `include` directory contains a few deps from GitHub I converted to be header only
- `helpers.h` defines small helper functions and macros
- `types.h` defines most of the types used throughout the project. It's in it's own file to prevent circular dependencies
- `settings.h` defines general game and environment settings, as well as weapon handling settings/logic
- `map.h` contains all map layouts and map setup logic
- `game.h` contains the game logic
- `env.h` contains the RL environment logic
