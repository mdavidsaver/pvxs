foreach t : [
    ['simplesrv', ['./simplesrv.cpp']],
    ['mailbox', ['./mailbox.cpp']],
    ['ticker', ['./ticker.cpp']],
    ['simpleget', ['./simpleget.cpp']],
    ['client', ['./client.cpp']],
    ['rpc_server', ['./rpc_server.cpp']],
    ['rpc_client', ['./rpc_client.cpp']],
  ]
  executable(
    t[0],
    t[1],
    dependencies: [epics_base, libCom, pvxs_dep],
  )
endforeach
