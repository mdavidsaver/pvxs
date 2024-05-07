import ctypes
import os

from epicscorelibs import ioc
from setuptools_dso.runtime import find_dso
import pvxslibs.path

# TODO: Quetion: Will this work if these lines are here or should they be inside epicscorelibs?
pvxsIoc = ctypes.CDLL(find_dso("pvxslibs.lib.pvxsIoc"), ctypes.RTLD_GLOBAL)
os.environ.setdefault("PVXS_QSRV_ENABLE", "YES")

if __name__ == "__main__":
    db_load = dbd_load=[(b'base.dbd',None), (b'qsrv.dbd',None),(b"pvxsIoc.dbd", pvxslibs.path.dbd_path)]
    ioc.main(db_load=dbd_load)