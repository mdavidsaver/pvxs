
#include <vector>

#include <pvxs/util.h>
#include <pvxs/log.h>
#include <pvxs/data.h>
#include <pvxs/client.h>

using namespace pvxs;

namespace {

DEFINE_LOGGER(app, "tablerx");

// per-subscription accounting
struct SubInfo {
    std::shared_ptr<client::Subscription> sub;
    uint32_t nextcount = 0u;
    bool connected = false;
    bool bad = false;
};

} // namespace

int main(int argc, char *argv[])
{
    // looks at $PVXS_LOG
    // eg. PVXS_LOG="tablerx=DEBUG"
    logger_config_env();

    std::vector<std::shared_ptr<SubInfo> > subs;
    subs.reserve(argc);

    // we queue borrowed referenced to 'subs', which is safe as 'subs' will outlive 'workQ',
    // which must in turn outlive 'ctxt' as context worker threads borrow references to this queue.
    MPMCFIFO<SubInfo*> workQ;

    auto ctxt(client::Config::fromEnv().build());

    // subscription setup
    for(int i=1; i<argc; i++) {
        log_info_printf(app, "Subscribe to '%s'\n", argv[i]);

        auto info(std::make_shared<SubInfo>());
        auto rawinfo = info.get();

        info->sub = ctxt.monitor(argv[i])
                // want to receive both Connected and Disconnected events/exceptions
                .maskConnected(false)
                .maskDisconnected(false)
                .event([&workQ, rawinfo](client::Subscription& sub) {
                    // Callback on PVA worker thread.
                    // Add to work queue when subscription update queue becomes not empty
                    log_debug_printf(app, "%s : not empty\n", sub.name().c_str());
                    workQ.emplace(rawinfo);
                })
                .exec();

        subs.emplace_back(std::move(info));
    }

    log_info_printf(app, "Prepared %zu\n", subs.size());

    SigInt handler([&workQ](){
        // allow user to interrupt for graceful exit
        workQ.emplace(nullptr);
    });

    // waiting for work
    while(auto info = workQ.pop()) {
        unsigned n;
        // queuing fairness.  de-queue up to 4 events from a Subscription before considering the
        // next Subscription.
        for(n=0; n<4u; n++) {
            log_debug_printf(app, "%s : handle %u\n", info->sub->name().c_str(), n);
            try {
                if(auto top = info->sub->pop()) {
                    // process update
                    auto value(top["value"]);
                    bool foundcounter = false;

                    // consider each column
                    for(auto col : value.ichildren()) {
                        auto& colname(value.nameOf(col));

                        if(colname=="secondsPastEpoch" || colname=="nanoseconds") {
                            // TODO: sanity check?

                        } else if(col.type()!=TypeCode::UInt64A) {
                            if(!info->bad) {
                                log_warn_printf(app, "%s : unsupported type %s\n",
                                                info->sub->name().c_str(), col.type().name());
                                info->bad = true;
                            }

                        } else {
                            // a counter column
                            foundcounter = true;

                            auto counts(col.as<shared_array<const uint64_t>>());
                            if(counts.empty()) {
                                if(!info->bad) {
                                    log_warn_printf(app, "%s : empty array for column %s\n",
                                                    info->sub->name().c_str(), colname.c_str());
                                    info->bad = true;
                                }

                            } else {
                                log_debug_printf(app, "%s : [0x%08x, 0x%08x]\n",
                                                 info->sub->name().c_str(),
                                                 unsigned(counts.front()),
                                                 unsigned(counts.back()));
                                if(info->nextcount != counts[0]) {
                                    log_warn_printf(app, "%s : skip 0x%08x -> 0x%08x\n",
                                                    info->sub->name().c_str(),
                                                    unsigned(info->nextcount-1u), unsigned(counts[0]));
                                }
                                info->nextcount = counts.back()+1u;
                            }
                            break; // only checking one counter column
                        }
                    }

                    if(!foundcounter && !info->bad) {
                        log_warn_printf(app, "%s : no counter columns\n",
                                        info->sub->name().c_str());
                        info->bad = true;
                    }


                } else {
                    // monitor queue is empty
                    break;
                }
            }catch(client::Connected& e){
                log_info_printf(app, "%s : connected\n", info->sub->name().c_str());
                info->connected = true;
                info->bad = false;

            }catch(client::Discovered& e){
                log_info_printf(app, "%s : disconnected\n", info->sub->name().c_str());
                info->connected = false;

            }catch(std::exception& e){
                log_err_printf(app, "%s : error %s\n", info->sub->name().c_str(), e.what());
            }
        }
        if(n==4u) // not empty
            workQ.push(info); // reschedule
    }

    log_info_printf(app, "Done%s", "\n");

    return 0;
}
