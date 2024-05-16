import os

from epicscorelibs import ioc
import pvxslibs.path

if __name__ == "__main__":
    os.environ.setdefault("PVXS_QSRV_ENABLE", "YES")
    extra_dbd_load = (("pvxsIoc.dbd", pvxslibs.path.dbd_path),)
    extra_dso_load = ({"dso":"pvxslibs.lib.pvxsIoc"},)
    ioc.main(extra_dbd_load=extra_dbd_load,extra_dso_load=extra_dso_load)
