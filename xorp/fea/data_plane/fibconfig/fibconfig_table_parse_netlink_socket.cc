// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#include <xorp_config.h>
#ifdef HAVE_NETLINK_SOCKETS

#include "fea/fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fea/fibconfig.hh"
#include "fea/data_plane/control_socket/netlink_socket_utilities.hh"

#include "fibconfig_table_get_netlink_socket.hh"


//
// Parse information about routing table information received from
// the underlying system.
//
// The information to parse is in NETLINK format
// (e.g., obtained by netlink(7) sockets mechanism).
//
// Reading netlink(3) manual page is a good start for understanding this
//

	int
FibConfigTableGetNetlinkSocket::parse_buffer_netlink_socket(
		int family,
		const IfTree& iftree,
		list<FteX>& fte_list,
		vector<uint8_t>& buffer,
		bool is_nlm_get_only, const FibConfig& fibconfig)
{
	size_t buffer_bytes = buffer.size();
	struct nlmsghdr* nlh;

	for (nlh = (struct nlmsghdr*)(&buffer[0]);
			NLMSG_OK(nlh, buffer_bytes);
			nlh = NLMSG_NEXT(nlh, buffer_bytes)) 
	{
		void* nlmsg_data = NLMSG_DATA(nlh);

		//XLOG_INFO("nlh->msg-type: %s (%i)",
		//	  NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
		//	  (int)(nlh->nlmsg_type));

		switch (nlh->nlmsg_type) 
		{
			case NLMSG_ERROR:
				{
					const struct nlmsgerr* err;

					err = reinterpret_cast<const struct nlmsgerr*>(nlmsg_data);
					if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(*err))) 
					{
						XLOG_ERROR("AF_NETLINK nlmsgerr length error");
						break;
					}
					errno = -err->error;
					XLOG_ERROR("AF_NETLINK NLMSG_ERROR message: %s", strerror(errno));
				}
				break;

			case NLMSG_DONE:
				{
					return (XORP_OK);	// XXX: OK even if there were no entries
				}
				break;

			case NLMSG_NOOP:
				break;

			case RTM_NEWROUTE:
			case RTM_DELROUTE:
			case RTM_GETROUTE:
				{
					if (is_nlm_get_only) 
					{
						//
						// Consider only the "get" entries returned by RTM_GETROUTE.
						// XXX: RTM_NEWROUTE below instead of RTM_GETROUTE is not
						// a mistake, but an artifact of Linux logistics.
						//
						if (nlh->nlmsg_type != RTM_NEWROUTE)
							break;
					}

					struct rtmsg* rtmsg;
					int rta_len = RTM_PAYLOAD(nlh);

					if (rta_len < 0) 
					{
						XLOG_ERROR("AF_NETLINK rtmsg length error");
						break;
					}
					rtmsg = (struct rtmsg*)(nlmsg_data);
					if (rtmsg->rtm_family != family)
						break;
					if (rtmsg->rtm_flags & RTM_F_CLONED)
						break;		// XXX: ignore cloned entries
					if (rtmsg->rtm_type == RTN_MULTICAST)
						break;		// XXX: ignore multicast entries
					if (rtmsg->rtm_type == RTN_BROADCAST)
						break;		// XXX: ignore broadcast entries

					FteX fte(family);
					string err_msg;
					if (NlmUtils::nlm_get_to_fte_cfg(iftree, fte, nlh, rtmsg, rta_len, fibconfig, err_msg)
							== XORP_OK) 
					{
						fte_list.push_back(fte);
					}
					else 
					{
						//XLOG_INFO("nlm_get_to_fte_cfg had error: %s", err_msg.c_str());
					}
				}
				break;

			default:
				debug_msg("Unhandled type %s(%d) (%d bytes)\n",
						NlmUtils::nlm_msg_type(nlh->nlmsg_type).c_str(),
						nlh->nlmsg_type, nlh->nlmsg_len);
		}
	}

	return (XORP_OK);		// XXX: OK even if there were no entries
}

#endif // HAVE_NETLINK_SOCKETS
