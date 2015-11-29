// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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


#ifndef __RIP_XRL_TARGET_COMMON_HH__
#define __RIP_XRL_TARGET_COMMON_HH__

#include "libxorp/status_codes.h"

#include "peer.hh"

class XrlProcessSpy;
template <typename A> class XrlPortManager;
template <typename A> class System;
template <typename A> class XrlRedistManager;

/**
 * @short Common handler for Xrl Requests.
 *
 * This class implements Xrl Target code that is common to both RIP
 * and RIP NG.
 */
template <typename A>
class XrlRipCommonTarget 
{
    public:
	XrlRipCommonTarget(XrlProcessSpy& 		xps,
		XrlPortManager<A>&	xpm,
		XrlRedistManager<A>&	xrm,
		System<A>&		rip_system);

	~XrlRipCommonTarget();

	void set_status(ProcessStatus ps, const string& annotation = "");

	XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);

	XrlCmdError common_0_1_shutdown();
	XrlCmdError common_0_1_startup();

	XrlCmdError
	    finder_event_observer_0_1_xrl_target_birth(const string& class_name,
		    const string& instance_name);

	XrlCmdError
	    finder_event_observer_0_1_xrl_target_death(const string& class_name,
		    const string& instance_name);

	XrlCmdError
	    ripx_0_1_add_rip_address(const string&	ifname,
		    const string&	vifname,
		    const A&		addr);

	XrlCmdError
	    ripx_0_1_remove_rip_address(const string&	ifname,
		    const string&	vifname,
		    const A&	addr);

	XrlCmdError
	    ripx_0_1_set_rip_address_enabled(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    const bool&	enabled);

	XrlCmdError
	    ripx_0_1_rip_address_enabled(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    bool&		enabled);

	XrlCmdError ripx_0_1_set_cost(const string&		ifname,
		const string&		vifname,
		const A&		addr,
		const uint32_t&	cost);

	XrlCmdError ripx_0_1_cost(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		uint32_t&		cost);

	XrlCmdError ripx_0_1_set_horizon(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		const string&	horizon);

	XrlCmdError ripx_0_1_horizon(const string&	ifname,
		const string&	vifname,
		const A&	addr,
		string&	horizon);

	XrlCmdError ripx_0_1_set_passive(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		const bool&	passive);

	XrlCmdError ripx_0_1_passive(const string&	ifname,
		const string&	vifname,
		const A&	addr,
		bool&		passive);

	XrlCmdError
	    ripx_0_1_set_accept_non_rip_requests(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const bool&	accept);

	XrlCmdError ripx_0_1_accept_non_rip_requests(const string&	ifname,
		const string&	vifname,
		const A&	addr,
		bool&		accept);

	XrlCmdError ripx_0_1_set_accept_default_route(const string&	ifname,
		const string&	vifname,
		const A&	addr,
		const bool&	accept);

	XrlCmdError ripx_0_1_accept_default_route(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		bool&		accept);

	XrlCmdError
	    ripx_0_1_set_advertise_default_route(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const bool&	advertise);

	XrlCmdError ripx_0_1_advertise_default_route(const string&	ifname,
		const string&	vifname,
		const A&	addr,
		bool&		advertise);

	XrlCmdError
	    ripx_0_1_set_route_timeout(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    const uint32_t&	t_secs);

	XrlCmdError
	    ripx_0_1_route_timeout(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_deletion_delay(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const uint32_t&	t_secs);

	XrlCmdError
	    ripx_0_1_deletion_delay(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_request_interval(const string&		ifname,
		    const string&		vifname,
		    const A&		addr,
		    const uint32_t&	t_secs);

	XrlCmdError
	    ripx_0_1_request_interval(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_update_interval(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const uint32_t& t_secs);

	XrlCmdError
	    ripx_0_1_update_interval(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_update_jitter(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const uint32_t&	t_jitter);

	XrlCmdError
	    ripx_0_1_update_jitter(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_triggered_update_delay(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const uint32_t&	t_secs);

	XrlCmdError
	    ripx_0_1_triggered_update_delay(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_triggered_update_jitter(const string&	ifname,
		    const string&	vifname,
		    const A&	addr,
		    const uint32_t& t_secs);

	XrlCmdError
	    ripx_0_1_triggered_update_jitter(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&		t_secs);

	XrlCmdError
	    ripx_0_1_set_interpacket_delay(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    const uint32_t&	t_msecs);

	XrlCmdError
	    ripx_0_1_interpacket_delay(const string&	ifname,
		    const string&	vifname,
		    const A&		addr,
		    uint32_t&	t_msecs);

	XrlCmdError ripx_0_1_rip_address_status(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		string&		status);

	XrlCmdError ripx_0_1_get_all_addresses(XrlAtomList&	ifnames,
		XrlAtomList&	vifnames,
		XrlAtomList&	addrs);

	XrlCmdError ripx_0_1_get_peers(const string& ifname,
		const string& vifname,
		const A&	 addr,
		XrlAtomList&	 peers);

	XrlCmdError ripx_0_1_get_all_peers(XrlAtomList& peers,
		XrlAtomList& ifnames,
		XrlAtomList& vifnames,
		XrlAtomList& addrs);

	XrlCmdError ripx_0_1_get_counters(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		XrlAtomList&	descriptions,
		XrlAtomList&	values);

	XrlCmdError ripx_0_1_get_peer_counters(const string&	ifname,
		const string&	vifname,
		const A&		addr,
		const A&		peer,
		XrlAtomList&		descriptions,
		XrlAtomList&		values,
		uint32_t&		peer_last_pkt);

	XrlCmdError trace(bool enable);

	XrlCmdError socketx_user_0_1_recv_event(const string&	sockid,
		const string&	if_name,
		const string&	vif_name,
		const A&		src_host,
		const uint32_t&	src_port,
		const vector<uint8_t>& pdata);

	XrlCmdError socketx_user_0_1_inbound_connect_event(
		const string&	sockid,
		const A&	src_host,
		const uint32_t&	src_port,
		const string&	new_sockid,
		bool&		accept);

	XrlCmdError socketx_user_0_1_outgoing_connect_event(
		const string&	sockid);

	XrlCmdError socketx_user_0_1_error_event(const string&	sockid,
		const string& 	reason,
		const bool&	fatal);

	XrlCmdError socketx_user_0_1_disconnect_event(const string&	sockid);

	XrlCmdError policy_backend_0_1_configure(const uint32_t& filter,
		const string& conf);

	XrlCmdError policy_backend_0_1_reset(const uint32_t& filter);

	XrlCmdError policy_backend_0_1_push_routes();


	XrlCmdError policy_redistx_0_1_add_routex(const IPNet<A>&	    net,
		const bool&	    unicast,
		const bool&	    multicast,
		const A&		    nexthop,
		const uint32_t&	    metric,
		const XrlAtomList&    policytags);

	XrlCmdError policy_redistx_0_1_delete_routex(const IPNet<A>&    net,
		const bool&	    unicast,
		const bool&	    multicast);

	/**
	 * Find Port associated with ifname, vifname, addr.
	 *
	 * @return on success the first item in the pair will be a
	 * non-null pointer to the port and the second item with be
	 * XrlCmdError::OKAY().  On failyre the first item in the pair
	 * will be null and the XrlCmdError will signify the reason for
	 * the failure.
	 */
	pair<Port<A>*,XrlCmdError> find_port(const string&	ifname,
		const string&	vifname,
		const A&	addr);

    protected:
	XrlProcessSpy&		_xps;
	XrlPortManager<A>&		_xpm;
	XrlRedistManager<A>&	_xrm;

	ProcessStatus		_status;
	string			_status_note;

	System<A>&			_rip_system;
};


// ----------------------------------------------------------------------------
// Inline implementation of XrlRipCommonTarget

    template <typename A>
XrlRipCommonTarget<A>::XrlRipCommonTarget(XrlProcessSpy&	xps,
	XrlPortManager<A>& 	xpm,
	XrlRedistManager<A>&	xrm,
	System<A>&		rip_system)
    : _xps(xps), _xpm(xpm), _xrm(xrm),
    _status(PROC_NULL), _status_note(""),
    _rip_system(rip_system)
{
}

    template <typename A>
XrlRipCommonTarget<A>::~XrlRipCommonTarget()
{
}

template <typename A>
    void
XrlRipCommonTarget<A>::set_status(ProcessStatus status, const string& note)
{
    debug_msg("Status Update %d -> %d: %s\n", _status, status, note.c_str());
    _status 	 = status;
    _status_note = note;
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::common_0_1_get_status(uint32_t& status,
	string&   reason)
{
    status = _status;
    reason = _status_note;
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::common_0_1_shutdown()
{
    debug_msg("Shutdown requested.\n");
    xorp_do_run = 0;
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::common_0_1_startup()
{
    debug_msg("Startup requested.\n");
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::finder_event_observer_0_1_xrl_target_birth(
	const string& cname,
	const string& iname
	)
{
    _xps.birth_event(cname, iname);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::finder_event_observer_0_1_xrl_target_death(
	const string& cname,
	const string& iname
	)
{
    _xps.death_event(cname, iname);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_add_rip_address(const string&	ifname,
	const string&	vifname,
	const A&		addr)
{
    debug_msg("rip_x_1_add_rip_address %s/%s/%s\n",
	    ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.add_rip_address(ifname, vifname, addr) == false) 
    {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_remove_rip_address(const string& ifname,
	const string&		 vifname,
	const A&   		 addr)
{
    debug_msg("ripx_0_1_remove_rip_address %s/%s/%s\n",
	    ifname.c_str(), vifname.c_str(), addr.str().c_str());
    if (_xpm.remove_rip_address(ifname, vifname, addr) == false) 
    {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// Utility methods

template <typename A>
    pair<Port<A>*, XrlCmdError>
XrlRipCommonTarget<A>::find_port(const string&	ifn,
	const string&	vifn,
	const A&	addr)
{
    Port<A>* p = _xpm.find_port(ifn, vifn, addr);
    if (p == 0) 
    {
	string e = c_format("RIP not running on %s/%s/%s",
		ifn.c_str(), vifn.c_str(), addr.str().c_str());
	return make_pair(p, XrlCmdError::COMMAND_FAILED(e));
    }
    return make_pair(p, XrlCmdError::OKAY());
}


// ----------------------------------------------------------------------------
// Port configuration accessors and modifiers

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_rip_address_enabled(const string&	ifn,
	const string&	vifn,
	const A&	addr,
	const bool&	en)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_enabled(en);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_rip_address_enabled(const string& ifn,
	const string& vifn,
	const A&	  addr,
	bool&	  en)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    en = p->enabled();

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_cost(const string&	ifname,
	const string&		vifname,
	const A&			addr,
	const uint32_t&		cost)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;

    if (cost > RIP_INFINITY) 
    {
	string e = c_format("Cost must be less that RIP infinity (%u)",
		XORP_UINT_CAST(RIP_INFINITY));
	return XrlCmdError::COMMAND_FAILED(e);
    }

    p->set_cost(cost);

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_cost(const string&	ifname,
	const string&	vifname,
	const A&		addr,
	uint32_t&		cost)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    cost = p->cost();

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_horizon(const string&	ifname,
	const string&	vifname,
	const A&		addr,
	const string&	horizon)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;

    RipHorizon rh[3] = { NONE, SPLIT, SPLIT_POISON_REVERSE };
    for (uint32_t i = 0; i < 3; ++i) 
    {
	if (string(rip_horizon_name(rh[i])) == horizon) 
	{
	    p->set_horizon(rh[i]);
	    return XrlCmdError::OKAY();
	}
    }
    return XrlCmdError::COMMAND_FAILED(
	    c_format("Horizon type \"%s\" not recognized.",
		horizon.c_str()));
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_horizon(const string&	ifname,
	const string&	vifname,
	const A&	addr,
	string&		horizon)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    horizon = rip_horizon_name(p->horizon());
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_passive(const string&	ifname,
	const string&	vifname,
	const A&		addr,
	const bool&		passive)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_passive(passive);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_passive(const string&	ifname,
	const string&	vifname,
	const A&	addr,
	bool&		passive)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    passive = p->passive();
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_accept_non_rip_requests(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const bool&	accept
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_accept_non_rip_requests(accept);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_accept_non_rip_requests(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	bool&		accept
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    accept = p->accept_non_rip_requests();
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_accept_default_route(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const bool&	accept
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_accept_default_route(accept);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_accept_default_route(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	bool&		accept
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    accept = p->accept_default_route();
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_advertise_default_route(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const bool&	advertise
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    p->set_advertise_default_route(advertise);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_advertise_default_route(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	bool&		advertise
	)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    advertise = p->advertise_default_route();
    return XrlCmdError::OKAY();
}


// ----------------------------------------------------------------------------
// The following pair of macros are used in setting timer constants on
// RIP ports.

#define PORT_TIMER_SET_HANDLER(field, t, min_val, max_val, method)	\
    do {									\
	pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);	\
	if (pp.first == 0)							\
	return pp.second;						\
	Port<A>* p = pp.first;						\
	\
	if (static_cast<int32_t>(t) < min_val) 				\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
		    "value supplied less than permitted minimum (%u < %u)", 	\
		    XORP_UINT_CAST(t), XORP_UINT_CAST(min_val)));		\
	if (t > max_val) 							\
	return XrlCmdError::COMMAND_FAILED(c_format(			\
		    "value supplied greater than permitted maximum (%u > %u)", 	\
		    XORP_UINT_CAST(t), XORP_UINT_CAST(max_val)));		\
	if (p->constants().set_##field (t) == false)			\
	return XrlCmdError::COMMAND_FAILED(				\
		"Failed to set value.");					\
	p->reschedule_##method ();						\
	return XrlCmdError::OKAY();						\
    } while (0)

#define PORT_TIMER_GET_HANDLER(field, t)				\
    do {									\
	pair<Port<A>*, XrlCmdError> pp = find_port(ifname, vifname, addr);	\
	if (pp.first == 0)							\
	return pp.second;						\
	Port<A>* p = pp.first;						\
	t = p->constants(). field ();					\
	return XrlCmdError::OKAY();						\
    } while (0)


template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_route_timeout(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t
	)
{
    PORT_TIMER_SET_HANDLER(expiry_secs, t, 10, 10000, dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_route_timeout(
	const string&	ifname,
	const string&	vifname,
	const A&		addr,
	uint32_t&		t
	)
{
    PORT_TIMER_GET_HANDLER(expiry_secs, t);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_deletion_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t
	)
{
    PORT_TIMER_SET_HANDLER(deletion_secs, t, 10, 10000, dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_deletion_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t
	)
{
    PORT_TIMER_GET_HANDLER(deletion_secs, t);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_request_interval(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t
	)
{
    PORT_TIMER_SET_HANDLER(table_request_period_secs, t, 0, 10000,
	    request_table_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_request_interval(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t
	)
{
    PORT_TIMER_GET_HANDLER(table_request_period_secs, t);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_update_interval(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t& t_secs
	)
{
    PORT_TIMER_SET_HANDLER(update_interval, t_secs, 1, 600, dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_update_interval(const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t_secs)
{
    PORT_TIMER_GET_HANDLER(update_interval, t_secs);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_update_jitter(const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t& t_jitter)
{
    PORT_TIMER_SET_HANDLER(update_jitter, t_jitter, 0, 100, dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_update_jitter(const string&	ifname,
	const string&	vifname,
	const A&		addr,
	uint32_t&		t_jitter)
{
    PORT_TIMER_GET_HANDLER(update_jitter, t_jitter);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_triggered_update_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t_secs)
{
    PORT_TIMER_SET_HANDLER(triggered_update_delay, t_secs, 1, 10000,
	    dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_triggered_update_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t_secs)
{
    PORT_TIMER_GET_HANDLER(triggered_update_delay, t_secs);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_triggered_update_jitter(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t_jitter)
{
    PORT_TIMER_SET_HANDLER(triggered_update_jitter, t_jitter, 0, 100,
	    dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_triggered_update_jitter(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t_jitter)
{
    PORT_TIMER_GET_HANDLER(triggered_update_jitter, t_jitter);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_set_interpacket_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	const uint32_t&	t_msecs
	)
{
    PORT_TIMER_SET_HANDLER(interpacket_delay_ms, t_msecs, 10, 10000,
	    dummy_timer);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_interpacket_delay(
	const string&	ifname,
	const string&	vifname,
	const A&	addr,
	uint32_t&	t_msecs
	)
{
    PORT_TIMER_GET_HANDLER(interpacket_delay_ms, t_msecs);
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_rip_address_status(const string&	ifn,
	const string&		vifn,
	const A&		addr,
	string&		status)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    status = p->enabled() ? "enabled" : "disabled";

    if (p->enabled()
	    && _xpm.underlying_rip_address_up(ifn, vifn, addr) == false) 
    {
	if (_xpm.underlying_rip_address_exists(ifn, vifn, addr)) 
	{
	    status += " [gated by disabled FEA interface/vif/address]";
	} else 
	{
	    status += " [gated by non-existant FEA interface/vif/address]";
	}
    }

    debug_msg("ripx_0_1_rip_address_status %s/%s/%s -> %s\n",
	    ifn.c_str(), vifn.c_str(), addr.str().c_str(),
	    status.c_str());

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_all_addresses(XrlAtomList&	ifnames,
	XrlAtomList&	vifnames,
	XrlAtomList&	addrs)
{
    const typename PortManagerBase<A>::PortList & ports = _xpm.const_ports();
    typename PortManagerBase<A>::PortList::const_iterator pci;

    for (pci = ports.begin(); pci != ports.end(); ++pci) 
    {
	const Port<A>*	     	port = *pci;
	const PortIOBase<A>* 	pio  = port->io_handler();
	if (pio == 0) 
	{
	    continue;
	}
	ifnames.append  ( XrlAtom(pio->ifname())   );
	vifnames.append ( XrlAtom(pio->vifname())  );
	addrs.append    ( XrlAtom(pio->address())  );
    }
    return XrlCmdError::OKAY();
}


template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_peers(const string&	ifn,
	const string&	vifn,
	const A&	addr,
	XrlAtomList&	peers)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    Port<A>* p = pp.first;
    typename Port<A>::PeerList::const_iterator pi = p->peers().begin();
    while (pi != p->peers().end()) 
    {
	const Peer<A>* peer = *pi;
	peers.append(XrlAtom(peer->address()));
	++pi;
    }
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_all_peers(XrlAtomList&	peers,
	XrlAtomList&	ifnames,
	XrlAtomList&	vifnames,
	XrlAtomList&	addrs)
{
    const typename PortManagerBase<A>::PortList & ports = _xpm.const_ports();
    typename PortManagerBase<A>::PortList::const_iterator pci;

    for (pci = ports.begin(); pci != ports.end(); ++pci) 
    {
	const Port<A>* port = *pci;
	const PortIOBase<A>* pio = port->io_handler();
	if (pio == 0) 
	{
	    continue;
	}

	typename Port<A>::PeerList::const_iterator pi;
	for (pi = port->peers().begin(); pi != port->peers().end(); ++pi) 
	{
	    const Peer<A>* peer = *pi;
	    peers.append    ( XrlAtom(peer->address()) );
	    ifnames.append  ( XrlAtom(pio->ifname())   );
	    vifnames.append ( XrlAtom(pio->vifname())  );
	    addrs.append    ( XrlAtom(pio->address())  );
	}
    }
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_counters(const string&	ifn,
	const string&	vifn,
	const A&		addr,
	XrlAtomList&	descriptions,
	XrlAtomList&	values)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    const Port<A>* p = pp.first;
    descriptions.append(XrlAtom("", string("Requests Sent")));
    values.append(XrlAtom(p->counters().table_requests_sent()));

    descriptions.append(XrlAtom("", string("Updates Sent")));
    values.append(XrlAtom(p->counters().unsolicited_updates()));

    descriptions.append(XrlAtom("", string("Triggered Updates Sent")));
    values.append(XrlAtom(p->counters().triggered_updates()));

    descriptions.append(XrlAtom("", string("Non-RIP Updates Sent")));
    values.append(XrlAtom(p->counters().non_rip_updates_sent()));

    descriptions.append(XrlAtom("", string("Total Packets Received")));
    values.append(XrlAtom(p->counters().packets_recv()));

    descriptions.append(XrlAtom("", string("Request Packets Received")));
    values.append(XrlAtom(p->counters().table_requests_recv()));

    descriptions.append(XrlAtom("", string("Update Packets Received")));
    values.append(XrlAtom(p->counters().update_packets_recv()));

    descriptions.append(XrlAtom("", string("Bad Packets Received")));
    values.append(XrlAtom(p->counters().bad_packets()));

    if (A::ip_version() == 4) 
    {
	descriptions.append(XrlAtom("", string("Authentication Failures")));
	values.append(XrlAtom(p->counters().bad_auth_packets()));
    }

    descriptions.append(XrlAtom("", string("Bad Routes Received")));
    values.append(XrlAtom(p->counters().bad_routes()));

    descriptions.append(XrlAtom("", string("Non-RIP Requests Received")));
    values.append(XrlAtom(p->counters().non_rip_requests_recv()));

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::ripx_0_1_get_peer_counters(
	const string&	ifn,
	const string&	vifn,
	const A&	addr,
	const A&	peer_addr,
	XrlAtomList&	descriptions,
	XrlAtomList&	values,
	uint32_t&	peer_last_active)
{
    pair<Port<A>*, XrlCmdError> pp = find_port(ifn, vifn, addr);
    if (pp.first == 0)
	return pp.second;

    const Port<A>* port = pp.first;

    typename Port<A>::PeerList::const_iterator pi;
    pi = find_if(port->peers().begin(), port->peers().end(),
	    peer_has_address<A>(peer_addr));

    if (pi == port->peers().end()) 
    {
	return XrlCmdError::COMMAND_FAILED(
		c_format("Peer %s not found on %s/%s/%s",
		    peer_addr.str().c_str(), ifn.c_str(), vifn.c_str(),
		    addr.str().c_str())
		);
    }

    const Peer<A>* const peer = *pi;

    descriptions.append(  XrlAtom("", string("Total Packets Received")) );
    values.append( XrlAtom(peer->counters().packets_recv()) );

    descriptions.append(XrlAtom("", string("Request Packets Received")));
    values.append(XrlAtom(peer->counters().table_requests_recv()));

    descriptions.append(XrlAtom("", string("Update Packets Received")));
    values.append(XrlAtom(peer->counters().update_packets_recv()));

    descriptions.append(XrlAtom("", string("Bad Packets Received")));
    values.append(XrlAtom(peer->counters().bad_packets()));

    if (A::ip_version() == 4) 
    {
	descriptions.append(XrlAtom("", string("Authentication Failures")));
	values.append(XrlAtom(peer->counters().bad_auth_packets()));
    }

    descriptions.append(XrlAtom("", string("Bad Routes Received")));
    values.append(XrlAtom(peer->counters().bad_routes()));

    descriptions.append(XrlAtom("", string("Routes Active")));
    values.append(XrlAtom(peer->route_count()));

    peer_last_active = peer->last_active().sec();

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::trace(bool enable)
{
    _rip_system.route_trace().all(enable);
    _xpm.packet_trace().all(enable);

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_recv_event(
	const string&		sockid,
	const string&		if_name,
	const string&		vif_name,
	const A&		src_host,
	const uint32_t&		src_port,
	const vector<uint8_t>&	pdata
	)
{
    _xpm.deliver_packet(sockid, if_name, vif_name, src_host, src_port, pdata);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_inbound_connect_event(
	const string&	sockid,
	const A&	src_host,
	const uint32_t&	src_port,
	const string&	new_sockid,
	bool&		accept
	)
{
    debug_msg("socketx_user_0_1_inbound_connect_event %s %s/%u %s\n",
	    sockid.c_str(), src_host.str().c_str(),
	    XORP_UINT_CAST(src_port), new_sockid.c_str());

    UNUSED(sockid);
    UNUSED(src_host);
    UNUSED(src_port);
    UNUSED(new_sockid);

    accept = false;
    return XrlCmdError::COMMAND_FAILED("Inbound connect not requested.");
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_outgoing_connect_event(
	const string&	sockid
	)
{
    debug_msg("socketx_user_0_1_outgoing_connect_event %s\n",
	    sockid.c_str());

    UNUSED(sockid);
    return XrlCmdError::COMMAND_FAILED("Outgoing connect not requested.");
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_error_event(
	const string&	sockid,
	const string& 	reason,
	const bool&	fatal
	)
{
    debug_msg("socketx_user_0_1_error_event %s %s %s \n",
	    sockid.c_str(), reason.c_str(),
	    fatal ? "fatal" : "non-fatal");

    UNUSED(sockid);
    UNUSED(reason);
    UNUSED(fatal);

    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::socketx_user_0_1_disconnect_event(
	const string&	sockid
	)
{
    debug_msg("socketx_user_0_1_disconnect_event %s\n",
	    sockid.c_str());

    UNUSED(sockid);

    return XrlCmdError::OKAY();
}


template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::policy_backend_0_1_configure(const uint32_t& filter,
	const string& conf)
{
    try 
    {
	_rip_system.configure_filter(filter, conf);
    } catch(const PolicyException& e) 
    {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
		e.str());
    }
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::policy_backend_0_1_reset(const uint32_t& filter)
{
    try 
    {
	_rip_system.reset_filter(filter);
    } catch(const PolicyException& e) 
    {
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " + 
		e.str());
    }
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError
XrlRipCommonTarget<A>::policy_backend_0_1_push_routes()
{
    _rip_system.push_routes();
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError 
XrlRipCommonTarget<A>::policy_redistx_0_1_add_routex(const IPNet<A>&	net,
	const bool&	unicast,
	const bool&	multicast,
	const A&	        nexthop,
	const uint32_t&	metric,
	const XrlAtomList& policytags)
{
    string ifname, vifname;

    UNUSED(multicast);

    if (! unicast)
	return XrlCmdError::OKAY();

    //
    // XXX: The interface and vif name are empty, because the policy
    // mechanism doesn't support setting them (yet).
    //
    _xrm.add_route(net, nexthop, ifname, vifname, metric, 0, policytags);
    return XrlCmdError::OKAY();
}

template <typename A>
    XrlCmdError 
XrlRipCommonTarget<A>::policy_redistx_0_1_delete_routex(const IPNet<A>&	net,
	const bool&	unicast,
	const bool&	multicast)
{
    UNUSED(multicast);

    if (! unicast)
	return XrlCmdError::OKAY();

    _xrm.delete_route(net);
    return XrlCmdError::OKAY();
}
#endif // __RIPX_XRL_TARGET_COMMON_HH__
