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



#include "output_updates.hh"
#include "packet_assembly.hh"
#include "packet_queue.hh"
#include "route_db.hh"

    template <typename A>
OutputUpdates<A>::OutputUpdates( Port<A>&	port,
	PacketQueue<A>&	pkt_queue,
	RouteDB<A>&	rdb,
	const A&	dst_addr,
	uint16_t	dst_port)
: OutputBase<A>( port, pkt_queue, dst_addr, dst_port),
    _uq(rdb.update_queue())
{
}

    template <typename A>
OutputUpdates<A>::~OutputUpdates()
{
    stop_output_processing();
}

template <typename A>
    void
OutputUpdates<A>::ffwd()
{
    _uq.ffwd(_uq_iter);
}

template <typename A>
    void
OutputUpdates<A>::output_packet()
{
    ResponsePacketAssembler<A> rpa(this->_port);
    RipPacket<A>* pkt = new RipPacket<A>(this->ip_addr(), this->ip_port());
    rpa.packet_start(pkt);

    set<const RouteEntry<A>*> added_routes;
    uint32_t done = 0;
    const RouteEntry<A>* r = 0;
    for (r = _uq.get(_uq_iter); r != 0; r = _uq.next(_uq_iter)) 
    {
	if (added_routes.find(r) != added_routes.end())
	    continue;

	pair<A,uint16_t> p = this->_port.route_policy(*r);

	if (p.second > RIP_INFINITY)
	    continue;

	RouteEntryOrigin<A>* origin = NULL;
	string ifname, vifname;		// XXX: not set, because not needed
	RouteEntry<A>* copy = new RouteEntry<A>(r->net(), p.first,
		ifname, vifname,
		p.second,
		origin, r->tag(),
		r->policytags());

	// Policy EXPORT filtering was done here.
	// It's moved to RouteDB<A>::do_filtering.
	// This was done because EXPORT filter could possibly change route metric,
	// and thus make it lower then RIP_INFINITY
	// Routes with cost > RIP_INFINTY would never come here, because they would be filtered out in RouteDB<A>::update_route
	// - IMAR

	rpa.packet_add_route(copy->net(), copy->nexthop(), copy->cost(), r->tag());
	added_routes.insert(r);
	delete copy;

	done++;
	if (rpa.packet_full()) 
	{
	    _uq.next(_uq_iter);
	    break;
	}
    }

    list<RipPacket<A>*> auth_packets;
    if (done == 0 || rpa.packet_finish(auth_packets) == false) 
    {
	// No routes added to packet or error finishing packet off.
    } else 
    {
	typename list<RipPacket<A>*>::iterator iter;
	for (iter = auth_packets.begin(); iter != auth_packets.end(); ++iter) 
	{
	    RipPacket<A>* auth_pkt = *iter;
	    this->_pkt_queue.enqueue_packet(auth_pkt);
	    this->_port.counters().incr_triggered_updates();
	    this->incr_packets_sent();
	}
	this->_port.push_packets();
    }
    delete pkt;

    if (r != 0) 
    {
	// Not finished with updates so set time to reschedule self
	this->_op_timer 
	    = EventLoop::instance().new_oneoff_after_ms(this->interpacket_gap_ms(),
		    callback(this, &OutputUpdates<A>::output_packet));
    } else 
    {
	// Finished with updates for this run.  Do not set timer.
    }
}

    template <typename A>
void OutputUpdates<A>::start_output_processing()
{
    if (_uq.reader_valid(_uq_iter) == false) 
    {
	_uq_iter = _uq.create_reader();
    }
    output_packet();
}

    template <typename A>
void OutputUpdates<A>::stop_output_processing()
{
    _uq.destroy_reader(_uq_iter);
    this->_op_timer.unschedule();
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class OutputUpdates<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class OutputUpdates<IPv6>;
#endif
