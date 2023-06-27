tests = [
  ['testsock', ['./testsock.cpp']],
  ['testutil', ['./testutil.cpp']],
  ['testev', ['./testev.cpp']],
  ['testlog', ['./testlog.cpp']],
  ['testudp', ['./testudp.cpp']],
  ['testshared', ['./testshared.cpp']],
  ['testbitmask', ['./testbitmask.cpp']],
  ['testxcode', ['./testxcode.cpp']],
  ['testtype', ['./testtype.cpp']],
  ['testdata', ['./testdata.cpp']],
  ['testnt', ['./testnt.cpp']],
  ['testconfig', ['./testconfig.cpp']],
  ['testwild', ['./testwild.cpp']],
  ['testpvreq', ['./testpvreq.cpp']],
  ['testinfo', ['./testinfo.cpp']],
  ['testget', ['./testget.cpp']],
  ['testmon', ['./testmon.cpp']],
  ['testput', ['./testput.cpp']],
  ['testrpc', ['./testrpc.cpp']],
  ['test1000', ['./test1000.cpp']],
  ['testendian', ['./testendian.cpp']],
]

epics_version = epics_base.version().split('.')

# TODO: registerRecordDeviceDriver?
# if epics_version[0] == '3' and epics_version[1] == '15'
#   tests += [
# endif

foreach t : tests
  test = executable(
    t[0],
    t[1],
    dependencies: [epics_base, libCom, pvxs_dep],
  )
  test(t[0], test, is_parallel: true)
endforeach

if epics_version[0] == '7' and epics_version[1] == '0'
  benchmark = executable(
    'benchdata',
    ['./benchdata.cpp'],
    dependencies: [epics_base, libCom, pvxs_dep],
  )
  benchmark('benchdata', benchmark)
endif

# Development utilities, not tests
foreach t : [
    ['mcat', ['./mcat.cpp']],
    ['spam', ['./spam.cpp']],
    ['eatspam', ['./eatspam.cpp']],
  ]
  test = executable(
    t[0],
    t[1],
    dependencies: [epics_base, libCom, pvxs_dep],
  )
endforeach
