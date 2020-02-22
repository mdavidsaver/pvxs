/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvxs is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef CLIENTIMPL_H
#define CLIENTIMPL_H

#include <list>

#include <epicsTime.h>

#include <pvxs/client.h>

#include "evhelper.h"
#include "dataimpl.h"
#include "utilpvt.h"
#include "udp_collector.h"
#include "conn.h"

namespace pvxs {
namespace client {

struct Channel;

// internal actions on an Operation
struct OperationBase : public Operation
{
    std::shared_ptr<Channel> chan;
    uint32_t ioid;

    OperationBase(operation_t op, const std::shared_ptr<Channel>& chan);
    virtual ~OperationBase();

    virtual void createOp() =0;
    virtual void disconnected(const std::shared_ptr<OperationBase>& self) =0;
};

struct RequestInfo {
    const uint32_t ioid;
    const Operation::operation_t op;
    const std::weak_ptr<OperationBase> handle;

    Value prototype;

    RequestInfo(uint32_t ioid, std::shared_ptr<OperationBase>& handle);
};

struct Connection : public ConnBase {
    const std::shared_ptr<Context::Pvt> context;

    const evevent echoTimer;

    bool ready = false;

    // channels to be created on this Connection
    std::list<std::weak_ptr<Channel>> pending;

    std::map<uint32_t, std::weak_ptr<Channel>> creatingByCID,
                                               chanBySID;

    std::map<uint32_t, RequestInfo> opByIOID;

    uint32_t nextIOID = 0u;

    Connection(const std::shared_ptr<Context::Pvt>& context, const SockAddr &peerAddr);
    ~Connection();

    void createChannels();

    virtual void bevEvent(short events) override final;

    virtual void cleanup() override final;

#define CASE(Op) virtual void handle_##Op() override final;
    CASE(CONNECTION_VALIDATION);
    CASE(CONNECTION_VALIDATED);

    CASE(CREATE_CHANNEL);
    CASE(DESTROY_CHANNEL);

    CASE(GET_FIELD);
#undef CASE

protected:
    void tickEcho();
    static void tickEchoS(evutil_socket_t fd, short evt, void *raw);
};

struct Channel {
    const std::shared_ptr<Context::Pvt> context;
    const std::string name;
    // Our choosen ID for this channel.
    // used as persistent CID and searchID
    const uint32_t cid;

    enum state_t {
        Searching,  // waiting for a server to claim
        Connecting, // waiting for Connection to become ready
        Creating,   // waiting for reply to CREATE_CHANNEL
        Active,
    } state = Searching;

    std::shared_ptr<Connection> conn;
    uint32_t sid = 0u;

    // when state==Searching, number of repeatitions
    size_t nSearch = 0u;

    // GUID of last positive reply when state!=Searching
    std::array<uint8_t, 12> guid;
    SockAddr replyAddr;

    std::list<std::weak_ptr<OperationBase>> pending;

    Channel(const std::shared_ptr<Context::Pvt>& context, const std::string& name, uint32_t cid);
    ~Channel();

    void createOperations();

    static
    std::shared_ptr<Channel> build(const std::shared_ptr<Context::Pvt>& context, const std::string &name);
};

struct Context::Pvt
{
    std::weak_ptr<Pvt> internal_self;

    // "const" after ctor
    Config effective;

    const Value caMethod;

    uint32_t nextCID=0u;

    evsocket searchTx;
    uint16_t searchRxPort;

    epicsTimeStamp lastPoke{};

    std::vector<uint8_t> searchMsg;

    // search destination address and whether to set the unicast flag
    std::vector<std::pair<SockAddr, bool>> searchDest;

    size_t currentBucket = 0u;
    std::vector<std::list<std::weak_ptr<Channel>>> searchBuckets;

    std::list<std::unique_ptr<UDPListener> > beaconRx;

    struct BTrack {
        std::array<uint8_t, 12> guid;
        epicsTimeStamp lastRx;
    };
    std::map<SockAddr, BTrack> beaconSenders;

    std::map<uint32_t, std::weak_ptr<Channel>> chanByCID;
    std::map<std::string, std::weak_ptr<Channel>> chanByName;

    std::map<SockAddr, std::weak_ptr<Connection>> connByAddr;

    evbase tcp_loop;
    const evevent searchRx;
    const evevent searchTimer;
    const evevent beaconCleaner;

    Pvt(const Config& conf);
    ~Pvt();

    void close();

    void poke();

    void onBeacon(const UDPManager::Beacon& msg);

    bool onSearch();
    static void onSearchS(evutil_socket_t fd, short evt, void *raw);
    void tickSearch();
    static void tickSearchS(evutil_socket_t fd, short evt, void *raw);
    void tickBeaconClean();
    static void tickBeaconCleanS(evutil_socket_t fd, short evt, void *raw);
};

} // namespace client

} // namespace pvxs

#endif // CLIENTIMPL_H
