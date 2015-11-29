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

// $XORP: xorp/pim/pim_nbr.hh,v 1.20 2008/10/02 21:57:54 bms Exp $


#ifndef __PIM_PIM_NBR_HH__
#define __PIM_PIM_NBR_HH__


//
// PIM router neighbors definitions.
//


#include "libxorp/ipvx.hh"
#include "libxorp/timer.hh"
#include "pim_proto_join_prune_message.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class PimVif;

class PimNbr 
{
	public:
		PimNbr(PimVif* pim_vif, const IPvX& primary_addr, int proto_version);
		~PimNbr();

		void	reset_received_options();
		PimNode*	pim_node()	const	{ return _pim_node; }
		PimVif*	pim_vif()	const	{ return _pim_vif; }
		uint32_t	vif_index()	const;
		const IPvX&	primary_addr()	const	{ return (_primary_addr); }
		void	set_primary_addr(const IPvX& v) { _primary_addr = v; }
		const list<IPvX>& secondary_addr_list() const { return _secondary_addr_list; }
		void	add_secondary_addr(const IPvX& v);
		void	delete_secondary_addr(const IPvX& v);
		void	clear_secondary_addr_list() { _secondary_addr_list.clear(); }
		bool	has_secondary_addr(const IPvX& secondary_addr) const;
		bool	is_my_addr(const IPvX& addr) const;
		int		proto_version() const { return (_proto_version); }
		void	set_proto_version(int v) { _proto_version = v; }

		uint16_t	hello_holdtime() const { return (_hello_holdtime); }
		void	set_hello_holdtime(uint16_t v) { _hello_holdtime = v; }

		uint32_t	genid()		const	{ return (_genid);	}
		void	set_genid(uint32_t v) { _genid = v; }
		bool	is_genid_present() const { return (_is_genid_present); }
		void	set_is_genid_present(bool v) { _is_genid_present = v; }

		uint32_t	dr_priority() const { return (_dr_priority); }
		void	set_dr_priority(uint32_t v) { _dr_priority = v; }
		bool	is_dr_priority_present() const 
		{
			return (_is_dr_priority_present);
		}
		void	set_is_dr_priority_present(bool v) 
		{
			_is_dr_priority_present = v;
		}

		bool	is_tracking_support_disabled() const 
		{
			return (_is_tracking_support_disabled);
		}
		void	set_is_tracking_support_disabled(bool v) 
		{
			_is_tracking_support_disabled = v;
		}

		uint16_t	propagation_delay() const { return (_propagation_delay); }
		void	set_propagation_delay(uint16_t v) { _propagation_delay = v; }

		uint16_t	override_interval() const { return (_override_interval); }
		void	set_override_interval(uint16_t v) { _override_interval = v; }

		bool	is_lan_prune_delay_present() const 
		{
			return (_is_lan_prune_delay_present);
		}
		void	set_is_lan_prune_delay_present(bool v) 
		{
			_is_lan_prune_delay_present = v;
		}

		void	pim_hello_holdtime_process(uint16_t holdtime);
		void	pim_hello_lan_prune_delay_process(bool lan_prune_delay_tbit,
				uint16_t propagation_delay,
				uint16_t override_interval);
		void	pim_hello_dr_priority_process(uint32_t dr_priority);
		void	pim_hello_genid_process(uint32_t genid);

		bool	is_nohello_neighbor() const { return (_is_nohello_neighbor); }
		void	set_is_nohello_neighbor(bool v) { _is_nohello_neighbor = v; }

		int		jp_entry_add(const IPvX& source_addr, const IPvX& group_addr,
				uint8_t group_mask_len,
				mrt_entry_type_t mrt_entry_type,
				action_jp_t action_jp, uint16_t holdtime,
				bool is_new_group);

		const XorpTimer& const_neighbor_liveness_timer() const 
		{
			return (_neighbor_liveness_timer);
		}

		const TimeVal& startup_time() const { return _startup_time; }
		void	set_startup_time(const TimeVal& v) { _startup_time = v; }

		list<PimMre *>& pim_mre_rp_list()	{ return (_pim_mre_rp_list); }
		list<PimMre *>& pim_mre_wc_list()	{ return (_pim_mre_wc_list); }
		list<PimMre *>& pim_mre_sg_list()	{ return (_pim_mre_sg_list); }
		list<PimMre *>& pim_mre_sg_rpt_list() { return (_pim_mre_sg_rpt_list); }
		list<PimMre *>& processing_pim_mre_rp_list() 
		{
			return (_processing_pim_mre_rp_list);
		}
		list<PimMre *>& processing_pim_mre_wc_list() 
		{
			return (_processing_pim_mre_wc_list);
		}
		list<PimMre *>& processing_pim_mre_sg_list() 
		{
			return (_processing_pim_mre_sg_list);
		}
		list<PimMre *>& processing_pim_mre_sg_rpt_list() 
		{
			return (_processing_pim_mre_sg_rpt_list);
		}
		void	init_processing_pim_mre_rp();
		void	init_processing_pim_mre_wc();
		void	init_processing_pim_mre_sg();
		void	init_processing_pim_mre_sg_rpt();
		void	add_pim_mre(PimMre *pim_mre);
		void	delete_pim_mre(PimMre *pim_mre);

	private:
		friend class PimVif;

		void	neighbor_liveness_timer_timeout();
		void	jp_send_timer_timeout();

		// Fields to hold information from the PIM_HELLO messages
		PimNode*	_pim_node;		// The associated PIM node
		PimVif*	_pim_vif;		// The corresponding PIM vif
		IPvX	_primary_addr;		// The primary address of the neighbor
		list<IPvX>	_secondary_addr_list;	// The secondary addresses of the neighbor
		int		_proto_version;		// The protocol version of the neighbor
		uint32_t	_genid;			// The Gen-ID of the neighbor
		bool	_is_genid_present;	// Is the Gen-ID field is present
		uint32_t	_dr_priority;		// The DR priority of the neighbor
		bool	_is_dr_priority_present;// Is the DR priority field present
		uint16_t	_hello_holdtime;	// The Holdtime from/for this nbr
		XorpTimer	_neighbor_liveness_timer; // Timer to expire the neighbor
		// LAN Prune Delay option related info
		bool	_is_tracking_support_disabled; // The T-bit
		uint16_t	_propagation_delay;	// The Propagation_Delay
		uint16_t	_override_interval;	// The Override_Interval
		bool	_is_lan_prune_delay_present;// Is the LAN Prune Delay present
		bool	_is_nohello_neighbor;	// True if no-Hello neighbor

		XorpTimer	_jp_send_timer;		// Timer to send the accumulated JP msg

		PimJpHeader _jp_header;

		TimeVal	_startup_time;		// Start-up time of this neighbor

		//
		// The lists of all related PimMre entries.
		// XXX: note that we don't have a list for PimMfc entries, because
		// they don't depend directly on PimNbr. For example, PimMfc has
		// a field for the RP address, therefore PimRp contains a list of
		// PimMfc entries related to that RP.
		//
		list<PimMre *> _pim_mre_rp_list;	// List of all related (*,*,RP) entries
		// for this PimNbr.
		list<PimMre *> _pim_mre_wc_list;	// List of all related (*,G) entries
		// for this PimNbr.
		list<PimMre *> _pim_mre_sg_list;	// List of all related (S,G) entries
		// for this PimNbr.
		list<PimMre *> _pim_mre_sg_rpt_list;// List of all related (S,G,rpt)
		// entries for this PimNbr.
		list<PimMre *> _processing_pim_mre_rp_list;	// List of all related
		// (*,*,RP) entries for this PimNbr
		// that are awaiting to be processed.
		list<PimMre *> _processing_pim_mre_wc_list;	// List of all related
		// (*,G) entries for this PimNbr
		// that are awaiting to be processed.
		list<PimMre *> _processing_pim_mre_sg_list;	// List of all related
		// (S,G) entries for this PimNbr that
		// are awaiting to be processed.
		list<PimMre *> _processing_pim_mre_sg_rpt_list;// List of all related
		// (S,G,rpt) entries for this PimNbr
		// that are awaiting to be processed.
};


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_NBR_HH__
