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

// $XORP: xorp/rib/register_server.hh,v 1.17 2008/10/02 21:58:11 bms Exp $

#ifndef __RIB_REGISTER_SERVER_HH__
#define __RIB_REGISTER_SERVER_HH__




#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "xrl/interfaces/rib_client_xif.hh"


class XrlRouter;
class NotifyQueueEntry;

typedef XrlRibClientV0p1Client ResponseSender;

/**
 * @short Queue of route event notifications
 *
 * A NotfiyQueue holds a queue of route event notifications waiting
 * transmission to the XORP process that registered interest in route
 * changes that affected one or more routes.  When a lot of routes
 * change, we need to queue the changes because we may generate them
 * faster than the recipient can handle being told about them.
 */
class NotifyQueue 
{
	public:
		/**
		 * NotifyQueue constructor
		 *
		 * @param module_name the XRL module target name for the process that
		 * this queue holds notifications for.  
		 */
		NotifyQueue(const string& module_name);

		/**
		 * Add an notification entry to the queue.
		 *
		 * @param e the notification entry to be queued.
		 */
		void add_entry(NotifyQueueEntry* e);

		/**
		 * Send the next entry in the queue to this queue's XRL target.
		 */
		void send_next();

		/**
		 * Flush is an indication to the queue that the changes since the
		 * last flush can be checked for consolidation.  Several add_entry
		 * events might occur in rapid succession affecting the same
		 * route.  A flush indicates that it is OK to start sending this
		 * batch of changes (and to consolidate those changes to avoid
		 * thrashing, but we don't currently do this).
		 */
		void flush(ResponseSender* response_sender);

		/**
		 * XRL transmit complete callback.  We use this to cause the next
		 * item in the queue to be sent.  
		 *
		 * @param e the XRL completion status code. 
		 */
		void xrl_done(const XrlError& e);

		typedef XorpCallback1<void, const XrlError&>::RefPtr XrlCompleteCB;

	private:
		string		_module_name;
		list<NotifyQueueEntry* > _queue;
		bool		_active;
		ResponseSender*	_response_sender;
};

/**
 * Base class for a queue entry to be held in a @ref NotifyQueue.
 */
class NotifyQueueEntry 
{
	public:
		/**
		 * The type of notifcation: either the data (nexthop, metric,
		 * admin_distance) associated with the registered route changed,
		 * or the data has changed enough that the registration has been
		 * invalidated and the client needs to register again to find out
		 * what happened.
		 */
		typedef enum { CHANGED, INVALIDATE } EntryType;

		/**
		 * NotifyQueueEntry constructor
		 */
		NotifyQueueEntry() {}

		/**
		 * NotifyQueueEntry destructor
		 */
		virtual ~NotifyQueueEntry() {};

		/** 
		 * Send the queue entry (pure virtual)
		 */
		virtual void send(ResponseSender* response_sender,
				const string& module_name,
				NotifyQueue::XrlCompleteCB& cb) = 0;

		/**
		 * @return The type of queue entry
		 * @see NotifyQueueEntry::EntryType
		 */
		virtual EntryType type() const = 0;

	private:
};

/**
 * Notification Queue entry indicating that a change occured to the
 * metric, admin_distance or nexthop of a route in which interest was
 * registered.
 *
 * The template class A is the address family: either the IPv4 class
 * or the IPv6 class.
 */
template <class A>
				  class NotifyQueueChangedEntry : public NotifyQueueEntry 
{
	public:
		/**
		 * NotifyQueueChangedEntry Constructor
		 *
		 * @param net the destination subnet of the route that changed.
		 * @param nexthop the new nexthop of the route that changed.
		 * @param metric the new routing protocol metric of the route that changed.
		 * @param admin_distance the adminstratively defined distance of the
		 * routing protocol this routing entry was computed by.
		 * @param protocol_origin the name of the protocol that originated this
		 * route.
		 * @param multicast true indicates that the change occured in the
		 * multicast RIB, false indicates that it occured in the unicast
		 * RIB.
		 */
		NotifyQueueChangedEntry(const IPNet<A>& net, const A& nexthop,
				uint32_t metric, uint32_t admin_distance,
				const string& protocol_origin, bool multicast)
			: _net(net), _nexthop(nexthop), _metric(metric),
			_admin_distance(admin_distance), _protocol_origin(protocol_origin),
			_multicast(multicast) {}

		/**
		 * @return CHANGED
		 * @see NotifyQueueEntry::EntryType
		 */
		EntryType type() const { return CHANGED; }

		/**
		 * Actually send the XRL that communicates this change to the
		 * registered process.
		 *
		 * @param response_sender the auto-generated stub class instance
		 * that will do the parameter marchalling.
		 * @param module_name the XRL module target name to send this
		 * information to.
		 * @param cb the method to call back when this XRL completes.
		 */
		void send(ResponseSender* response_sender,
				const string& module_name,
				NotifyQueue::XrlCompleteCB& cb);

	private:
		IPNet<A>	_net;	// The route's full subnet (not the valid_subnet)
		A		_nexthop;	// The new nexthop of the route
		uint32_t	_metric;	// The metric of the route
		uint32_t	_admin_distance; // The administratively defined distance of
		// the routing protocol this routing entry
		// was computed by
		string	_protocol_origin; // The name of the protocol that originated
		// this route
		bool	_multicast;	// If true, a change occured in multicast RIB,
		// otherwise it occured in the unicast RIB
};

/**
 * Notification Queue entry indicating that a change occured which has
 * caused a route registration to become invalid.  The client must
 * re-register to find out what actually happened.
 *
 * The template class A is the address family: either the IPv4 class
 * or the IPv6 class.
 */
template <class A>
				  class NotifyQueueInvalidateEntry : public NotifyQueueEntry 
{
	public:
		/**
		 * NotifyQueueInvalidateEntry Constructor
		 *
		 * @param net the valid_subnet of the route registration that is
		 * now invalid.
		 * @param multicast true indicates that the change occured in the
		 * multicast RIB, false indicates that it occured in the unicast
		 * RIB.  
		 */
		NotifyQueueInvalidateEntry(const IPNet<A>& net, bool multicast)
			: _net(net), _multicast(multicast) {}

		/**
		 * @return INVALIDATE
		 * @see NotifyQueueEntry::EntryType
		 */
		EntryType type() const { return INVALIDATE; }

		/**
		 * Actually send the XRL that communicates this change to the
		 * registered process.
		 *
		 * @param response_sender the auto-generated stub class instance
		 * that will do the parameter marchalling.
		 * @param module_name the XRL module target name to send this
		 * information to.
		 * @param cb the method to call back when this XRL completes.
		 */
		void send(ResponseSender* response_sender,
				const string& module_name,
				NotifyQueue::XrlCompleteCB& cb);

	private:
		IPNet<A>	_net;	// The valid_subnet from the RouteRegister
		// instance.  The other end already knows the
		// route's full subnet.
		bool	_multicast;	// If true, a change occured in multicast RIB,
		// otherwise it occured in the unicast RIB
};

/**
 * @short RegisterServer handles communication of notifications to the
 * clients that registered interest in changes affecting specific
 * routes.
 *
 * RegisterServer handles communication of notifications to the
 * clients that registered interest in changes affecting specific
 * routes.  As these notifications can sometimes be generated faster
 * than the recipient can handle them, the notifications can be queued
 * here. 
 */
class RegisterServer 
{
	public:
		/**
		 * RegisterServer constructor
		 *
		 * @param xrl_router the XRL router instance used to send and
		 * receive XRLs in this process.
		 */
		RegisterServer(XrlRouter* xrl_router);

		/**
		 * RegisterServer destructor
		 */
		virtual ~RegisterServer() {}

		/** 
		 * send_route_changed is called to communicate to another XRL
		 * module that routing information in which it had registered an
		 * interest has changed its nexthop, metric, or admin distance.
		 *
		 * @param module_name the XRL target name of the module to notify.
		 * @param net the destination subnet of the route that changed.
		 * @param nexthop the new nexthop of the route that changed.
		 * @param metric the new routing protocol metric of the route that changed.
		 * @param admin_distance the new admin distance of the route that changed.
		 * @param protocol_origin the name of the protocol that originated this
		 * route.
		 * @param multicast true indicates that the change occured in the
		 * multicast RIB, false indicates that it occured in the unicast
		 * RIB.
		 */
		virtual void send_route_changed(const string& module_name,
				const IPv4Net& net,
				const IPv4& nexthop,
				uint32_t metric,
				uint32_t admin_distance,
				const string& protocol_origin,
				bool multicast);
		virtual void send_invalidate(const string& module_name,
				const IPv4Net& net,
				bool multicast);

#ifdef HAVE_IPV6
		/** 
		 * send_route_changed is called to communicate to another XRL
		 * module that routing information in which it had registered an
		 * interest has changed its nexthop, metric, or admin distance.
		 *
		 * @param module_name the XRL target name of the module to notify.
		 * @param net the destination subnet of the route that changed.
		 * @param nexthop the new nexthop of the route that changed.
		 * @param metric the new routing protocol metric of the route that changed.
		 * @param admin_distance the new admin distance of the route that changed.
		 * @param protocol_origin the name of the protocol that originated this
		 * route.
		 * @param multicast true indicates that the change occured in the
		 * multicast RIB, false indicates that it occured in the unicast
		 * RIB.
		 */
		virtual void send_route_changed(const string& module_name,
				const IPv6Net& net,
				const IPv6& nexthop,
				uint32_t metric,
				uint32_t admin_distance,
				const string& protocol_origin,
				bool multicast);

		/**
		 * send_invalidate is called to communicate to another XRL module
		 * that routing information in which it had registered an interest
		 * has changed in some unspecified way that has caused the
		 * registration to become invalid.  The client must re-register to
		 * find out what really happened.
		 *
		 * @param module_name the XRL target name of the module to notify.
		 * @param net the valid_subnet of the route registration that is
		 * now invalid.
		 * @param multicast true indicates that the change occured in the
		 * multicast RIB, false indicates that it occured in the unicast
		 * RIB.  
		 */
		virtual void send_invalidate(const string& module_name,
				const IPv6Net& net,
				bool multicast);
#endif

		/**
		 * @see NotifyQueue::flush
		 */
		virtual void flush();

	protected:
		void add_entry_to_queue(const string& module_name, NotifyQueueEntry* e);
		map<string, NotifyQueue* > _queuemap;
		ResponseSender _response_sender;
};

#endif // __RIB_REGISTER_SERVER_HH__
