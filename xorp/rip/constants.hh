// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rip/constants.hh,v 1.23 2008/10/02 21:58:16 bms Exp $

#ifndef __RIP_CONSTANTS_HH__
#define __RIP_CONSTANTS_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

static const uint32_t DEFAULT_UPDATE_INTERVAL = 30;
static const uint32_t DEFAULT_UPDATE_JITTER = 16;

static const uint32_t DEFAULT_EXPIRY_SECS = 180;
static const uint32_t DEFAULT_DELETION_SECS = 120;

static const uint32_t DEFAULT_TRIGGERED_UPDATE_DELAY = 3;
static const uint32_t DEFAULT_TRIGGERED_UPDATE_JITTER = 66;

static const uint32_t DEFAULT_TABLE_REQUEST_SECS = 1;

static const uint32_t RIPv2_ROUTES_PER_PACKET = 25;

/**
 * The default delay between back to back RIP packets when an update
 * is sent that spans more than 1 packet or there are multiple packets
 * to be sent.
 */
static const uint32_t DEFAULT_INTERPACKET_DELAY_MS = 50;

/**
 * The maximum delay between back to back RIP packets when an update
 * is sent that spans more than 1 packet.
 */
static const uint32_t MAXIMUM_INTERPACKET_DELAY_MS = 250;

/**
 * The default delay between accepting route request packets that
 * query specific routes.
 */
static const uint32_t DEFAULT_INTERQUERY_GAP_MS = 250;

/**
 * Protocol specified metric corresponding to an unreachable (or expired)
 * host or network.
 */
static const uint32_t RIP_INFINITY = 16;

/**
 * The maximum cost of a routing entry.
 * Note that it must be larger than the protocol defined RIP_INFINITY.
 */
static const uint32_t RIP_MAX_COST = 0xffff;

/**
 * Time-To-Live value that should be used for multicast packets.
 */
static const uint32_t RIP_TTL = 1;

/**
 * Hop-Count value that should be used for multicast RIPng packets.
 */
static const uint32_t RIP_NG_HOP_COUNT = 255;

/**
 * RIP IPv4 protocol port
 */
static const uint16_t RIP_PORT = 520;

/**
 * RIPng protocol port
 */
static const uint16_t RIP_NG_PORT = 521;

static const IPv4Net IPv4_DEFAULT_ROUTE = IPv4Net(IPv4::ZERO(), 0);
static const IPv6Net IPv6_DEFAULT_ROUTE = IPv6Net(IPv6::ZERO(), 0);

/**
 * Basis of specialized classes containing RIP constants.
 */
template <typename A>
struct RIP_AF_CONSTANTS;

template <>
struct RIP_AF_CONSTANTS<IPv4>
{
	static const IPv4		IP_GROUP() { return IPv4::RIP2_ROUTERS(); }
	static const uint16_t	IP_PORT = RIP_PORT;
	static const IPv4Net&	DEFAULT_ROUTE() { return IPv4_DEFAULT_ROUTE; }
	static const uint8_t	PACKET_VERSION = 2;
};

template <>
struct RIP_AF_CONSTANTS<IPv6>
{
	static const IPv6&		IP_GROUP() { return IPv6::RIP2_ROUTERS(); }
	static const uint16_t	IP_PORT = RIP_NG_PORT;
	static const IPv6Net&	DEFAULT_ROUTE() { return IPv6_DEFAULT_ROUTE; }
	static const uint8_t	 PACKET_VERSION = 1;
};

/**
 * Enumeration of RIP Horizon types.
 */
enum RipHorizon 
{
	// No filtering
	NONE		 = 0,
	// Don't a route origin its own routes.
	SPLIT		 = 1,
	// Show a route origin its own routes but with cost of infinity.
	SPLIT_POISON_REVERSE = 2
};

	inline static const char*
rip_horizon_name(RipHorizon h)
{
	switch (h) 
	{
		case SPLIT_POISON_REVERSE:
			return "split-horizon-poison-reverse";
		case SPLIT:
			return "split-horizon";
		case NONE:
			break;
	}
	return "none";
};

#endif // __RIP_CONSTANTS_HH__
