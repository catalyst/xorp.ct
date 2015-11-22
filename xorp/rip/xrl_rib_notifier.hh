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

// $XORP: xorp/rip/xrl_rib_notifier.hh,v 1.12 2008/10/02 21:58:19 bms Exp $

#ifndef __RIP_XRL_RIB_NOTIFIER_HH__
#define __RIP_XRL_RIB_NOTIFIER_HH__


#include "libxorp/ipnet.hh"
#include "libxorp/service.hh"

#include "rib_notifier_base.hh"

class XrlSender;
class XrlRouter;

/**
 * @short Class to send RIP updates to RIB process.
 *
 * This class periodically checks the UpdateQueue for updates and
 * sends them to the RIB.  Once an instance has been created, the
 * @ref startup() method should be called to register a routing table for
 * RIP and begin checking for updates.  Before being destructed,
 * @ref shutdown() should be called to remove the RIP routing table from
 * the RIB.
 *
 * The XrlRibNotifier may be in one of several states enumerated by
 * @ref RunStatus.  Before startup(), an instances state is SERVICE_READY.
 * Then when startup is called it transitions into
 * INSTALLING_RIP_TABLE and transitions into SERVICE_RUNNING.  When in
 * SERVICE_RUNNING state updates are sent to the RIB.  When shutdown() is
 * called it enters UNINSTALLING_RIP_TABLE before entering SERVICE_SHUTDOWN.
 * At any time it may fall into state SERVICE_FAILED if communication with
 * the RIB fails.
 */
template <typename A>
class XrlRibNotifier : public RibNotifierBase<A>, public ServiceBase
{
public:
    typedef RibNotifierBase<A> Super;

    static const uint32_t DEFAULT_INFLIGHT = 20;

public:
    /**
     * Constructor.
     */
    XrlRibNotifier( UpdateQueue<A>&	uq,
		   XrlRouter&		xr,
		   uint32_t		max_inflight = DEFAULT_INFLIGHT,
		   uint32_t		poll_ms = Super::DEFAULT_POLL_MS);

    /**
     * Constructor taking an XrlSender, a class name, and an instance
     * name as arguments.  These arguments are broken out for
     * debugging instances, ie a fake XrlSender can be used to test
     * behaviour of this class.
     */
    XrlRibNotifier( UpdateQueue<A>&	uq,
		   XrlSender&		xs,
		   const string&	class_name,
		   const string&	intance_name,
		   uint32_t		max_inflight = DEFAULT_INFLIGHT,
		   uint32_t		poll_ms = Super::DEFAULT_POLL_MS);

    ~XrlRibNotifier();

    /**
     * Request RIB instantiates a RIP routing table and once instantiated
     * start passing route updates to RIB.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Stop forwarding route updates to RIB and request RIB
     * unregisters RIP routing table.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Accessor returning the current number of Xrls inflight.
     */
    uint32_t xrls_inflight() const;

    /**
     * Accessor returning the maximum number of Xrls inflight.
     */
    uint32_t max_xrls_inflight() const;

protected:
    void updates_available();

private:
    void add_igp_cb(const XrlError& e);
    void delete_igp_cb(const XrlError& e);

    void send_add_route(const RouteEntry<A>& re);
    void send_delete_route(const RouteEntry<A>& re);
    void send_route_cb(const XrlError& e);

    void incr_inflight();
    void decr_inflight();

protected:
    XrlSender&		_xs;
    string		_cname;
    string		_iname;
    const uint32_t	_max_inflight;
    uint32_t		_inflight;

    set<IPNet<A> >	_ribnets;	// XXX hack
};

// ----------------------------------------------------------------------------
// Public inline method definitions

template <typename A>
inline uint32_t
XrlRibNotifier<A>::xrls_inflight() const
{
    return _inflight;
}

template <typename A>
inline uint32_t
XrlRibNotifier<A>::max_xrls_inflight() const
{
    return _max_inflight;
}

#endif // __RIP_XRL_RIB_NOTIFIER_HH__
