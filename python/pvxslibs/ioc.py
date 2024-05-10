import ctypes
import os

from epicscorelibs import ioc
from setuptools_dso.runtime import find_dso
import pvxslibs.path

os.environ.setdefault("PVXS_QSRV_ENABLE", "YES")

if __name__ == "__main__":
    extra_dbd_load = [(b"pvxsIoc.dbd", pvxslibs.path.dbd_path)]
    extra_dso_load = ["pvxslibs.lib.pvxsIoc"]
    ioc.main(extra_dbd_load=extra_dbd_load,extra_dso_load=extra_dso_load)
