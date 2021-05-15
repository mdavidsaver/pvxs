/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * pvxs is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */

#include "osiSockExt.h"

#include <string.h>

#include <pvxs/log.h>

namespace pvxs {

DEFINE_LOGGER(log, "pvxs.util");

void osiSockAttachExt() {}

void enable_SO_RXQ_OVFL(SOCKET sock)
{
#ifdef SO_RXQ_OVFL
    // Linux specific feature exposes OS dropped packet count
    int val = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_RXQ_OVFL, (char*)&val, sizeof(val)))
        log_warn_printf(log, "Unable to set SO_RXQ_OVFL: %d\n", SOCKERRNO);

#endif
}

void enable_IP_PKTINFO(SOCKET sock)
{
#ifdef IP_PKTINFO
#define sizeof_in_pktinfo sizeof(in_pktinfo)
    int val = 1;
    if(setsockopt(sock, IPPROTO_IP, IP_PKTINFO, (char*)&val, sizeof(val)))
        log_warn_printf(log, "Unable to set IP_PKTINFO: %d\n", SOCKERRNO);
#else
#define sizeof_in_pktinfo (0u)
#endif
}

int recvfromx::call()
{
    msghdr msg{};

    iovec iov = {buf, buflen};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1u;

    msg.msg_name = &(*src)->sa;
    msg.msg_namelen = src ? src->size() : 0u;

    alignas (alignof (cmsghdr)) char cbuf[CMSG_SPACE(4u) + CMSG_SPACE(sizeof_in_pktinfo)];
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    if(dst)
        *dst = SockAddr();
    ndrop = 0u;

    int ret = recvmsg(sock, &msg, 0);

    if(ret>=0) {
        if(msg.msg_flags & MSG_CTRUNC)
            log_debug_printf(log, "MSG_CTRUNC %zu, %zu\n", msg.msg_controllen, sizeof(cbuf));

        for(cmsghdr *hdr = CMSG_FIRSTHDR(&msg); hdr ; hdr = CMSG_NXTHDR(&msg, hdr)) {
            if(0) {}
#ifdef SO_RXQ_OVFL
            else if(hdr->cmsg_level==SOL_SOCKET && hdr->cmsg_type==SO_RXQ_OVFL && hdr->cmsg_len>=CMSG_LEN(4u)) {
                memcpy(&ndrop, CMSG_DATA(hdr), 4u);
            }
#endif
#ifdef IP_PKTINFO
            else if(dst && hdr->cmsg_level==IPPROTO_IP && hdr->cmsg_type==IP_PKTINFO && hdr->cmsg_len>=CMSG_LEN(sizeof(in_pktinfo))) {
                (*dst)->in.sin_family = AF_INET;
                memcpy(&(*dst)->in.sin_addr, CMSG_DATA(hdr) + offsetof(in_pktinfo, ipi_addr), sizeof(in_addr_t));
            }
#endif
        }
    }

    return ret;
}

} // namespace pvxs
