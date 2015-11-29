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



#include "rip_module.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "route_entry.hh"

template <typename A>
    inline void
RouteEntry<A>::dissociate()
{
    RouteEntryOrigin<A>* o = _origin;
    _origin = 0;
    if (o) 
    {
	o->dissociate(this);
    }
}

template <typename A>
    inline void
RouteEntry<A>::associate(Origin* o)
{
    if (o)
	o->associate(this);
    _origin = o;
}

    template <typename A>
RouteEntry<A>::RouteEntry(const Net&	n,
	const Addr&	nh,
	const string&	ifname,
	const string&	vifname,
	uint16_t	cost,
	Origin*&	o,
	uint16_t	tag)
: _net(n), _nh(nh), _ifname(ifname), _vifname(vifname),
    _cost(cost), _tag(tag), _ref_cnt(0), _filtered(false)
{
    associate(o);
}

    template <typename A>
RouteEntry<A>::RouteEntry(const Net&		n,
	const Addr&		nh,
	const string&		ifname,
	const string&		vifname,
	uint16_t		cost,
	Origin*&		o,
	uint16_t		tag,
	const PolicyTags&	policytags)
: _net(n), _nh(nh), _ifname(ifname), _vifname(vifname),
    _cost(cost), _tag(tag), _ref_cnt(0), _policytags(policytags),
    _filtered(false)
{
    associate(o);
}

    template <typename A>
RouteEntry<A>::~RouteEntry()
{
    Origin* o = _origin;
    _origin = 0;
    if (o) 
    {
	o->dissociate(this);
    }
}

template <typename A>
string
RouteEntry<A>::str() const 
{
    ostringstream oss;
    oss << " net: " << _net.str() << " nexthop: " << _nh.str() << " vif: " << _ifname << ":" << _vifname
	<< " cost: " << _cost << " tag: " << _tag << " refcnt: " << _ref_cnt
	<< " policytags: " << _policytags.str() << " filtered: " << _filtered << flush;
    return oss.str();
}


template <typename A>
    bool
RouteEntry<A>::set_nexthop(const A& nh)
{
    if (nh != _nh) 
    {
	_nh = nh;
	//
	// XXX: If the nexthop is not link-local or zero, then reset
	// the interface and vif name.
	// Ideally, we shouldn't do this if the policy mechanism has
	// support for setting the interface and vif name.
	// For the time being it is safer to reset them and let the RIB
	// find the interface and vif name based on the common subnet
	// address to the nexthop.
	//
	if (! (_nh.is_linklocal_unicast() || _nh.is_zero())) 
	{
	    set_ifname("");
	    set_vifname("");
	}
	return true;
    }
    return false;
}

template <typename A>
    bool
RouteEntry<A>::set_ifname(const string& ifname)
{
    if (ifname != _ifname) 
    {
	_ifname = ifname;
	return true;
    }
    return false;
}

template <typename A>
    bool
RouteEntry<A>::set_vifname(const string& vifname)
{
    if (vifname != _vifname) 
    {
	_vifname = vifname;
	return true;
    }
    return false;
}

template <typename A>
    bool
RouteEntry<A>::set_cost(uint16_t cost)
{
    if (cost != _cost) 
    {
	_cost = cost;
	return true;
    }
    return false;
}

template <typename A>
    bool
RouteEntry<A>::set_origin(Origin* o)
{
    if (o != _origin) 
    {
	dissociate();
	associate(o);
	return true;
    }
    return false;
}

template <typename A>
    bool
RouteEntry<A>::set_tag(uint16_t tag)
{
    if (tag != _tag) 
    {
	_tag = tag;
	return true;
    }
    return false;
}


template <typename A>
    bool
RouteEntry<A>::set_policytags(const PolicyTags& ptags)
{
    if (ptags != _policytags) 
    {
	_policytags = ptags;
	return true;
    }
    return false;
}


/**
 * A comparitor for the purposes of sorting containers of RouteEntry objects.
 * It examines the data rather than using the address of pointers.  The
 * latter approach makes testing difficult on different platforms since the
 * tests may inadvertantly make assumptions about the memory layout.  The
 * comparison is arbitrary, it just has to be consistent and reversible.
 *
 * IFF speed proves to be an issue, RouteEntry can be changed to be an element
 * in an intrusive linked list that has a sentinel embedded in the
 * RouteEntryOrigin.
 */
template <typename A>
struct NetCmp 
{
    bool operator() (const IPNet<A>& l, const IPNet<A>& r) const
    {
	if (l.prefix_len() < r.prefix_len())
	    return true;
	if (l.prefix_len() > r.prefix_len())
	    return false;
	return l.masked_addr() < r.masked_addr();
    }
};

// ----------------------------------------------------------------------------
// RouteEntryOrigin<A>::RouteEntryStore and related
//
// RouteEntryStore is a private class that is used by Route Origin's to store
// their associated routes.
//



template <typename A>
struct RouteEntryOrigin<A>::RouteEntryStore 
{
    public:
	typedef map<IPNet<A>, RouteEntry<A>*, NetCmp<A> > Container;
	Container routes;
};


// ----------------------------------------------------------------------------
// RouteEntryOrigin

    template <typename A>
    RouteEntryOrigin<A>::RouteEntryOrigin(bool is_rib_origin)
: _is_rib_origin(is_rib_origin)
{
    _rtstore = new RouteEntryStore();
}

    template <typename A>
RouteEntryOrigin<A>::~RouteEntryOrigin()
{
    // XXX store should be empty
    XLOG_ASSERT(_rtstore->routes.empty());
    delete _rtstore;
}

template <typename A>
    bool
RouteEntryOrigin<A>::associate(Route* r)
{
    XLOG_ASSERT(r != 0);
    if (_rtstore->routes.find(r->net()) != _rtstore->routes.end()) 
    {
	XLOG_FATAL("entry already exists");
	return false;
    }
    _rtstore->routes.insert(typename
	    RouteEntryStore::Container::value_type(r->net(), r));
    return true;
}

template <typename A>
    bool
RouteEntryOrigin<A>::dissociate(Route* r)
{
    typename RouteEntryStore::Container::iterator
	i = _rtstore->routes.find(r->net());

    if (i == _rtstore->routes.end()) 
    {
	XLOG_FATAL("entry does not exist");
	return false;
    }
    _rtstore->routes.erase(i);
    return true;
}

template <typename A>
uint32_t
RouteEntryOrigin<A>::route_count() const
{
    return static_cast<uint32_t>(_rtstore->routes.size());
}

template <typename A>
    void
RouteEntryOrigin<A>::clear()
{
    typename RouteEntryStore::Container::iterator
	i = _rtstore->routes.begin();
    while (i != _rtstore->routes.end()) 
    {
	Route* r = i->second;
	delete r;
	i = _rtstore->routes.begin();
    }
}

template <typename A>
void
RouteEntryOrigin<A>::dump_routes(vector<const Route*>& routes) const
{
    typename RouteEntryStore::Container::const_iterator
	i = _rtstore->routes.begin();
    typename RouteEntryStore::Container::const_iterator
	end = _rtstore->routes.end();

    while (i != end) 
    {
	routes.push_back(i->second);
	++i;
    }
}

template <typename A>
RouteEntry<A>*
RouteEntryOrigin<A>::find_route(const Net& n) const
{
    typename RouteEntryStore::Container::const_iterator
	i = _rtstore->routes.find(n);
    if (i == _rtstore->routes.end())
	return 0;
    return i->second;
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class RouteEntryOrigin<IPv4>;
template class RouteEntry<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class RouteEntryOrigin<IPv6>;
template class RouteEntry<IPv6>;
#endif
