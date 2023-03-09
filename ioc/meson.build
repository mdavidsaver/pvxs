install_data(['./pvxsIoc.dbd'], install_dir: dbd_install_dir)

libpvxsIoc = library(
  'pvxsIoc',
  ['./iochooks.cpp'],
  cpp_args: ['-DPVXS_IOC_API_BUILDING'],
  dependencies: [epics_base, libCom, pvxs_dep],
  install: true,
  install_dir: lib_install_dir,
  # HACK: Workaround https://github.com/mesonbuild/meson/issues/6541
  install_rpath: epics_base_libdir,
)

install_headers(
  ['./pvxs/iochooks.h'],
  subdir: 'pvxs',
)

pkgconfig = import('pkgconfig')
pkgconfig.generate(libpvxsIoc)
