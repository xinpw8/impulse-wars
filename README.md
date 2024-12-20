# Impulse Wars

A C reinforcement learning environment that is very closely based off of the game Retrograde Arena. 

## Building

Build the Python module with `make python-module-release`. You can then run the `main.py` file to train a policy or evaluate one. 

A recent C compiler is required to build, I use clang 18 but GCC 14 will work, you may have to remove the `-Werror` flag in `CMakeLists.txt`. Python 3.11 is what I'm developing with, I make no promises for other versions. `scikit-core-build` is used to build the Python module, but will be installed automatically if the correct make command is invoked. `autopxd2` is used to generate declarations in a PXD file for the Cython code, which will automatically be installed as well. There are a few parts of my C headers that `autopxd2` fails to parse, but they are guarded by defines. 


