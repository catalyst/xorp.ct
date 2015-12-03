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



#include "rip_module.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_atoms.hh"

#include "port.hh"
#include "xrl_port_manager.hh"
#include "xrl_port_io.hh"

// ----------------------------------------------------------------------------
// Utility methods

/**
 * Query whether an address exists on given interface and vif path and all
 * items on path are enabled.
 */
template <typename A>
	static bool
address_exists(const IfMgrIfTree&	iftree,
		const string&		ifname,
		const string&		vifname,
		const A&			addr)
{
	debug_msg("Looking for %s/%s/%s\n",
			ifname.c_str(), vifname.c_str(), addr.str().c_str());

	const IfMgrIfAtom* ia = iftree.find_interface(ifname);
	if (ia == NULL)
		return false;

	const IfMgrVifAtom* va = ia->find_vif(vifname);
	if (va == NULL)
		return false;

	const typename IfMgrIP<A>::Atom* aa = va->find_addr(addr);
	if (aa == NULL)
		return false;

	return true;
}

/**
 * Query whether an address exists on given interface and vif path and all
 * items on path are enabled.
 */
template <typename A>
	static bool
address_enabled(const IfMgrIfTree&	iftree,
		const string&		ifname,
		const string&		vifname,
		const A&		addr)
{
	debug_msg("Looking for %s/%s/%s\n",
			ifname.c_str(), vifname.c_str(), addr.str().c_str());

	const IfMgrIfAtom* ia = iftree.find_interface(ifname);
	if (ia == 0 || ia->enabled() == false || ia->no_carrier()) 
	{
		debug_msg("if %s exists ? %d ?\n", ifname.c_str(), (ia ? 1 : 0));
		return false;
	}

	const IfMgrVifAtom* va = ia->find_vif(vifname);
	if (va == 0 || va->enabled() == false) 
	{
		debug_msg("vif %s exists ? %d?\n", vifname.c_str(), (va ? 1: 0));
		return false;
	}

	const typename IfMgrIP<A>::Atom* aa = va->find_addr(addr);
	if (aa == 0 || aa->enabled() == false) 
	{
		debug_msg("addr %s exists ? %d?\n", addr.str().c_str(), (aa ? 1: 0));
		return false;
	}
	debug_msg("Found\n");
	return true;
}

/**
 * Unary function object to test whether a particular address is
 * associated with a RIP port.
 */
template <typename A>
struct port_has_address 
{
	port_has_address(const A& addr) : _addr(addr) {}
	bool operator() (const Port<A>* p) 
	{
		const PortIOBase<A>* io = p->io_handler();
		return io && io->address() == _addr;
	}
	private:
	A _addr;
};

/**
 * Unary function object to test whether a particular port's io
 * system is in a given state.
 */
template <typename A>
struct port_has_io_in_state 
{
	port_has_io_in_state(ServiceStatus st) : _st(st) {}

	bool operator() (const Port<A>* p) const
	{
		const PortIOBase<A>* 	io  = p->io_handler();
		const XrlPortIO<A>* 	xio = dynamic_cast<const XrlPortIO<A>*>(io);
		if (xio == 0)
			return false;
		return xio->status() == _st;
	}
	protected:
	ServiceStatus _st;
};

/**
 * Unary function object to test whether a particular RIP port is appropriate
 * for packet arriving on socket.
 *
 * NB At a future date we might want to track socket id to XrlPortIO
 * mappings.  This would be more efficient.
 */
template <typename A>
struct is_port_for 
{
	is_port_for(const string* sockid, const string* ifname,
			const string* vifname, const A* addr, IfMgrXrlMirror* im)
		: _psid(sockid),
		_ifname(ifname),
		_vifname(vifname),
		_pa(addr),
		_pim(im)
	{}

	bool operator() (Port<A>*& p);

	protected:
	bool link_addr_valid() const;

	private:
	const string* 	_psid;
	const string*	_ifname;
	const string*	_vifname;
	const A* 		_pa;
	IfMgrXrlMirror* 	_pim;
};

template <>
inline bool
is_port_for<IPv4>::link_addr_valid() const
{
	return true;
}

template <>
inline bool
is_port_for<IPv6>::link_addr_valid() const
{
	return _pa->is_linklocal_unicast();
}

template <typename A>
	bool
is_port_for<A>::operator() (Port<A>*& p)
{
	PortIOBase<A>* 	io  = p->io_handler();
	XrlPortIO<A>* 	xio = dynamic_cast<XrlPortIO<A>*>(io);
	bool rv;

	if (xio == 0) 
	{
		//XLOG_INFO("is_port_for, xio is NULL.\n");
		return false;
	}

	//
	// Perform address family specific check for source address being
	// link-local.  For IPv4 the concept does not exist, for IPv6
	// check if origin is link local.
	//
	if (link_addr_valid() == false) 
	{
		//XLOG_INFO("is_port_for: %s, link-addr-valid == false.\n", xio->ifname().c_str());
		return false;
	}

	// If another socket, ignore
	if (xio->socket_id() != *_psid) 
	{
		//XLOG_INFO("is_port_for: %s socket mismatch: %s %s\n",
		//	  xio->ifname().c_str(), xio->socket_id().c_str(), _psid->c_str());
		return false;
	}

	// If our packet, ignore
	if (xio->address() == *_pa) 
	{
		//XLOG_INFO("is_port_for: %s address mismatch: %s %s\n",
		//	  xio->ifname().c_str(), xio->address().str().c_str(),
		//	  _pa->str().c_str());
		return false;
	}

	// Check the incoming interface and vif name (if known)
	if ((! _ifname->empty()) && (! _vifname->empty())) 
	{
		if (xio->ifname() != *_ifname) 
		{
			//XLOG_INFO("is_port_for: %s ifname mismatch: %s\n",
			//      xio->ifname().c_str(), _ifname->c_str());
			return false;
		}
		if (xio->vifname() != *_vifname) 
		{
			//XLOG_INFO("is_port_for: %s vifname mismatch: %s %s\n",
			//      xio->ifname().c_str(), xio->vifname().c_str(), _vifname->c_str());
			return false;
		}
	}

	//
	// Packet has arrived on multicast socket and is not one of ours.
	//
	// Check source address to find originating neighbour on local nets
	// or p2p link.
	//
	const typename IfMgrIP<A>::Atom* ifa;
	ifa = _pim->iftree().find_addr(xio->ifname(),
			xio->vifname(),
			xio->address());

	if (ifa == 0) 
	{
		//XLOG_INFO("is_port_for: %s  ifa == 0\n",
		//	  xio->ifname().c_str());
		return false;
	}
	if (ifa->has_endpoint()) 
	{
		rv = ifa->endpoint_addr() == *_pa;
		if (!rv) 
		{
			//XLOG_INFO("is_port_for: %s  has-endpoint failed\n",
			//      xio->ifname().c_str());
		}
		return rv;
	}

	IPNet<A> n(ifa->addr(), ifa->prefix_len());
	rv = n.contains(*_pa);
	if (!rv) 
	{
		//XLOG_INFO("is_port_for: %s  subnet-contains failed, n: %s  adddr: %s/%d\n",
		//  xio->ifname().c_str(), n.str().c_str(), ifa->addr().str().c_str(),
		//  ifa->prefix_len());
	}
	return rv;
}

// ----------------------------------------------------------------------------
// XrlPortManager

template <typename A>
	int
XrlPortManager<A>::startup()
{
	set_status(SERVICE_STARTING);
	// Transition to SERVICE_RUNNING occurs when tree_complete() is called, ie
	// we have interface/vif/address state available.

	return (XORP_OK);
}

template <typename A>
	int
XrlPortManager<A>::shutdown()
{
	set_status(SERVICE_SHUTTING_DOWN);

	typename PortManagerBase<A>::PortList& pl = this->ports();
	typename PortManagerBase<A>::PortList::iterator i = pl.begin();

	// XXX Walk ports and shut them down.  Only when they are all
	// shutdown should we consider ourselves shutdown.

	debug_msg("XXX XrlPortManager<A>::shutdown (%p)\n", this);
	debug_msg("XXX n_ports = %u n_dead_ports %u\n",
			XORP_UINT_CAST(this->ports().size()),
			XORP_UINT_CAST(_dead_ports.size()));

	while (i != pl.end()) 
	{
		Port<A>* p = *i;
		XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
		if (xio) 
		{
			_dead_ports.insert(make_pair(xio, p));
			xio->shutdown();
			pl.erase(i++);
			debug_msg("   XXX killing port %p\n", p);
		} else 
		{
			i++;
			debug_msg("   XXX skipping port %p\n", p);
		}
	}

	return (XORP_OK);
}

template <typename A>
	void
XrlPortManager<A>::tree_complete()
{
	debug_msg("XrlPortManager<IPv%u>::tree_complete notification\n",
			XORP_UINT_CAST(A::ip_version()));
	set_status(SERVICE_RUNNING);
}

template <typename A>
	void
XrlPortManager<A>::updates_made()
{
	debug_msg("XrlPortManager<IPv%u>::updates_made notification\n",
			XORP_UINT_CAST(A::ip_version()));

	// Scan ports and enable/disable underlying I/O handler
	// according to fea state
	typename PortManagerBase<A>::PortList::iterator pi;
	for (pi = this->ports().begin(); pi != this->ports().end(); ++pi) 
	{
		Port<A>* p = *pi;
		XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
		if (xio == 0)
			continue;
		bool fea_en = address_enabled(_ifm.iftree(), xio->ifname(),
				xio->vifname(), xio->address());
		if (fea_en != xio->enabled()) 
		{
			XLOG_INFO("Detected iftree change on %s %s %s setting transport enabled %s",
					xio->ifname().c_str(), xio->vifname().c_str(),
					xio->address().str().c_str(),
					bool_c_str(fea_en));
			xio->set_enabled(fea_en);
		}
	}
}

template <typename A>
	bool
XrlPortManager<A>::add_rip_address(const string& ifname,
		const string& vifname,
		const A&	 addr)
{
	if (status() != SERVICE_RUNNING) 
	{
		debug_msg("add_rip_address failed: not running.\n");
		return false;
	}

	// Check whether address exists, fail if not.
	const IfMgrIfTree& iftree = _ifm.iftree();
	if (address_exists(iftree, ifname, vifname, addr) == false) 
	{
		debug_msg("add_rip_address failed: address does not exist.\n");
		return false;
	}

	// Check if port already exists
	typename PortManagerBase<A>::PortList::const_iterator pi;
	pi = find_if(this->ports().begin(), this->ports().end(),
			port_has_address<A>(addr));
	if (pi != this->ports().end())
		return true;

	// Create port
	Port<A>* p = new Port<A>(*this);
	this->ports().push_back(p);

	// Create XrlPortIO object for port
	XrlPortIO<A>* io = new XrlPortIO<A>(_xr, *p, ifname, vifname, addr);

	// Bind io to port
	p->set_io_handler(io, false);

	// Add self to observers of io objects status
	io->set_observer(this);

	// Start port's I/O handler if no others are starting up already
	try_start_next_io_handler();

	return true;
}

template <typename A>
	bool
XrlPortManager<A>::remove_rip_address(const string& 	/* ifname */,
		const string&	/* vifname */,
		const A&	addr)
{
	typename PortManagerBase<A>::PortList& pl = this->ports();
	typename PortManagerBase<A>::PortList::iterator i;
	i = find_if(pl.begin(), pl.end(), port_has_address<A>(addr));
	if (i != pl.end()) 
	{
		Port<A>* p = *i;
		XrlPortIO<A>* xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
		if (xio) 
		{
			_dead_ports.insert(make_pair(xio, p));
			xio->shutdown();
		}
		pl.erase(i);
	}
	return true;
}

template <typename A>
	bool
XrlPortManager<A>::deliver_packet(const string& 		sockid,
		const string&			ifname,
		const string&			vifname,
		const A& 			src_addr,
		uint16_t 			src_port,
		const vector<uint8_t>& 	pdata)
{
	typename PortManagerBase<A>::PortList& pl = this->ports();
	typename PortManagerBase<A>::PortList::iterator i;
	Port<A>* p = NULL;

	XLOG_TRACE(packet_trace()._packets,
			"Packet on %s from interface %s vif %s %s/%u %u bytes\n",
			sockid.c_str(), ifname.c_str(), vifname.c_str(),
			src_addr.str().c_str(), src_port, XORP_UINT_CAST(pdata.size()));
	debug_msg("Packet on %s from interface %s vif %s %s/%u %u bytes\n",
			sockid.c_str(), ifname.c_str(), vifname.c_str(),
			src_addr.str().c_str(), src_port, XORP_UINT_CAST(pdata.size()));

	i = find_if(pl.begin(), pl.end(),
			is_port_for<A>(&sockid, &ifname, &vifname, &src_addr, &_ifm));
	if (i == this->ports().end()) 
	{

		XLOG_TRACE(packet_trace()._packets,
				"Discarding packet %s/%u %u bytes\n",
				src_addr.str().c_str(), src_port,
				XORP_UINT_CAST(pdata.size()));
		debug_msg("Discarding packet %s/%u %u bytes\n",
				src_addr.str().c_str(), src_port,
				XORP_UINT_CAST(pdata.size()));
		return false;
	}
	p = *i;
	XLOG_ASSERT(find_if(++i, pl.end(),
				is_port_for<A>(&sockid, &ifname, &vifname, &src_addr,
					&_ifm))
			== pl.end());

	p->port_io_receive(src_addr, src_port, &pdata[0], pdata.size());

	return true;
}

template <typename A>
	Port<A>*
XrlPortManager<A>::find_port(const string& 	ifname,
		const string& 	vifname,
		const A&		addr)
{
	typename PortManagerBase<A>::PortList::iterator pi;
	pi = find_if(this->ports().begin(), this->ports().end(),
			port_has_address<A>(addr));
	if (pi == this->ports().end()) 
	{
		return 0;
	}

	Port<A>* port = *pi;
	PortIOBase<A>* port_io = port->io_handler();
	if (port_io->ifname() != ifname || port_io->vifname() != vifname) 
	{
		return 0;
	}
	return port;
}

template <typename A>
const Port<A>*
XrlPortManager<A>::find_port(const string& 	ifname,
		const string& 	vifname,
		const A&		addr) const
{
	typename PortManagerBase<A>::PortList::const_iterator pi;
	pi = find_if(this->ports().begin(), this->ports().end(),
			port_has_address<A>(addr));
	if (pi == this->ports().end()) 
	{
		return 0;
	}

	const Port<A>* port = *pi;
	const PortIOBase<A>* port_io = port->io_handler();
	if (port_io->ifname() != ifname || port_io->vifname() != vifname) 
	{
		return 0;
	}
	return port;
}

template <typename A>
bool
XrlPortManager<A>::underlying_rip_address_up(const string&	ifname,
		const string&	vifname,
		const A&		addr) const
{
	return address_enabled(_ifm.iftree(), ifname, vifname, addr);
}

template <typename A>
bool
XrlPortManager<A>::underlying_rip_address_exists(const string&	ifname,
		const string&	vifname,
		const A&	addr) const
{
	return _ifm.iftree().find_addr(ifname, vifname, addr) != 0;
}

template <typename A>
	void
XrlPortManager<A>::status_change(ServiceBase* 	service,
		ServiceStatus 	/* old_status */,
		ServiceStatus 	new_status)
{
	debug_msg("XXX %p status -> %s\n",
			service, service_status_name(new_status));

	try_start_next_io_handler();

	if (new_status != SERVICE_SHUTDOWN)
		return;

	typename map<ServiceBase*, Port<A>*>::iterator i;
	i = _dead_ports.find(service);
	XLOG_ASSERT(i != _dead_ports.end());
	//    delete i->second;
	//    _dead_ports.erase(i);
}

	template <typename A>
XrlPortManager<A>::~XrlPortManager()
{
	_ifm.detach_hint_observer(this);

	while (_dead_ports.empty() == false) 
	{
		Port<A>* p = _dead_ports.begin()->second;
		PortIOBase<A>* io = p->io_handler();
		delete io;
		delete p;
		_dead_ports.erase(_dead_ports.begin());
	}
}

template <typename A>
	void
XrlPortManager<A>::try_start_next_io_handler()
{
	typename PortManagerBase<A>::PortList::const_iterator cpi;
	cpi = find_if(this->ports().begin(), this->ports().end(),
			port_has_io_in_state<A>(SERVICE_STARTING));
	if (cpi != this->ports().end()) 
	{
		return;
	}

	typename PortManagerBase<A>::PortList::iterator pi = this->ports().begin();
	XrlPortIO<A>* xio = 0;
	while (xio == 0) 
	{
		pi = find_if(pi, this->ports().end(),
				port_has_io_in_state<A>(SERVICE_READY));
		if (pi == this->ports().end()) 
		{
			return;
		}
		Port<A>* p = (*pi);
		xio = dynamic_cast<XrlPortIO<A>*>(p->io_handler());
		pi++;
	}
	xio->startup();
}

#ifdef INSTANTIATE_IPV4
template class XrlPortManager<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class XrlPortManager<IPv6>;
#endif
