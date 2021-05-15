/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvxs is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include "osiSockExt.h"

#include <mswsock.h>

#include <pvxs/log.h>
#include "evhelper.h"

#include <epicsThread.h>
#include <cantProceed.h>

namespace pvxs {

DEFINE_LOGGER(log, "pvxs.util");

static
LPFN_WSARECVMSG WSARecvMsg;

static
epicsThreadOnceId oseOnce = EPICS_THREAD_ONCE_INIT;

static
void oseDoOnce(void*)
{
    evsocket dummy(AF_INET, SOCK_DGRAM, 0);
    GUID guid      = WSAID_WSARECVMSG;
    DWORD nout;

    if(WSAIoctl(dummy.sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
                &guid, sizeof(guid),
                &WSARecvMsg, sizeof(WSARecvMsg),
                &nout, nullptr, nullptr))
    {
        cantProceed("Unable to get &WSARecvMsg: %d", WSAGetLastError());
    }
    if(!WSARecvMsg)
        cantProceed("Unable to get &WSARecvMsg!!");
}

void osiSockAttachExt()
{
    osiSockAttach();
    epicsThreadOnce(&oseOnce, &oseDoOnce, nullptr);
}

void enable_SO_RXQ_OVFL(SOCKET sock) {}

void enable_IP_PKTINFO(SOCKET sock)
{
    int val = 1;
    if(setsockopt(sock, IPPROTO_IP, IP_PKTINFO, (char*)&val, sizeof(val)))
        log_warn_printf(log, "Unable to set IP_PKTINFO: %d\n", SOCKERRNO);
}

int recvfromx::call()
{
    WSAMSG msg{};

    WSABUF iov = {(ULONG)buflen, (char*)buf};
    msg.lpBuffers = &iov;
    msg.dwBufferCount = 1u;

    msg.name = &(*src)->sa;
    msg.namelen = src->size();

    alignas (alignof (WSACMSGHDR)) char cbuf[WSA_CMSG_SPACE(sizeof(in_pktinfo))];
    msg.Control = {sizeof(cbuf), cbuf};

    DWORD nrx=0u;
    if(!WSARecvMsg(sock, &msg, &nrx, nullptr, nullptr)) {
        if(msg.dwFlags & MSG_CTRUNC)
            log_debug_printf(log, "MSG_CTRUNC %zu, %zu\n", msg.Control.len, sizeof(cbuf));

        for(WSACMSGHDR *hdr = WSA_CMSG_FIRSTHDR(&msg); hdr ; hdr = WSA_CMSG_NXTHDR(&msg, hdr)) {
            if(dst && hdr->cmsg_level==IPPROTO_IP && hdr->cmsg_type==IP_PKTINFO && hdr->cmsg_len>=WSA_CMSG_LEN(sizeof(in_pktinfo))) {
                (*dst)->in.sin_family = AF_INET;
                memcpy(&(*dst)->in.sin_addr, WSA_CMSG_DATA(hdr) + offsetof(in_pktinfo, ipi_addr), sizeof(IN_ADDR));
            }
        }

        return nrx;

    } else {
        return -1;
    }
}

} // namespace pvxs
