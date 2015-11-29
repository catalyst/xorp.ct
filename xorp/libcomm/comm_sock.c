/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/* vim:set sts=4 ts=8 sw=4: */
/*
 * Copyright (c) 2001
 * YOID Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */



/*
 * COMM socket library lower `sock' level implementation.
 */

#include "comm_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/random.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif


#include "comm_api.h"
#include "comm_private.h"

#ifdef L_ERROR
char addr_str_255[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
#endif

/* XXX: Single threaded socket errno, used to record last error code. */
int _comm_serrno;


int comm_sock_open(int domain, int type, int protocol, int is_blocking)
{
    int sock;

    /* Create the kernel socket */
    sock = socket(domain, type, protocol);
    if (sock == -1 ) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error opening socket (domain = %d, type = %d, "
		"protocol = %d): %s",
		domain, type, protocol,
		comm_get_error_str(comm_get_last_error()));
	return -1;
    }

    /* Set the receiving and sending socket buffer size in the kernel */
    if (comm_sock_set_rcvbuf(sock, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	    < SO_RCV_BUF_SIZE_MIN) 
    {
	_comm_set_serrno();
	comm_sock_close(sock);
	return (-1);
    }
    if (comm_sock_set_sndbuf(sock, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	    < SO_SND_BUF_SIZE_MIN) 
    {
	_comm_set_serrno();
	comm_sock_close(sock);
	return (-1);
    }

    /* Enable TCP_NODELAY */
    if (type == SOCK_STREAM && (domain == AF_INET || domain == AF_INET6) 
	    && comm_set_nodelay(sock, 1) != XORP_OK) 
    {
	_comm_set_serrno();
	comm_sock_close(sock);
	return (-1);
    }

    /* Set blocking mode */
    if (comm_sock_set_blocking(sock, is_blocking) != XORP_OK) 
    {
	_comm_set_serrno();
	comm_sock_close(sock);
	return (-1);
    }

    return (sock);
}

int comm_sock_pair(int domain, int type, int protocol, int sv[2])
{
    if (socketpair(domain, type, protocol, sv) == -1) 
    {
	_comm_set_serrno();
	return (XORP_ERROR);
    }
    return (XORP_OK);

}

int comm_sock_bind4(int sock, const struct in_addr *my_addr,
	unsigned short my_port)
{
    int family;
    struct sockaddr_in sin_addr;

    family = comm_sock_get_family(sock);
    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&sin_addr, 0, sizeof(sin_addr));
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    sin_addr.sin_len = sizeof(sin_addr);
#endif
    sin_addr.sin_family = (u_char)family;
    sin_addr.sin_port = my_port;		/* XXX: in network order */
    if (my_addr != NULL)
	sin_addr.sin_addr.s_addr = my_addr->s_addr; /* XXX: in network order */
    else
	sin_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error binding socket (family = %d, "
		"my_addr = %s, my_port = %d): %s",
		family,
		(my_addr)? inet_ntoa(*my_addr) : "ANY",
		ntohs(my_port),
		comm_get_error_str(comm_get_last_error()));
	return XORP_ERROR;
    }
    else 
    {
	XLOG_INFO("Bound socket (family = %d, my_addr = %s, my_port = %d)",
		family, (my_addr)? inet_ntoa(*my_addr) : "ANY", ntohs(my_port));
    }

    return XORP_OK;
}

int comm_sock_bind6(int sock, const struct in6_addr *my_addr,
	unsigned int my_ifindex, unsigned short my_port)
{
#ifdef HAVE_IPV6
    int family;
    struct sockaddr_in6 sin6_addr;

    family = comm_sock_get_family(sock);
    if (family != AF_INET6) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&sin6_addr, 0, sizeof(sin6_addr));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
    sin6_addr.sin6_len = sizeof(sin6_addr);
#endif
    sin6_addr.sin6_family = (u_char)family;
    sin6_addr.sin6_port = my_port;		/* XXX: in network order     */
    sin6_addr.sin6_flowinfo = 0;		/* XXX: unused (?)	     */
    if (my_addr != NULL)
	memcpy(&sin6_addr.sin6_addr, my_addr, sizeof(sin6_addr.sin6_addr));
    else
	memcpy(&sin6_addr.sin6_addr, &in6addr_any, sizeof(sin6_addr.sin6_addr));

    sin6_addr.sin6_scope_id = my_ifindex;

    if (bind(sock, (struct sockaddr *)&sin6_addr, sizeof(sin6_addr)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error binding socket (family = %d, "
		"my_addr = %s, my_port = %d scope: %d): %s",
		family,
		(my_addr)?
		inet_ntop(family, my_addr, addr_str_255, sizeof(addr_str_255))
		: "ANY",
		ntohs(my_port), sin6_addr.sin6_scope_id,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }
    else 
    {
	XLOG_INFO("Bound socket (family = %d, my_addr = %s, my_port = %d scope: %d)",
		family,
		(my_addr) ? inet_ntop(family, my_addr, addr_str_255, sizeof(addr_str_255)) : "ANY",
		ntohs(my_port), sin6_addr.sin6_scope_id);
    }

    return XORP_OK;
#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_sock_bind6", sock, my_addr, my_ifindex, my_port);
    return XORP_ERROR;
#endif /* ! HAVE_IPV6 */
}

int comm_sock_bind(int sock, const struct sockaddr *sin)
{
    switch (sin->sa_family) 
    {
	case AF_INET:
	    {
		const struct sockaddr_in *sin4 = (const struct sockaddr_in *)((const void *)sin);
		return comm_sock_bind4(sock, &sin4->sin_addr, sin4->sin_port);
	    }
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    {
		const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)((const void *)sin);
		return comm_sock_bind6(sock, &sin6->sin6_addr, sin6->sin6_scope_id,
			sin6->sin6_port);
	    }
	    break;
#endif /* HAVE_IPV6 */
	default:
	    XLOG_FATAL("Error comm_sock_bind invalid family = %d", sin->sa_family);
	    return (XORP_ERROR);
    }

    XLOG_UNREACHABLE();

    return XORP_ERROR;
}

int comm_sock_join4(int sock, const struct in_addr *mcast_addr,
	const struct in_addr *my_addr)
{
    int family;
    struct ip_mreq imr;		/* the multicast join address */

    family = comm_sock_get_family(sock);
    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&imr, 0, sizeof(imr));
    imr.imr_multiaddr.s_addr = mcast_addr->s_addr;
    if (my_addr != NULL)
	imr.imr_interface.s_addr = my_addr->s_addr;
    else
	imr.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(const void*)&imr, sizeof(imr)) < 0) 
    {
	char mcast_addr_str[32], my_addr_str[32];
	_comm_set_serrno();
	strncpy(mcast_addr_str, inet_ntoa(*mcast_addr),
		sizeof(mcast_addr_str) - 1);
	mcast_addr_str[sizeof(mcast_addr_str) - 1] = '\0';
	if (my_addr != NULL)
	    strncpy(my_addr_str, inet_ntoa(*my_addr),
		    sizeof(my_addr_str) - 1);
	else
	    strncpy(my_addr_str, "ANY", sizeof(my_addr_str) - 1);
	my_addr_str[sizeof(my_addr_str) - 1] = '\0';
	XLOG_ERROR("Error joining mcast group (family = %d, "
		"mcast_addr = %s my_addr = %s): %s",
		family, mcast_addr_str, my_addr_str,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_sock_join6(int sock, const struct in6_addr *mcast_addr,
	unsigned int my_ifindex)
{
#ifdef HAVE_IPV6
    int family;
    struct ipv6_mreq imr6;	/* the multicast join address */

    family = comm_sock_get_family(sock);
    if (family != AF_INET6) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&imr6, 0, sizeof(imr6));
    memcpy(&imr6.ipv6mr_multiaddr, mcast_addr, sizeof(*mcast_addr));
    imr6.ipv6mr_interface = my_ifindex;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		(const void*)(&imr6), sizeof(imr6)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error joining mcast group (family = %d, "
		"mcast_addr = %s my_ifindex = %d): %s",
		family,
		inet_ntop(family, mcast_addr, addr_str_255, sizeof(addr_str_255)),
		my_ifindex, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_sock_join6", sock, mcast_addr, my_ifindex);
    return (XORP_ERROR);
#endif /* ! HAVE_IPV6 */
}

int comm_sock_leave4(int sock, const struct in_addr *mcast_addr,
	const struct in_addr *my_addr)
{
    int family;
    struct ip_mreq imr;		/* the multicast leave address */

    family = comm_sock_get_family(sock);
    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&imr, 0, sizeof(imr));
    imr.imr_multiaddr.s_addr = mcast_addr->s_addr;
    if (my_addr != NULL)
	imr.imr_interface.s_addr = my_addr->s_addr;
    else
	imr.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(const void*)(&imr), sizeof(imr)) < 0) 
    {
	char mcast_addr_str[32], my_addr_str[32];
	_comm_set_serrno();
	strncpy(mcast_addr_str, inet_ntoa(*mcast_addr),
		sizeof(mcast_addr_str) - 1);
	mcast_addr_str[sizeof(mcast_addr_str) - 1] = '\0';
	if (my_addr != NULL)
	    strncpy(my_addr_str, inet_ntoa(*my_addr),
		    sizeof(my_addr_str) - 1);
	else
	    strncpy(my_addr_str, "ANY", sizeof(my_addr_str) - 1);
	my_addr_str[sizeof(my_addr_str) - 1] = '\0';
	XLOG_ERROR("Error leaving mcast group (family = %d, "
		"mcast_addr = %s my_addr = %s): %s",
		family, mcast_addr_str, my_addr_str,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_sock_leave6(int sock, const struct in6_addr *mcast_addr,
	unsigned int my_ifindex)
{
#ifdef HAVE_IPV6
    int family;
    struct ipv6_mreq imr6;	/* the multicast leave address */

    family = comm_sock_get_family(sock);
    if (family != AF_INET6) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&imr6, 0, sizeof(imr6));
    memcpy(&imr6.ipv6mr_multiaddr, mcast_addr, sizeof(*mcast_addr));
    imr6.ipv6mr_interface = my_ifindex;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
		(const void*)(&imr6), sizeof(imr6)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error leaving mcast group (family = %d, "
		"mcast_addr = %s my_ifindex = %d): %s",
		family,
		inet_ntop(family, mcast_addr, addr_str_255, sizeof(addr_str_255)),
		my_ifindex, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_sock_leave6", sock, mcast_addr, my_ifindex);
    return (XORP_ERROR);
#endif /* ! HAVE_IPV6 */
}

int comm_sock_connect4(int sock, const struct in_addr *remote_addr,
	unsigned short remote_port, int is_blocking,
	int *in_progress)
{
    int family;
    struct sockaddr_in sin_addr;

    if (in_progress != NULL)
	*in_progress = 0;

    family = comm_sock_get_family(sock);
    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&sin_addr, 0, sizeof(sin_addr));
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    sin_addr.sin_len = sizeof(sin_addr);
#endif
    sin_addr.sin_family = (u_char)family;
    sin_addr.sin_port = remote_port;		/* XXX: in network order */
    sin_addr.sin_addr.s_addr = remote_addr->s_addr; /* XXX: in network order */

    if (connect(sock, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) 
    {
	_comm_set_serrno();
	if (! is_blocking) 
	{
	    if (comm_get_last_error() == EINPROGRESS) 
	    {
		/*
		 * XXX: The connection is non-blocking, and the connection
		 * cannot be completed immediately, therefore set the
		 * in_progress flag to 1 and return an error.
		 */
		if (in_progress != NULL)
		    *in_progress = 1;
		return (XORP_ERROR);
	    }
	}

	XLOG_ERROR("Error connecting socket (family = %d, "
		"remote_addr = %s, remote_port = %d): %s",
		family, inet_ntoa(*remote_addr), ntohs(remote_port),
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_sock_connect6(int sock, const struct in6_addr *remote_addr,
	unsigned short remote_port, int is_blocking,
	int *in_progress)
{
#ifdef HAVE_IPV6
    int family;
    struct sockaddr_in6 sin6_addr;

    if (in_progress != NULL)
	*in_progress = 0;

    family = comm_sock_get_family(sock);
    if (family != AF_INET6) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&sin6_addr, 0, sizeof(sin6_addr));
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
    sin6_addr.sin6_len = sizeof(sin6_addr);
#endif
    sin6_addr.sin6_family = (u_char)family;
    sin6_addr.sin6_port = remote_port;		/* XXX: in network order     */
    sin6_addr.sin6_flowinfo = 0;		/* XXX: unused (?)	     */
    memcpy(&sin6_addr.sin6_addr, remote_addr, sizeof(sin6_addr.sin6_addr));
    sin6_addr.sin6_scope_id = 0;		/* XXX: unused (?)	     */

    if (connect(sock, (struct sockaddr *)&sin6_addr, sizeof(sin6_addr)) < 0) 
    {
	_comm_set_serrno();
	if (! is_blocking) 
	{
	    if (comm_get_last_error() == EINPROGRESS) 
	    {
		/*
		 * XXX: The connection is non-blocking, and the connection
		 * cannot be completed immediately, therefore set the
		 * in_progress flag to 1 and return an error.
		 */
		if (in_progress != NULL)
		    *in_progress = 1;
		return (XORP_ERROR);
	    }
	}

	XLOG_ERROR("Error connecting socket (family = %d, "
		"remote_addr = %s, remote_port = %d): %s",
		family,
		(remote_addr)?
		inet_ntop(family, remote_addr, addr_str_255, sizeof(addr_str_255))
		: "ANY",
		ntohs(remote_port),
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! HAVE_IPV6 */
    if (in_progress != NULL)
	*in_progress = 0;

    comm_sock_no_ipv6("comm_sock_connect6", sock, remote_addr, remote_port,
	    is_blocking);
    return (XORP_ERROR);
#endif /* ! HAVE_IPV6 */
}

int comm_sock_connect(int sock, const struct sockaddr *sin, int is_blocking,
	int *in_progress)
{
    switch (sin->sa_family) 
    {
	case AF_INET:
	    {
		const struct sockaddr_in *sin4 = (const struct sockaddr_in *)((const void *)sin);
		return comm_sock_connect4(sock, &sin4->sin_addr, sin4->sin_port,
			is_blocking, in_progress);
	    }
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    {
		const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)((const void *)sin);
		return comm_sock_connect6(sock, &sin6->sin6_addr, sin6->sin6_port,
			is_blocking, in_progress);
	    }
	    break;
#endif /* HAVE_IPV6 */
	default:
	    XLOG_FATAL("Error comm_sock_connect invalid family = %d",
		    sin->sa_family);
	    return (XORP_ERROR);
    }

    XLOG_UNREACHABLE();

    return XORP_ERROR;
}

int comm_sock_accept(int sock)
{
    int sock_accept;
    struct sockaddr addr;
    socklen_t socklen = sizeof(addr);

    sock_accept = accept(sock, &addr, &socklen);
    if (sock_accept == -1 ) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error accepting socket %d: %s",
		sock, comm_get_error_str(comm_get_last_error()));
	return -1;
    }


    /* Enable TCP_NODELAY */
    if ((addr.sa_family == AF_INET || addr.sa_family == AF_INET6)
	    && comm_set_nodelay(sock_accept, 1) != XORP_OK) 
    {
	comm_sock_close(sock_accept);
	return -1;
    }

    return (sock_accept);
}

int comm_sock_listen(int sock, int backlog)
{
    int ret;
    ret = listen(sock, backlog);

    if (ret < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error listening on socket (socket = %d) : %s",
		sock, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_sock_close(int sock)
{
    int ret;

    ret = close(sock);

    if (ret < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error closing socket (socket = %d) : %s",
		sock, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_send_broadcast(int sock, int val)
{
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_BROADCAST on socket %d: %s",
		(val)? "set": "reset",  sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_receive_broadcast(int sock, int val)
{
    UNUSED(sock);
    UNUSED(val);
    return (XORP_OK);
}

int comm_set_nodelay(int sock, int val)
{
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s TCP_NODELAY on socket %d: %s",
		(val)? "set": "reset",  sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_linger(int sock, int enabled, int secs)
{
#ifdef SO_LINGER
    struct linger l;

    l.l_onoff = enabled;
    l.l_linger = secs;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, 
		(const void*)(&l), sizeof(l)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_LINGER %ds on socket %d: %s",
		(enabled)? "set": "reset", secs, sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! SO_LINGER */
    UNUSED(sock);
    UNUSED(enabled);
    UNUSED(secs);
    XLOG_WARNING("SO_LINGER Undefined!");

    return (XORP_ERROR);
#endif /* ! SO_LINGER */
}

int comm_set_keepalive(int sock, int val)
{
#ifdef SO_KEEPALIVE
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_KEEPALIVE on socket %d: %s",
		(val)? "set": "reset", sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! SO_KEEPALIVE */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_KEEPALIVE Undefined!");

    return (XORP_ERROR);
#endif /* ! SO_KEEPALIVE */
}

int comm_set_nosigpipe(int sock, int val)
{
#ifdef SO_NOSIGPIPE
    if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_NOSIGPIPE on socket %d: %s",
		(val)? "set": "reset",  sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else /* ! SO_NOSIGPIPE */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_NOSIGPIPE Undefined!");

    return (XORP_ERROR);
#endif /* ! SO_NOSIGPIPE */
}

int comm_set_reuseaddr(int sock, int val)
{
#ifdef SO_REUSEADDR
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_REUSEADDR on socket %d: %s",
		(val)? "set": "reset", sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! SO_REUSEADDR */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_REUSEADDR Undefined!");

    return (XORP_ERROR);
#endif /* ! SO_REUSEADDR */
}

int comm_set_reuseport(int sock, int val)
{
#ifdef SO_REUSEPORT
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s SO_REUSEPORT on socket %d: %s",
		(val)? "set": "reset", sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! SO_REUSEPORT */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_REUSEPORT Undefined!");

    return (XORP_OK);
#endif /* ! SO_REUSEPORT */
}

int comm_set_loopback(int sock, int val)
{
    int family = comm_sock_get_family(sock);

    switch (family) 
    {
	case AF_INET:
	    {
		u_char loop = val;

		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
			    (const void*)(&loop), sizeof(loop)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IP_MULTICAST_LOOP %u: %s",
			    loop,
			    comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#ifdef HAVE_IPV6
	case AF_INET6:
	    {
		unsigned int loop6 = val;

		if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
			    (const void*)(&loop6), sizeof(loop6)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IPV6_MULTICAST_LOOP %u: %s",
			    loop6, comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#endif /* HAVE_IPV6 */
	default:
	    XLOG_FATAL("Error %s setsockopt IP_MULTICAST_LOOP/IPV6_MULTICAST_LOOP "
		    "on socket %d: invalid family = %d",
		    (val)? "set": "reset", sock, family);
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_tcpmd5(int sock, int val)
{
#ifdef TCP_MD5SIG /* XXX: Defined in <netinet/tcp.h> across Free/Net/OpenBSD */
    if (setsockopt(sock, IPPROTO_TCP, TCP_MD5SIG,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s TCP_MD5SIG on socket %d: %s",
		(val)? "set": "reset",  sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! TCP_MD5SIG */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("TCP_MD5SIG Undefined!");

    return (XORP_ERROR);
#endif /* ! TCP_MD5SIG */
}

int comm_set_nopush(int sock, int val)
{
#ifdef TCP_NOPUSH /* XXX: Defined in <netinet/tcp.h> across Free/Net/OpenBSD */
    if (setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH,
		(const void*)(&val), sizeof(val)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error %s TCP_NOPUSH on socket %d: %s",
		(val)? "set": "reset",  sock,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! TCP_NOPUSH */
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("TCP_NOPUSH Undefined!");

    return (XORP_ERROR);
#endif /* ! TCP_NOPUSH */
}

int comm_set_tos(int sock, int val)
{
#ifdef IP_TOS
    /*
     * Most implementations use 'int' to represent the value of
     * the IP_TOS option.
     */
    int family, ip_tos;

    family = comm_sock_get_family(sock);
    if (family != AF_INET) 
    {
	XLOG_FATAL("Error %s setsockopt IP_TOS on socket %d: "
		"invalid family = %d",
		(val)? "set": "reset", sock, family);
	return (XORP_ERROR);
    }

    /*
     * Note that it is not guaranteed that the TOS will be successfully
     * set or indeed propagated where the host platform is running
     * its own traffic classifier; the use of comm_set_tos() is
     * intended for link-scoped traffic.
     */
    ip_tos = val;
    if (setsockopt(sock, IPPROTO_IP, IP_TOS,
		(const void*)(&ip_tos), sizeof(ip_tos)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("setsockopt IP_TOS 0x%x: %s",
		ip_tos, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("IP_TOS Undefined!");

    return (XORP_ERROR);
#endif /* ! IP_TOS */
}

int comm_set_unicast_ttl(int sock, int val)
{
    int family = comm_sock_get_family(sock);

    switch (family) 
    {
	case AF_INET:
	    {
		/* XXX: Most platforms now use int for this option;
		 * legacy BSD specified u_char.
		 */
		int ip_ttl = val;

		if (setsockopt(sock, IPPROTO_IP, IP_TTL,
			    (const void*)(&ip_ttl), sizeof(ip_ttl)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IP_TTL %u: %s",
			    ip_ttl, comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#ifdef HAVE_IPV6
	case AF_INET6:
	    {
		int ip_ttl = val;

		if (setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS,
			    (const void*)(&ip_ttl), sizeof(ip_ttl)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IPV6_UNICAST_HOPS %u: %s",
			    ip_ttl, comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#endif /* HAVE_IPV6 */
	default:
	    XLOG_FATAL("Error %s setsockopt IP_TTL/IPV6_UNICAST_HOPS "
		    "on socket %d: invalid family = %d",
		    (val)? "set": "reset", sock, family);
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_multicast_ttl(int sock, int val)
{
    int family = comm_sock_get_family(sock);

    switch (family) 
    {
	case AF_INET:
	    {
		/* XXX: Most platforms now use int for this option;
		 * legacy BSD specified u_char.
		 */
		u_char ip_multicast_ttl = val;

		if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
			    (const void*)(&ip_multicast_ttl),
			    sizeof(ip_multicast_ttl)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IP_MULTICAST_TTL %u: %s",
			    ip_multicast_ttl,
			    comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#ifdef HAVE_IPV6
	case AF_INET6:
	    {
		int ip_multicast_ttl = val;

		if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
			    (const void*)&ip_multicast_ttl,
			    sizeof(ip_multicast_ttl)) < 0) 
		{
		    _comm_set_serrno();
		    XLOG_ERROR("setsockopt IPV6_MULTICAST_HOPS %u: %s",
			    ip_multicast_ttl,
			    comm_get_error_str(comm_get_last_error()));
		    return (XORP_ERROR);
		}
		break;
	    }
#endif /* HAVE_IPV6 */
	default:
	    XLOG_FATAL("Error %s setsockopt IP_MULTICAST_TTL/IPV6_MULTICAST_HOPS "
		    "on socket %d: invalid family = %d",
		    (val)? "set": "reset", sock, family);
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_iface4(int sock, const struct in_addr *in_addr)
{
    int family = comm_sock_get_family(sock);
    struct in_addr my_addr;

    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    if (in_addr != NULL)
	my_addr.s_addr = in_addr->s_addr;
    else
	my_addr.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
		(const void*)&my_addr, sizeof(my_addr)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("setsockopt IP_MULTICAST_IF %s: %s",
		(in_addr)? inet_ntoa(my_addr) : "ANY",
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_set_iface6(int sock, unsigned int my_ifindex)
{
#ifdef HAVE_IPV6
    int family = comm_sock_get_family(sock);

    if (family != AF_INET6) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		(const void*)&my_ifindex, sizeof(my_ifindex)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("setsockopt IPV6_MULTICAST_IF for interface index %d: %s",
		my_ifindex, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_set_iface6", sock, my_ifindex);
    return (XORP_ERROR);
#endif /* ! HAVE_IPV6 */
}

int comm_set_onesbcast(int sock, int enabled)
{
#ifdef IP_ONESBCAST
    int family = comm_sock_get_family(sock);

    if (family != AF_INET) 
    {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		sock, family, AF_INET);
	return (XORP_ERROR);
    }

    if (setsockopt(sock, IPPROTO_IP, IP_ONESBCAST,
		(const void*)(&enabled), sizeof(enabled)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("setsockopt IP_ONESBCAST %d: %s", enabled,
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);

#else /* ! IP_ONESBCAST */
    XLOG_ERROR("setsockopt IP_ONESBCAST %u: %s", enabled,
	    "IP_ONESBCAST support not present.");
    UNUSED(sock);
    UNUSED(enabled);
    return (XORP_ERROR);
#endif /* ! IP_ONESBCAST */
}

int comm_set_bindtodevice(int sock, const char * my_ifname)
{
#ifdef SO_BINDTODEVICE
    char tmp_ifname[IFNAMSIZ];

    /*
     * Bind a socket to an interface by name.
     *
     * Under Linux, use of the undirected broadcast address
     * 255.255.255.255 requires that the socket is bound to the
     * underlying interface, as it is an implicitly scoped address
     * with no meaning on its own. This is not architecturally OK,
     * and requires additional support for SO_BINDTODEVICE.
     * See socket(7) man page in Linux.
     *
     */
    strncpy(tmp_ifname, my_ifname, IFNAMSIZ-1);
    tmp_ifname[IFNAMSIZ-1] = '\0';
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, tmp_ifname,
		sizeof(tmp_ifname)) < 0) 
    {
	_comm_set_serrno();
	static bool do_once = true;
	// Ignore EPERM..just means user isn't running as root, ie for 'scons check'
	// most likely..User will have bigger trouble if not running as root for
	// a production system anyway.
	if (errno != EPERM) 
	{
	    if (do_once) 
	    {
		XLOG_WARNING("setsockopt SO_BINDTODEVICE %s: %s  This error will only be printed once per program instance.",
			tmp_ifname, comm_get_error_str(comm_get_last_error()));
		do_once = false;
	    }
	}
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
#ifndef __FreeBSD__
    // FreeBSD doesn't implement this, so no use filling logs with errors that can't
    // be helped.  Assume calling code deals with the error code as needed.
    XLOG_ERROR("setsockopt SO_BINDTODEVICE %s: %s",
	    my_ifname, "SO_BINDTODEVICE support not present.");
#endif
    UNUSED(my_ifname);
    UNUSED(sock);
    return (XORP_ERROR);
#endif
}

int comm_sock_set_sndbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;

    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
		(const void*)&desired_bufsize,
		sizeof(desired_bufsize)) < 0) 
    {
	desired_bufsize -= delta;
	while (1) 
	{
	    if (delta > 1)
		delta /= 2;

	    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			(const void*)&desired_bufsize,
			sizeof(desired_bufsize)) < 0) 
	    {
		_comm_set_serrno();
		desired_bufsize -= delta;
		if (desired_bufsize <= 0)
		    break;
	    } else 
	    {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) 
	{
	    XLOG_ERROR("Cannot set sending buffer size of socket %d: "
		    "desired buffer size %u < minimum allowed %u",
		    sock, desired_bufsize, min_bufsize);
	    return (XORP_ERROR);
	}
    }

    return (desired_bufsize);
}

int comm_sock_set_rcvbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;

    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
		(const void*)&desired_bufsize,
		sizeof(desired_bufsize)) < 0) 
    {
	desired_bufsize -= delta;
	while (1) 
	{
	    if (delta > 1)
		delta /= 2;

	    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
			(const void*)&desired_bufsize,
			sizeof(desired_bufsize)) < 0) 
	    {
		_comm_set_serrno();
		desired_bufsize -= delta;
		if (desired_bufsize <= 0)
		    break;
	    } else 
	    {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) 
	{
	    XLOG_ERROR("Cannot set receiving buffer size of socket %d: "
		    "desired buffer size %u < minimum allowed %u",
		    sock, desired_bufsize, min_bufsize);
	    return (XORP_ERROR);
	}
    }

    return (desired_bufsize);
}

int comm_sock_get_family(int sock)
{
    /* XXX: Should use struct sockaddr_storage. */
#ifndef MAXSOCKADDR
#define MAXSOCKADDR	128	/* max socket address structure size */
#endif
    union 
    {
	struct sockaddr	sa;
	char		data[MAXSOCKADDR];
    } un;
    socklen_t len;

    len = MAXSOCKADDR;
    if (getsockname(sock, &un.sa, &len) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("Error getsockname() for socket %d: %s",
		sock, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (un.sa.sa_family);
}

int comm_sock_get_type(int sock)
{
    int err, type;
    socklen_t len = sizeof(type);

    err = getsockopt(sock, SOL_SOCKET, SO_TYPE,
	    (void*)&type, &len);
    if (err != 0)  
    {
	_comm_set_serrno();
	XLOG_ERROR("Error getsockopt(SO_TYPE) for socket %d: %s",
		sock, comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return type;
}

int comm_sock_set_blocking(int sock, int is_blocking)
{
    int flags;
    if ( (flags = fcntl(sock, F_GETFL, 0)) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("F_GETFL error: %s",
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    if (is_blocking)
	flags &= ~O_NONBLOCK;
    else
	flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0) 
    {
	_comm_set_serrno();
	XLOG_ERROR("F_SETFL error: %s",
		comm_get_error_str(comm_get_last_error()));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int comm_sock_is_connected(int sock, int *is_connected)
{
    struct sockaddr_storage ss;
    int err;
    socklen_t sslen;

    if (is_connected == NULL) 
    {
	XLOG_ERROR("comm_sock_is_connected() error: "
		"return value pointer is NULL");
	return (XORP_ERROR);
    }
    *is_connected = 0;

    sslen = sizeof(ss);
    memset(&ss, 0, sslen);
    err = getpeername(sock, (struct sockaddr *)&ss, &sslen);

    if (err != 0) 
    {
	if ((err == ENOTCONN) || (err == ECONNRESET)) 
	{
	    return (XORP_OK);	/* Socket is not connected */
	}
	_comm_set_serrno();
	return (XORP_ERROR);
    }

    /*  Socket is connected */
    *is_connected = 1;

    return (XORP_OK);
}

void comm_sock_no_ipv6(const char* method, ...)
{
    _comm_serrno = EAFNOSUPPORT;
    XLOG_ERROR("%s: IPv6 support not present.", method);
    UNUSED(method);
}

void _comm_set_serrno(void)
{
    _comm_serrno = errno;
    /*
     * TODO: XXX - Temporarily don't set errno to 0 we still have code
     * using errno 2005-05-09 Atanu.
     */
    /* errno = 0; */
}
