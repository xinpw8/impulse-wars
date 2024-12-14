from libc.stdlib cimport calloc, free
import numpy as np

from impulse_wars cimport createEnv, setupEnv, destroyEnv

def test_c():
    e = createEnv()
    setupEnv(e)
    print(f"loaded map: columns={e.columns} rows={e.rows}")
    destroyEnv(e)
