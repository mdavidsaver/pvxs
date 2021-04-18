/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvxs is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <list>
#include <atomic>

#include <epicsVersion.h>
#include <epicsGetopt.h>
#include <epicsThread.h>

#include <pvxs/client.h>
#include <pvxs/nt.h>
#include <pvxs/log.h>
#include "utilpvt.h"
#include "evhelper.h"

using namespace pvxs;

namespace {

void usage(const char* argv0)
{
    std::cerr<<"Usage: "<<argv0<<" [IP[:Port] ...]\n"
                                 "\n"
                                 "  -h        Show this message.\n"
                                 "  -V        Print version and exit.\n"
                                 "  -i        Print server info.  Requires address(es)\n"
                                 "  -v        Make more noise.\n"
                                 "  -d        Shorthand for $PVXS_LOG=\"pvxs.*=DEBUG\".  Make a lot of noise.\n"
                                 "  -w <sec>  Operation timeout in seconds.  default 5 sec.\n"
               ;
}

} // namespace

int main(int argc, char *argv[])
{
    try {
        logger_config_env(); // from $PVXS_LOG
        double timeout = 5.0;
        bool verbose = false;
        bool info = false;

        {
            int opt;
            while ((opt = getopt(argc, argv, "hVivdw:")) != -1) {
                switch(opt) {
                case 'h':
                    usage(argv[0]);
                    return 0;
                case 'V':
                    std::cout<<version_str()<<"\n";
                    std::cout<<EPICS_VERSION_STRING<<"\n";
                    std::cout<<"libevent "<<event_get_version()<<"\n";
                    return 0;
                case 'i':
                    info = true;
                    break;
                case 'v':
                    verbose = true;
                    break;
                case 'd':
                    logger_level_set("pvxs.*", Level::Debug);
                    break;
                case 'w':
                    timeout = parseTo<double>(optarg);
                    break;
                default:
                    usage(argv[0]);
                    std::cerr<<"\nUnknown argument: "<<char(opt)<<std::endl;
                    return 1;
                }
            }
        }

        if(info && optind==argc) {
            usage(argv[0]);
            std::cerr<<"\nError: -i requires at least one server"<<std::endl;
            return 1;
        }

        auto ctxt = client::Config::fromEnv().build();
        auto conf = ctxt.config();

        epicsEvent done;
        SigInt H([&done]() {
            done.signal();
        });

        std::vector<std::shared_ptr<client::Operation>> ops;
        ops.reserve(argc-optind);

        if(optind==argc) { // discover mode, search of all servers
            std::set<ServerGUID> guids; // captured by value (copy), really local to the lambda

            ops.push_back(ctxt.discover([guids, verbose](const client::Discovered& serv) mutable {
                // avoid printing duplicates, unless the GUID changes (restart)
                if(guids.find(serv.guid)!=guids.end())
                    return;
                guids.insert(serv.guid);

                std::cout<<serv.server;

                if(verbose) {
                    std::cout<<" guid="<<serv.guid<<" via="<<serv.peer;
                }

                std::cout<<std::endl;
            })
                          .exec());

        } else { // query mode, fetch info from specific servers

            std::atomic<int> remaining{argc-optind};

            for(auto n : range(optind, argc)) {
                ops.push_back(ctxt.rpc("server")
                              .server(argv[n])
                              .arg("op", info ? "info" : "channels")
                              .result([argv, n, info, verbose, &remaining, &done](client::Result&& r)
                      {
                          try {
                              auto top(r());

                              if(info) {
                                  std::cout<<argv[n];
                                  std::string temp;
                                  if(top["version"].as(temp)) {
                                      std::cout<<" version=\""<<escape(temp)<<"\"";
                                  };
                                  if(top["implLang"].as(temp)) {
                                      std::cout<<" lang=\""<<escape(temp)<<"\"";
                                  };
                                  std::cout<<"\n";

                              } else { // channels
                                  if(verbose)
                                      std::cout<<"# From "<<argv[n]<<"\n";

                                  auto channels(top["value"].as<shared_array<const std::string>>());
                                  for(auto& name : channels) {
                                      std::cout<<name<<"\n";
                                  }
                              }
                              std::cout.flush();
                          }catch(std::exception& e){
                              std::cerr<<"From "<<argv[n]<<" : "<<e.what()<<std::endl;
                          }

                          if(0==--remaining) {
                              done.signal();
                          }
                      })
                              .exec());
            }
        }

        if(timeout>0.0)
            done.wait(timeout);
        else
            done.wait();

        return 0;

    }catch(std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return 1;
    }
}
