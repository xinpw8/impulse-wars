# cython: language_level=3

from libc.stdlib cimport calloc, free
import numpy as np

from env cimport createEnv, setupEnv, destroyEnv

def test_c():
    e = createEnv()
    setupEnv(e)
    print(f"loaded map: columns={e.columns} rows={e.rows}")
    destroyEnv(e)
